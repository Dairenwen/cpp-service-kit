#pragma once

#include <curl/curl.h>
#include <memory>
#include <sstream>
#include <string>

namespace Breezecode
{
    // 发送验证码所需的 SMTP 配置，由调用方提供
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
        // 可选附件路径；留空时只发送 HTML 正文。
        std::string attachment_path;
    };

    // 验证码客户端接口；未来可增加其他发送方式，便于扩展，调用时无需关心具体实现
    class CodeClient
    {
    public:
        CodeClient() = default;
        virtual ~CodeClient() = default; // 支持通过基类指针正确释放派生对象。
        virtual bool send(const std::string &to, const std::string &code) = 0;
    };

    // 使用 libcurl 和 SMTP 发送 HTML 验证码邮件。
    class MailClient : public CodeClient
    {
    public:
        using ptr = std::shared_ptr<MailClient>;
        MailClient(const mail_settings &settings);
        ~MailClient() override = default;

        // to 是收件人邮箱，code 是待发送的验证码；成功返回 true。
        bool send(const std::string &to, const std::string &code) override;

    private:
        // 读取 HTML 模板并把 {{code}} 替换成验证码。
        std::stringstream codeBody(const std::string &code);

        mail_settings _settings;
    };
}
