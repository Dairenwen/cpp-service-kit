#include "BreezeCode.hpp"
#include "BreezeLog.hpp"
#include <algorithm>
#include <condition_variable>
#include <cstdlib>
#include <ctime>
#include <exception>
#include <fstream>
#include <functional>
#include <iomanip>
#include <locale>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>
#include <utility>

namespace
{
    using Clock = std::chrono::steady_clock;
    using Deadline = Clock::time_point;
    using CurlList = std::unique_ptr<curl_slist, decltype(&curl_slist_free_all)>;
    using Breezecode::mail_settings;                           // 使用hpp中定义的SMTP配置
    using Breezecode::MailMessage;                             // 使用hpp中定义的邮件数据结构
    constexpr long kConnectTimeoutMs = 15000;                  // libcurl 默认连接超时为 15 秒
    constexpr long kSmtpTimeoutMs = 30000;                     // libcurl 默认传输超时为 30 秒
    constexpr char kBinaryType[] = "application/octet-stream"; // libcurl 默认的 MIME 类型

    // 全局配置只初始化一次，并在进程结束时释放，避免多个客户端相互清理。
    struct CurlGlobal
    {
        CurlGlobal()
        {
            const auto result = curl_global_init(CURL_GLOBAL_DEFAULT);
            if (result != CURLE_OK)
            {
                ERR("初始化 CURL 全局配置失败: {}", curl_easy_strerror(result));
                std::abort();
            }
        }
        ~CurlGlobal() { curl_global_cleanup(); }
    };

    // Date 邮件头使用英文星期、月份及 UTC 时区，不受系统语言设置影响。
    std::string mailDate()
    {
        const std::time_t now = std::time(nullptr);
        std::tm utc{};
        if (!gmtime_r(&now, &utc))
            return {};
        std::ostringstream date;
        date.imbue(std::locale::classic());
        date << std::put_time(&utc, "%a, %d %b %Y %H:%M:%S +0000");
        return date.str();
    }

    // 检查邮件附件路径是否包含换行或空字符，避免邮件头注入攻击。
    bool invalidHeader(const std::string &value)
    {
        return value.find_first_of("\r\n") != std::string::npos || value.find('\0') != std::string::npos;
    }

    // libcurl 的 MIME 编码要求每行不超过 76 个字符，且使用 CRLF 换行。邮件主题可能包含 UTF-8 字符，需要 Base64 编码。
    std::string encodeSubject(const std::string &subject)
    {
        static constexpr char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        constexpr std::size_t kBytesPerWord = 39;
        std::string header = "Subject: ";
        for (std::size_t start = 0; start < subject.size();)
        {
            // 在 UTF-8 字符边界结束本段，超出首行的编码词折到下一行。
            auto end = start + std::min(kBytesPerWord, subject.size() - start);
            while (end < subject.size() && end > start &&
                   (static_cast<unsigned char>(subject[end]) & 0xC0) == 0x80)
                --end;
            if (end == start)
                return {};
            if (start != 0)
                header += "\r\n ";
            header += "=?UTF-8?B?";
            // 每三个原始字节编码成四个字符，不足三个字节时以等号补齐。
            for (auto i = start; i < end; i += 3)
            {
                const unsigned a = static_cast<unsigned char>(subject[i]);
                const unsigned b = i + 1 < end ? static_cast<unsigned char>(subject[i + 1]) : 0;
                const unsigned c = i + 2 < end ? static_cast<unsigned char>(subject[i + 2]) : 0;
                header += alphabet[a >> 2];
                header += alphabet[((a & 3) << 4) | (b >> 4)];
                header += i + 1 < end ? alphabet[((b & 15) << 2) | (c >> 6)] : '=';
                header += i + 2 < end ? alphabet[c & 63] : '=';
            }
            header += "?=";
            start = end;
        }
        return header;
    }

