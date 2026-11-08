#include "BreezeCode.hpp"

#include <gtest/gtest.h>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

std::vector<std::string> recipients;

// 真实邮件测试：向自己的 QQ 邮箱发送 HTML 验证码和 PNG 附件。
TEST(BreezeCodeTest, SendRealMail)
{

    Breezecode::mail_settings settings;
    settings.username = "3484725023@qq.com";                           // QQ 邮箱登录账号。
    settings.password = "lshcqpcfdmilxxxx";                            // 该账号生成的客户端授权码。
    settings.url = "smtps://smtp.qq.com:465";                          // QQ 邮箱加密 SMTP 服务。
    settings.from = "3484725023@qq.com";                               // 发件人使用登录邮箱。
    settings.login_options = "AUTH=LOGIN";                             // 当前 QQ 邮箱使用 LOGIN 认证。
    settings.html_path = BREEZE_MAIL_TEST_DIR "/mail.html";            // 正文模板中的 {{code}} 会被替换。
    settings.attachment_path = BREEZE_MAIL_TEST_DIR "/attachment.png"; // 待发送的二进制附件。

    // 每次生成一个六位测试码，便于在收件箱核对这次测试的邮件。
    const auto now = std::chrono::system_clock::now().time_since_epoch().count();
    const std::string code = std::to_string(100000 + now % 900000);
    std::cout << "本次测试验证码: " << code << std::endl;

    Breezecode::MailClient client(settings);
    ASSERT_FALSE(recipients.empty()) << "请在命令行参数中提供至少一个收件人邮箱";
    for (const auto &recipient : recipients)
        EXPECT_TRUE(client.send(recipient, code)) << "发送失败，收件人: " << recipient;
}

int main(int argc, char **argv)
{
    // Google Test 先处理自身选项，剩余参数按空白字符拆分为收件人。
    ::testing::InitGoogleTest(&argc, argv);
    for (int i = 1; i < argc; ++i)
    {
        std::istringstream input(argv[i]);
        std::string recipient;
        while (input >> recipient)
            recipients.push_back(recipient);
    }
    return RUN_ALL_TESTS();
}
