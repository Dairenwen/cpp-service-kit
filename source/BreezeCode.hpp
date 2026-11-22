#pragma once

#include <curl/curl.h>
#include <chrono>
#include <cstddef>
#include <future>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace Breezecode
{
    // 验证码邮件和普通邮件共用的 SMTP 配置，由调用方提供。
    struct mail_settings
    {
        std::string username; // 邮箱登录账号。
        std::string password; // 邮箱的客户端授权码。
        std::string url;      // SMTP 服务器地址，如 smtps://smtp.qq.com:465。
        std::string from;     // 信封发件人，通常与登录邮箱相同。
        // 可选登录方式；QQ 邮箱在当前环境可设置为 AUTH=LOGIN。
        std::string login_options;
        // 验证码邮件的 HTML 模板路径
        std::string html_path;
        // 验证码邮件的可选附件路径，普通邮件通过 MailMessage 传入附件。
        std::string attachment_path;
    };

    // 邮件附件数据结构，包含文件名、原始字节和 MIME 类型。
    struct MailAttachment
    {
        std::string filename;     // 收件人看到的文件名。
        std::string data;         // 原始文件字节。
        std::string content_type; // 留空时使用 application/octet-stream。
    };

    // 邮件数据结构，包含收件人、主题、HTML 正文和附件列表。
    struct MailMessage
    {
        std::string to;      // 收件人邮箱。
        std::string subject; // UTF-8 邮件主题。
        std::string html;    // UTF-8 HTML 正文。
        std::vector<MailAttachment> attachments;
    };

    // 邮件发送队列的配置选项，允许限制任务数量、负载大小和超时时间。
    struct MailQueueOptions
    {
        std::size_t max_tasks = 32;                        // 默认最大任务数为 32，超过时新任务将被拒绝。
        std::size_t max_payload_bytes = 512 * 1024 * 1024; // 默认最大负载为 512MB，超过时新任务将被拒绝。
        std::chrono::milliseconds task_timeout{30000};     // 从提交到 SMTP 发送结束的超时时间，默认 30 秒。
    };

    // 验证码客户端接口；未来可增加其他发送方式，便于扩展，调用时无需关心具体实现
    class CodeClient
    {
    public:
        CodeClient() = default;
        virtual ~CodeClient() = default; // 支持通过基类指针正确释放派生对象。
        virtual bool send(const std::string &to, const std::string &code) = 0;
    };

    // 使用 libcurl 和 SMTP 发送邮件。队列启用时由一个后台线程按提交顺序发送。
    class MailClient : public CodeClient
    {
    public:
        using ptr = std::shared_ptr<MailClient>;
        // 构造函数传入 SMTP 配置和可选队列配置，队列启用时将异步发送邮件，否则默认同步发送方式
        MailClient(const mail_settings &settings);
        MailClient(const mail_settings &settings, const MailQueueOptions &queue_options);
        ~MailClient() override = default;

        // 同步发送验证码或普通邮件，SMTP 服务器接收邮件后返回 true
        bool send(const std::string &to, const std::string &code) override;
        bool sendMail(const MailMessage &message);

        // 异步方法接管任务数据。调用后立即返回 future<bool>，后台线程处理任务并在完成后设置结果，
        // 调用方通过 future.get() 获取结果
        std::future<bool> sendAsync(std::string to, std::string code);
        std::future<bool> sendMailAsync(MailMessage message);

    private:
        class Queue;
        std::shared_ptr<const mail_settings> _settings; // SMTP 配置
        std::shared_ptr<Queue> _queue;                  // 任务队列
    };
}