    // libcurl 用相同的 MIME 编码方式处理 HTML 正文、文件附件和内存附件
    curl_mimepart *addMimePart(curl_mime *mime, const char *type)
    {
        auto *part = curl_mime_addpart(mime);
        if (!part || curl_mime_type(part, type) != CURLE_OK || curl_mime_encoder(part, "base64") != CURLE_OK)
            return nullptr;
        return part;
    }

    // 根据邮件内容组装完整 MIME 多段结构，包括 HTML 正文、固定文件附件、动态内存附件，所有分段统一启用 base64 编码
    bool buildMime(curl_mime *mime, const MailMessage &message, const std::string &file_path)
    {
        // 1. 构造 HTML 正文，使用 Base64 编码，避免邮件客户端对 UTF-8 字符的错误处理
        auto *part = addMimePart(mime, "text/html; charset=UTF-8");
        if (!part || curl_mime_data(part, message.html.data(), message.html.size()) != CURLE_OK)
        {
            ERR("构造邮件正文失败");
            return false;
        }

        // 验证码邮件沿用配置中的附件文件，libcurl 按块读取。
        if (!file_path.empty())
        {
            if (invalidHeader(file_path))
            {
                ERR("邮件附件路径包含无效字符");
                return false;
            }
            std::ifstream file(file_path, std::ios::binary);
            part = addMimePart(mime, kBinaryType);
            // libcurl 流式读取磁盘文件
            if (!file || !part || curl_mime_filedata(part, file_path.c_str()) != CURLE_OK)
            {
                ERR("无法读取邮件附件");
                return false;
            }
        }
        for (const auto &attachment : message.attachments)
        {
            if (attachment.filename.empty() || invalidHeader(attachment.filename) || invalidHeader(attachment.content_type))
            {
                ERR("邮件附件名称或类型无效");
                return false;
            }
            const auto *type = attachment.content_type.empty() ? kBinaryType : attachment.content_type.c_str();
            part = addMimePart(mime, type);
            // libcurl 直接使用内存数据
            if (!part || curl_mime_data(part, attachment.data.data(), attachment.data.size()) != CURLE_OK ||
                curl_mime_filename(part, attachment.filename.c_str()) != CURLE_OK)
            {
                ERR("构造邮件附件失败");
                return false;
            }
        }
        return true;
    }

    // 追加邮件头到 libcurl 的 slist 链表，成功时仍由同一个智能指针管理完整链表
    bool appendHeader(CurlList &headers, const std::string &header)
    {
        auto *updated = curl_slist_append(headers.get(), header.c_str());
        if (!updated)
            return false;
        // 追加成功后仍由同一个智能指针管理完整链表。
        headers.release();
        headers.reset(updated);
        return true;
    }

    // 在发送过程中周期性检查是否超出业务层截止时间，超时则主动中止传输。
    int checkDeadline(void *data, curl_off_t, curl_off_t, curl_off_t, curl_off_t)
    {
        const auto *deadline = static_cast<const Deadline *>(data);
        return Clock::now() >= *deadline;
    }

