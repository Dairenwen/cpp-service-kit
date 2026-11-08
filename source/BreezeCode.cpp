#include "BreezeCode.hpp"
#include "BreezeLog.hpp"
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <locale>
#include <memory>

namespace
{
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
}

namespace Breezecode
{
    MailClient::MailClient(const mail_settings &settings) : _settings(settings)
    {
        // 函数内静态对象由 C++ 保证只初始化一次，多个实例共享其生命周期。
        static CurlGlobal global;
        (void)global; // 避免编译器警告未使用变量
    }

    bool MailClient::send(const std::string &to, const std::string &code)
    {
        // 收件人和发件人会写入邮件头，不能包含换行或 NUL。
        if (to.find_first_of("\r\n") != std::string::npos ||
            to.find('\0') != std::string::npos ||
            _settings.from.find_first_of("\r\n") != std::string::npos ||
            _settings.from.find('\0') != std::string::npos)
        {
            ERR("邮件地址包含无效字符");
            return false;
        }
        // 模板读取失败或没有验证码标记时，不连接服务器。
        auto body = codeBody(code);
        const std::string html = body.str();
        if (html.empty())
            return false;

        // 附件为可选项；如果指定，发送前先检查文件是否可读。
        if (!_settings.attachment_path.empty())
        {
            std::ifstream attachment(_settings.attachment_path, std::ios::binary);
            if (!attachment || _settings.attachment_path.find('\0') != std::string::npos)
            {
                ERR("无法读取邮件附件: {}", _settings.attachment_path);
                return false;
            }
        }
        std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> curl(curl_easy_init(), &curl_easy_cleanup); // 指定析构函数
        if (!curl)
        {
            ERR("构造 CURL 操作句柄失败");
            return false;
        }

        // 设置连接和总超时时间；保留 libcurl 默认的 TLS 证书与主机名验证。
        auto setOption = [&](CURLoption option, auto value, const char *name)
        {
            const auto ret = curl_easy_setopt(curl.get(), option, value);
            if (ret != CURLE_OK)
                ERR("设置 CURL 的 {} 参数失败: {}", name, curl_easy_strerror(ret));
            return ret == CURLE_OK;
        };
        if (!setOption(CURLOPT_CONNECTTIMEOUT, 15L, "CURLOPT_CONNECTTIMEOUT") ||            // 连接超时 15 秒
            !setOption(CURLOPT_TIMEOUT, 30L, "CURLOPT_TIMEOUT") ||                          // 总超时 30 秒
            !setOption(CURLOPT_URL, _settings.url.c_str(), "CURLOPT_URL") ||                // 设置 SMTP 服务器地址
            !setOption(CURLOPT_USERNAME, _settings.username.c_str(), "CURLOPT_USERNAME") || // 设置登录用户名
            !setOption(CURLOPT_PASSWORD, _settings.password.c_str(), "CURLOPT_PASSWORD"))   // 设置登录密码（客户端授权码）
            return false;

        // 如果配置了认证方式（如 AUTH=LOGIN），将其交给 libcurl。
        if (!_settings.login_options.empty() && !setOption(CURLOPT_LOGIN_OPTIONS, _settings.login_options.c_str(), "CURLOPT_LOGIN_OPTIONS"))
            return false;
        if (!setOption(CURLOPT_MAIL_FROM, _settings.from.c_str(), "CURLOPT_MAIL_FROM"))
            return false;

        // SMTP 收件人以链表传入，直到本次发送结束都必须保持有效。
        std::unique_ptr<curl_slist, decltype(&curl_slist_free_all)> recipients(curl_slist_append(nullptr, to.c_str()), &curl_slist_free_all);
        if (!recipients)
        {
            ERR("创建邮件收件人列表失败");
            return false;
        }
        if (!setOption(CURLOPT_MAIL_RCPT, recipients.get(), "CURLOPT_MAIL_RCPT")) // 设置收件人
            return false;

        // MIME 负责组织 HTML 正文和可选附件；附件由 libcurl 分块读取并编码。
        std::unique_ptr<curl_mime, decltype(&curl_mime_free)> mime(curl_mime_init(curl.get()), &curl_mime_free);
        auto *part = mime ? curl_mime_addpart(mime.get()) : nullptr;
        // 正文使用 UTF-8 编码，base64 传输，避免中文乱码。
        if (!part || curl_mime_data(part, html.data(), html.size()) != CURLE_OK ||
            curl_mime_type(part, "text/html; charset=UTF-8") != CURLE_OK ||
            curl_mime_encoder(part, "base64") != CURLE_OK)
        {
            ERR("构造邮件正文失败");
            return false;
        }
        if (!_settings.attachment_path.empty())
        {
            part = curl_mime_addpart(mime.get()); // 添加附件
            // 附件使用二进制流传输，base64 编码，避免乱码。
            if (!part || curl_mime_filedata(part, _settings.attachment_path.c_str()) != CURLE_OK ||
                curl_mime_type(part, "application/octet-stream") != CURLE_OK ||
                curl_mime_encoder(part, "base64") != CURLE_OK)
            {
                ERR("构造邮件附件失败");
                return false;
            }
        }

        // SMTP 信封地址负责投递，邮件头负责在收件客户端中显示。
        std::unique_ptr<curl_slist, decltype(&curl_slist_free_all)> headers(nullptr, &curl_slist_free_all);
        auto addHeader = [&](const std::string &header)
        {
            auto *updated = curl_slist_append(headers.get(), header.c_str()); // 添加邮件头
            if (!updated)
                return false;
            headers.release();
            headers.reset(updated); // 释放旧链表，使用新链表
            return true;
        };
        const auto date = mailDate();
        // 固定主题“这是您的验证码（自动发送）”使用 MIME 编码，避免中文乱码。
        if (date.empty() || !addHeader("Date: " + date) || !addHeader("From: " + _settings.from) ||
            !addHeader("To: " + to) ||
            !addHeader("Subject: =?UTF-8?B?6L+Z5piv5oKo55qE6aqM6K+B56CB77yI6Ieq5Yqo5Y+R6YCB77yJ?="))
        {
            ERR("构造邮件头失败");
            return false;
        }
        // 设置邮件头和 MIME 内容
        if (!setOption(CURLOPT_HTTPHEADER, headers.get(), "CURLOPT_HTTPHEADER") ||
            !setOption(CURLOPT_MIMEPOST, mime.get(), "CURLOPT_MIMEPOST"))
            return false;

        const auto ret = curl_easy_perform(curl.get()); // 发送邮件
        if (ret != CURLE_OK)
        {
            ERR("请求邮件服务器失败: {}", curl_easy_strerror(ret));
            return false;
        }
        DBG("邮件发送成功");
        return true;
    }

    std::stringstream MailClient::codeBody(const std::string &code)
    {
        std::stringstream ss;
        // 读取调用方指定的 HTML 模板
        std::ifstream file(_settings.html_path);
        if (!file)
        {
            ERR("无法读取验证码邮件模板: {}", _settings.html_path);
            return ss;
        }
        std::stringstream content;
        std::string line;
        bool has_code = false;
        while (std::getline(file, line))
        {
            if (!line.empty() && line.back() == '\r')
                line.pop_back();
            // 将模板中的 {{code}} 替换为本次传入的验证码。
            size_t pos = 0;
            while ((pos = line.find("{{code}}", pos)) != std::string::npos)
            {
                line.replace(pos, 8, code);
                pos += code.size();
                has_code = true;
            }
            content << line << "\r\n";
        }
        if (file.bad())
        {
            ERR("读取验证码邮件模板失败: {}", _settings.html_path);
            return ss;
        }
        if (!has_code)
        {
            ERR("验证码邮件模板缺少 {{{{code}}}} 标记: {}", _settings.html_path);
            return ss;
        }
        ss << content.str(); // 将读取并替换后的 HTML 文件内容作为正文。
        return ss;
    }
}
