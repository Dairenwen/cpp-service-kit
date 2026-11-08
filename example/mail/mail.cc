#include <curl/curl.h>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>

// 替换成已开启 SMTP 服务的授权邮箱地址。
const std::string username = "3484725023@qq.com";
// SMTP over TLS 服务地址；QQ 邮箱的 465 端口使用 smtps 协议。
const std::string url = "smtps://smtp.qq.com:465";
// 信封发件人通常与授权邮箱相同；to 是邮件的实际投递地址。
const std::string from = "3484725023@qq.com";
const std::string to = "1962478161@qq.com";

// libcurl 需要上传邮件数据时调用：最多读取 size * nitems 个字节。
// userdata 指向下方构造的邮件流；返回实际读取字节数，0 表示读完。
size_t callback(char *buffer, size_t size, size_t nitems, void *userdata)
{
    std::stringstream *ss = (std::stringstream *)userdata; // userdata 指向传入的邮件地址
    ss->read(buffer, size * nitems);
    return ss->gcount();
}

int main()
{
    std::ifstream html(MAIL_TEMPLATE_PATH);
    if (!html)
    {
        std::cerr << "无法打开邮件模板: " << MAIL_TEMPLATE_PATH << std::endl;
        return -1;
    }

    // 邮件头部和正文之间必须有一个空行。主题使用 UTF-8 Base64 编码， 避免邮件客户端无法正确显示中文；正文声明使用 UTF-8 的 HTML。
    std::stringstream ss;
    ss << "From: " << from << "\r\n";
    ss << "To: " << to << "\r\n";
    ss << "Subject: =?UTF-8?B?6aqM6K+B56CB?=\r\n"; // 「验证码」
    ss << "MIME-Version: 1.0\r\n";
    ss << "Content-Type: text/html; charset=UTF-8\r\n";
    ss << "Content-Transfer-Encoding: 8bit\r\n";
    ss << "\r\n";

    // 按行读取 HTML，统一转换成邮件要求的 CRLF 换行。
    std::string line;
    while (std::getline(html, line))
    {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        ss << line << "\r\n";
    }
    if (html.bad())
    {
        std::cerr << "读取邮件模板失败: " << MAIL_TEMPLATE_PATH << std::endl;
        return -1;
    }

    // 1. 初始化 libcurl 的全局环境；成功后才能创建操作句柄。
    auto ret = curl_global_init(CURL_GLOBAL_DEFAULT);
    if (ret != CURLE_OK)
    {
        std::cout << "curl_global_init failed: ";
        std::cout << curl_easy_strerror(ret) << std::endl;
        return -1;
    }

    // 2. 创建本次 SMTP 操作使用的 easy 句柄。
    auto curl = curl_easy_init();
    if (curl == nullptr)
    {
        std::cout << "curl_easy_init failed" << std::endl;
        return -1;
    }

    // 3. 设置 SMTP 服务器地址与认证信息；curl_easy_setopt 返回错误码。
    ret = curl_easy_setopt(curl, CURLOPT_URL, url.c_str()); // 设置
    if (ret != CURLE_OK)
    {
        std::cout << "curl_easy_setopt CURLOPT_URL failed: ";
        std::cout << curl_easy_strerror(ret) << std::endl;
        return -1;
    }
    ret = curl_easy_setopt(curl, CURLOPT_USERNAME, username.c_str()); // 设置用户名
    if (ret != CURLE_OK)
    {
        std::cout << "curl_easy_setopt CURLOPT_USERNAME failed: ";
        std::cout << curl_easy_strerror(ret) << std::endl;
        return -1;
    }
    ret = curl_easy_setopt(curl, CURLOPT_PASSWORD, "lshcqpcfdmilchjj"); // 设置授权码（非邮箱密码）
    if (ret != CURLE_OK)
    {
        std::cout << "curl_easy_setopt CURLOPT_PASSWORD failed: ";
        std::cout << curl_easy_strerror(ret) << std::endl;
        return -1;
    }

    // 外层 smtps 仍使用 TLS 加密，LOGIN 是 TLS 连接内部的认证方式。
    ret = curl_easy_setopt(curl, CURLOPT_LOGIN_OPTIONS, "AUTH=LOGIN");
    if (ret != CURLE_OK)
    {
        std::cout << "curl_easy_setopt CURLOPT_LOGIN_OPTIONS failed: ";
        std::cout << curl_easy_strerror(ret) << std::endl;
        return -1;
    }

    // 信封发件人用于 SMTP 投递，与邮件正文中的 From 头是不同的设置。
    ret = curl_easy_setopt(curl, CURLOPT_MAIL_FROM, from.c_str()); // 设置发件人
    if (ret != CURLE_OK)
    {
        std::cout << "curl_easy_setopt CURLOPT_MAIL_FROM failed: ";
        std::cout << curl_easy_strerror(ret) << std::endl;
        return -1;
    }

    // 收件人以链表提供；多个收件人可以继续调用 curl_slist_append。
    struct curl_slist *cs = nullptr;
    cs = curl_slist_append(cs, to.c_str());
    ret = curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, cs); // 设置收件人
    if (ret != CURLE_OK)
    {
        std::cout << "curl_easy_setopt CURLOPT_MAIL_RCPT failed: ";
        std::cout << curl_easy_strerror(ret) << std::endl;
        return -1;
    }

    // 将邮件流传给回调；libcurl 上传时会通过 callback 读取邮件字节。
    ret = curl_easy_setopt(curl, CURLOPT_READDATA, (void *)&ss); // 设置要发送的邮件
    if (ret != CURLE_OK)
    {
        std::cout << "curl_easy_setopt CURLOPT_READDATA failed: ";
        std::cout << curl_easy_strerror(ret) << std::endl;
        return -1;
    }
    ret = curl_easy_setopt(curl, CURLOPT_READFUNCTION, callback); // 设置回调函数，当 libcurl 需要读取数据时会调用该函数
    if (ret != CURLE_OK)
    {
        std::cout << "curl_easy_setopt CURLOPT_READFUNCTION failed: ";
        std::cout << curl_easy_strerror(ret) << std::endl;
        return -1;
    }
    // 启用上传，libcurl 才会通过回调提供的内容发送邮件。
    ret = curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L); // 设置上传模式，1L 表示启用上传
    if (ret != CURLE_OK)
    {
        std::cout << "curl_easy_setopt CURLOPT_UPLOAD failed: ";
        std::cout << curl_easy_strerror(ret) << std::endl;
        return -1;
    }

    // 4. 连接 SMTP 服务器并发送邮件；非 CURLE_OK 时输出错误原因。
    ret = curl_easy_perform(curl);
    if (ret != CURLE_OK)
    {
        std::cout << "curl_easy_perform failed: ";
        std::cout << curl_easy_strerror(ret) << std::endl;
        return -1;
    }

    // 5. 发送成功后依次释放收件人链表、easy 句柄和 libcurl 全局资源。
    curl_slist_free_all(cs);
    curl_easy_cleanup(curl);
    curl_global_cleanup();
    return 0;
}