    // 校验 → 创建句柄 → 配置连接 → 组装内容 → 设置超时 → 执行发送
    bool sendMessage(const mail_settings &settings, const MailMessage &message, const std::string &file_path, Deadline deadline)
    {
        // 1. 校验邮件头字段，为本次发送创建独立的 libcurl 句柄。
        if (invalidHeader(message.to) || invalidHeader(settings.from) || invalidHeader(message.subject))
        {
            ERR("邮件头包含无效字符");
            return false;
        }
        auto transfer_deadline = deadline; // 超时回调在句柄关闭前一直使用这个变量
        std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> curl(curl_easy_init(), &curl_easy_cleanup);
        if (!curl)
        {
            ERR("构造 CURL 操作句柄失败");
            return false;
        }
        auto *handle = curl.get();

        // 2. 配置 SMTP 连接、认证和信封地址，沿用 libcurl 默认的 TLS 校验。
        CurlList recipients(curl_slist_append(nullptr, message.to.c_str()), &curl_slist_free_all);
        if (!recipients || curl_easy_setopt(handle, CURLOPT_NOSIGNAL, 1L) != CURLE_OK ||
            curl_easy_setopt(handle, CURLOPT_URL, settings.url.c_str()) != CURLE_OK ||
            curl_easy_setopt(handle, CURLOPT_USERNAME, settings.username.c_str()) != CURLE_OK ||
            curl_easy_setopt(handle, CURLOPT_PASSWORD, settings.password.c_str()) != CURLE_OK ||
            curl_easy_setopt(handle, CURLOPT_MAIL_FROM, settings.from.c_str()) != CURLE_OK ||
            curl_easy_setopt(handle, CURLOPT_MAIL_RCPT, recipients.get()) != CURLE_OK)
        {
            ERR("设置 SMTP 连接或信封地址失败");
            return false;
        }
        if (!settings.login_options.empty() && curl_easy_setopt(handle, CURLOPT_LOGIN_OPTIONS, settings.login_options.c_str()) != CURLE_OK)
        {
            ERR("设置 SMTP 认证方式失败");
            return false;
        }

        // 3. 组装 UTF-8 HTML 正文和附件。
        std::unique_ptr<curl_mime, decltype(&curl_mime_free)> mime(curl_mime_init(handle), &curl_mime_free);
        if (!mime)
        {
            ERR("创建邮件 MIME 对象失败");
            return false;
        }
        if (!buildMime(mime.get(), message, file_path))
            return false;

        // 4. 生成收件端看到的日期、地址和主题，并绑定邮件内容。
        CurlList headers(nullptr, &curl_slist_free_all);
        const auto date = mailDate();
        const auto subject = encodeSubject(message.subject);
        if (date.empty() || subject.empty() || !appendHeader(headers, "Date: " + date) ||
            !appendHeader(headers, "From: " + settings.from) ||
            !appendHeader(headers, "To: " + message.to) || !appendHeader(headers, subject) ||
            curl_easy_setopt(handle, CURLOPT_HTTPHEADER, headers.get()) != CURLE_OK ||
            curl_easy_setopt(handle, CURLOPT_MIMEPOST, mime.get()) != CURLE_OK)
        {
            ERR("设置邮件头或正文失败");
            return false;
        }

        // 5. 按任务的剩余时间发送。关闭 SMTP 连接时也检查同一截止时间。
        const auto now = Clock::now();
        transfer_deadline = std::min(deadline, now + std::chrono::milliseconds(kSmtpTimeoutMs));
        const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(transfer_deadline - now);
        if (remaining <= std::chrono::milliseconds::zero())
            return false;
        const auto timeout_ms = static_cast<long>(remaining.count());
        const auto connect_timeout_ms = std::min(kConnectTimeoutMs, timeout_ms);
        // 设置 libcurl 的超时选项，CURLOPT_TIMEOUT_MS 和 CURLOPT_CONNECTTIMEOUT_MS 都是毫秒级别
        if (curl_easy_setopt(handle, CURLOPT_FORBID_REUSE, 1L) != CURLE_OK ||
            curl_easy_setopt(handle, CURLOPT_NOPROGRESS, 0L) != CURLE_OK ||
            curl_easy_setopt(handle, CURLOPT_XFERINFOFUNCTION, &checkDeadline) != CURLE_OK ||
            curl_easy_setopt(handle, CURLOPT_XFERINFODATA, &transfer_deadline) != CURLE_OK ||
            curl_easy_setopt(handle, CURLOPT_TIMEOUT_MS, timeout_ms) != CURLE_OK ||
            curl_easy_setopt(handle, CURLOPT_CONNECTTIMEOUT_MS, connect_timeout_ms) != CURLE_OK)
        {
            ERR("设置 SMTP 超时或连接关闭选项失败");
            return false;
        }
        // 6. 执行发送，SMTP 服务器接收邮件后返回 true
        const auto ret = curl_easy_perform(handle);
        if (ret != CURLE_OK)
        {
            ERR("请求邮件服务器失败: {}", curl_easy_strerror(ret));
            return false;
        }
        DBG("邮件发送成功");
        return true;
    }

    // 读取 HTML 模板并替换其中的验证码。
    std::string codeBody(const std::string &path, const std::string &code)
    {
        // 1. 打开调用方配置的 HTML 模板。
        std::ifstream file(path);
        if (!file)
        {
            ERR("无法读取验证码邮件模板: {}", path);
            return {};
        }
        // 2. 替换每行中的验证码占位符，统一使用邮件的 CRLF 换行。
        const std::string marker = "{{code}}";
        std::string html;
        std::string line;
        bool has_code = false;
        while (std::getline(file, line))
        {
            if (!line.empty() && line.back() == '\r')
                line.pop_back();
            std::size_t pos = line.find(marker);
            while (pos != std::string::npos)
            {
                line.replace(pos, marker.size(), code);
                has_code = true;
                pos = line.find(marker, pos + code.size());
            }
            html += line;
            html += "\r\n";
        }

        // 3. 确认模板读取完整且至少替换了一处占位符。
        if (file.bad())
        {
            ERR("读取验证码邮件模板失败: {}", path);
            return {};
        }
        if (!has_code)
        {
            ERR("验证码邮件模板缺少 {{{{code}}}} 标记: {}", path);
            return {};
        }
        return html;
    }

    // 发送验证码邮件，使用 HTML 模板和可选附件，返回 SMTP 服务器接收结果。
    bool sendCode(const mail_settings &settings, const std::string &to, const std::string &code, Deadline deadline)
    {
        auto html = codeBody(settings.html_path, code);
        if (html.empty())
            return false;
        return sendMessage(settings, {to, "Chat2Data-验证码（自动发送）", std::move(html), {}}, settings.attachment_path, deadline);
    }

    // 队列未启用或超出限额时，立即返回结果为 false 的 future
    std::future<bool> rejectedTask()
    {
        std::promise<bool> result; // 生产者
        result.set_value(false);
        return result.get_future();
    }
}

namespace Breezecode
{
    // 实现一个简单的异步队列，按提交顺序发送邮件。队列启用时由一个后台线程处理任务，调用方无需关心具体实现。
    class MailClient::Queue
    {
    public:
        explicit Queue(const MailQueueOptions &options) : _options(options)
        {
            if (_options.max_tasks == 0 || _options.max_payload_bytes == 0 || _options.task_timeout <= std::chrono::milliseconds::zero())
                throw std::invalid_argument("邮件队列容量和任务超时必须大于零");
            _worker = std::thread(&Queue::run, this); // 启动后台线程处理任务
        }

        ~Queue()
        {
            // 等待所有已提交任务处理完毕后退出线程
            {
                std::lock_guard<std::mutex> lock(_mutex);
                _stopping = true;
            }
            // 唤醒后台线程，确保退出前处理完所有已接收的任务
            _ready.notify_one();
            _worker.join();
        }

        // 向队列提交发送任务，返回关联的 future<bool>，调用方可后续获取结果
        std::future<bool> submit(std::size_t bytes, std::function<bool(Deadline)> send)
        {
            // 1. 从提交时开始计时，将任务和结果交给队列。
            const auto now = Clock::now();
            const auto maximum = std::chrono::duration_cast<std::chrono::milliseconds>(Deadline::max() - now);
            Task task{now + std::min(_options.task_timeout, maximum), bytes, std::move(send), {}};

            auto result = task.result.get_future(); // 与调用方的 future<bool> 关联，调用方可后续获取结果
            {
                // 2. 在同一把锁下检查限额，并为已接收的任务占用额度。
                std::lock_guard<std::mutex> lock(_mutex);
                if (_stopping || _pending == _options.max_tasks || bytes > _options.max_payload_bytes - _bytes)
                {
                    // 队列未启用或超出限额时，立即返回结果为 false 的 future
                    task.result.set_value(false);
                    return result;
                }
                _tasks.push(std::move(task));
                ++_pending;
                _bytes += bytes;
            }
            // 3. 唤醒后台线程，调用方可以稍后读取发送结果。
            _ready.notify_one();
            return result;
        }

    private:
        // 任务结构体
        struct Task
        {
            Deadline deadline;                  // 任务截止时间，超过时直接返回 false
            std::size_t bytes;                  // 任务负载字节数，用于限额计算
            std::function<bool(Deadline)> send; // 任务处理函数，调用时传入截止时间，返回 SMTP 发送结果
            std::promise<bool> result;          // 关联 future<bool>，只能够写入一次，调用方可后续获取结果
        };

        void run()
        {
            for (;;)
            {
                // 1. 等待任务或关闭通知。关闭时继续取出已接收的任务。
                std::unique_lock<std::mutex> lock(_mutex);
                _ready.wait(lock, [this]
                            { return _stopping || !_tasks.empty(); });
                if (_tasks.empty())
                    return;
                auto task = std::move(_tasks.front());
                _tasks.pop();
                lock.unlock();

                // 2. 在锁外发送，跳过超过截止时间的任务。
                bool sent = false;
                std::exception_ptr error;
                try
                {
                    sent = Clock::now() < task.deadline && task.send(task.deadline);
                }
                catch (...)
                {
                    error = std::current_exception();
                }
                // 3. 释放任务内存和额度，让调用方收到结果后可以立即提交下一封邮件。
                task.send = {};
                lock.lock();
                --_pending;
                _bytes -= task.bytes;
                lock.unlock();

                // 4. 把发送结果或异常交给这一封邮件的 future。
                if (error)
                    task.result.set_exception(error);
                else
                    task.result.set_value(sent);
            }
        }

        const MailQueueOptions _options; // 队列配置
        std::mutex _mutex;               // 互斥锁
        std::condition_variable _ready;  // 条件变量
        std::queue<Task> _tasks;         // 任务队列
        std::size_t _pending = 0;        // 当前排队和正在发送的任务数
        std::size_t _bytes = 0;          // 当前排队和正在发送的任务总负载字节数
        bool _stopping = false;          // 队列正在关闭，后台线程退出后不再接收新任务
        std::thread _worker;             // 处理任务的后台线程
    };

    MailClient::MailClient(const mail_settings &settings)
        : _settings(std::make_shared<const mail_settings>(settings))
    {
        // 进程内的客户端共享一次全局初始化，随后每封邮件使用自己的传输句柄。
        static CurlGlobal global;
        (void)global;
    }

    MailClient::MailClient(const mail_settings &settings, const MailQueueOptions &queue_options)
        : MailClient(settings)
    {
        _queue = std::make_shared<Queue>(queue_options);
    }

    // 同步发送验证码
    bool MailClient::send(const std::string &to, const std::string &code)
    {
        return sendCode(*_settings, to, code, Deadline::max());
    }

    // 同步发送普通邮件
    bool MailClient::sendMail(const MailMessage &message)
    {
        return sendMessage(*_settings, message, {}, Deadline::max());
    }

    // 异步发送验证码
    std::future<bool> MailClient::sendAsync(std::string to, std::string code)
    {
        if (!_queue)
            return rejectedTask();
        const auto bytes = to.size() + code.size();
        auto task = [settings = _settings, to = std::move(to), code = std::move(code)](Deadline deadline)
        {
            return sendCode(*settings, to, code, deadline);
        };
        return _queue->submit(bytes, std::move(task));
    }

    // 异步发送普通邮件
    std::future<bool> MailClient::sendMailAsync(MailMessage message)
    {
        if (!_queue)
            return rejectedTask();
        auto bytes = message.to.size() + message.subject.size() + message.html.size();
        for (const auto &attachment : message.attachments)
            bytes += sizeof(MailAttachment) + attachment.filename.size() + attachment.data.size() + attachment.content_type.size();
        auto task = [settings = _settings, message = std::move(message)](Deadline deadline)
        {
            return sendMessage(*settings, message, {}, deadline);
        };
        return _queue->submit(bytes, std::move(task));
    }
}
