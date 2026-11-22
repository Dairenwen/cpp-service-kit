#include "BreezeCode.hpp"

#include <arpa/inet.h>
#include <gtest/gtest.h>
#include <sys/socket.h>
#include <unistd.h>

#include <condition_variable>
#include <mutex>
#include <stdexcept>
#include <string_view>
#include <thread>

namespace
{
    using namespace std::chrono_literals;
    using Breezecode::MailClient;
    using Breezecode::MailMessage;
    using Breezecode::MailQueueOptions;

    constexpr char kRecipient[] = "recipient@example.test";

    // 本地 SMTP 接收端记录原始邮件，并可控制服务器确认邮件的时机。
    class SmtpReceiver
    {
    public:
        explicit SmtpReceiver(bool hold_reply = false, bool reject_first = false)
            : _hold_reply(hold_reply), _reject_first(reject_first)
        {
            _listener = socket(AF_INET, SOCK_STREAM, 0);
            if (_listener < 0)
                throw std::runtime_error("创建 SMTP 测试套接字失败");

            sockaddr_in address{};
            address.sin_family = AF_INET;
            address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
            socklen_t size = sizeof(address);
            if (bind(_listener, reinterpret_cast<sockaddr *>(&address), size) != 0 ||
                getsockname(_listener, reinterpret_cast<sockaddr *>(&address), &size) != 0 ||
                listen(_listener, 8) != 0)
            {
                close(_listener);
                throw std::runtime_error("启动本地 SMTP 接收端失败");
            }

            _port = ntohs(address.sin_port);
            _thread = std::thread(&SmtpReceiver::run, this);
        }

        ~SmtpReceiver()
        {
            releaseReply();
            shutdown(_listener, SHUT_RDWR);
            _thread.join();
            close(_listener);
        }

        Breezecode::mail_settings settings() const
        {
            Breezecode::mail_settings options;
            options.from = "sender@example.test";
            options.url = "smtp://127.0.0.1:" + std::to_string(_port);
            options.html_path = BREEZE_MAIL_TEST_DIR "/mail.html";
            return options;
        }

        bool waitFor(std::size_t count)
        {
            std::unique_lock<std::mutex> lock(_mutex);
            return _changed.wait_for(lock, 3s, [&] { return _messages.size() >= count; });
        }

        void releaseReply()
        {
            {
                std::lock_guard<std::mutex> lock(_mutex);
                _hold_reply = false;
            }
            _changed.notify_all();
        }

        std::vector<std::string> messages()
        {
            std::lock_guard<std::mutex> lock(_mutex);
            return _messages;
        }

    private:
        static bool reply(int connection, std::string_view text)
        {
            while (!text.empty())
            {
                const auto written = send(connection, text.data(), text.size(), MSG_NOSIGNAL);
                if (written <= 0)
                    return false;
                text.remove_prefix(written);
            }
            return true;
        }

        static bool readLine(int connection, std::string &pending, std::string &line)
        {
            for (;;)
            {
                const auto end = pending.find('\n');
                if (end != std::string::npos)
                {
                    line = pending.substr(0, end + 1);
                    pending.erase(0, end + 1);
                    return true;
                }
                char buffer[4096];
                const auto size = recv(connection, buffer, sizeof(buffer), 0);
                if (size <= 0)
                    return false;
                pending.append(buffer, size);
            }
        }

        void receive(int connection)
        {
            const timeval timeout{3, 0};
            setsockopt(connection, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
            std::string pending;
            std::string line;
            if (!reply(connection, "220 localhost SMTP test\r\n"))
                return;
            while (readLine(connection, pending, line))
            {
                if (line == "QUIT\r\n")
                {
                    reply(connection, "221 closing\r\n");
                    return;
                }
                if (line != "DATA\r\n")
                {
                    if (!reply(connection, "250 OK\r\n"))
                        return;
                    continue;
                }
                if (!reply(connection, "354 end with a dot\r\n"))
                    return;
                std::string message;
                while (readLine(connection, pending, line) && line != ".\r\n")
                    message += line;
                if (line != ".\r\n")
                    return;

                std::unique_lock<std::mutex> lock(_mutex);
                const bool reject = _reject_first && _messages.empty();
                _messages.push_back(std::move(message));
                _changed.notify_all();
                _changed.wait(lock, [&] { return !_hold_reply; });
                lock.unlock();
                if (!reply(connection, reject ? "550 rejected\r\n" : "250 accepted\r\n"))
                    return;
            }
        }

        void run()
        {
            for (;;)
            {
                const int connection = accept(_listener, nullptr, nullptr);
                if (connection < 0)
                    return;
                receive(connection);
                close(connection);
            }
        }

        int _listener = -1;
        int _port = 0;
        std::mutex _mutex;
        std::condition_variable _changed;
        std::vector<std::string> _messages;
        bool _hold_reply;
        const bool _reject_first;
        std::thread _thread;
    };

    // 解码收到的 MIME 内容，检查 SMTP 上传的字节，而不是检查发送端参数。
    std::string decodeBase64(std::string_view encoded)
    {
        constexpr std::string_view alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        std::string decoded;
        unsigned value = 0;
        int bits = 0;
        for (const auto ch : encoded)
        {
            if (ch == '=')
                break;
            const auto index = alphabet.find(ch);
            if (index == std::string_view::npos)
                continue;
            value = (value << 6) | static_cast<unsigned>(index);
            bits += 6;
            if (bits >= 8)
            {
                bits -= 8;
                decoded += static_cast<char>((value >> bits) & 0xFF);
            }
        }
        return decoded;
    }

    std::string readMimePart(const std::string &mail, const std::string &part)
    {
        const auto header = mail.find(part);
        if (header == std::string::npos)
            return {};
        const auto separator = mail.find("\r\n\r\n", header);
        if (separator == std::string::npos)
            return {};
        const auto start = separator + 4;
        const auto end = mail.find("\r\n--", start);
        return decodeBase64(std::string_view(mail).substr(start, end - start));
    }

    std::string readSubject(const std::string &mail)
    {
        constexpr std::string_view prefix = "=?UTF-8?B?";
        auto word = mail.find("Subject: ");
        std::string subject;
        while ((word = mail.find(prefix, word)) != std::string::npos)
        {
            const auto start = word + prefix.size();
            const auto end = mail.find("?=", start);
            if (end == std::string::npos)
                break;
            subject += decodeBase64(std::string_view(mail).substr(start, end - start));
            word = end + 2;
            if (mail.compare(word, 3, "\r\n ") != 0)
                break;
        }
        return subject;
    }

    TEST(BreezeCodeTest, SendsVerificationCodeAndConfiguredFile)
    {
        SmtpReceiver receiver;
        auto settings = receiver.settings();
        settings.attachment_path = BREEZE_MAIL_TEST_DIR "/attachment.png";
        MailClient client(settings);

        ASSERT_TRUE(client.send(kRecipient, "123456"));
        const auto mails = receiver.messages();
        ASSERT_EQ(mails.size(), 1u);
        EXPECT_NE(mails.front().find("To: recipient@example.test"), std::string::npos);
        EXPECT_EQ(readSubject(mails.front()), "Chat2Data-验证码（自动发送）");
        const auto html = readMimePart(mails.front(), "Content-Type: text/html");
        EXPECT_NE(html.find("123456"), std::string::npos);
        EXPECT_EQ(html.find("{{code}}"), std::string::npos);
        const auto attachment = readMimePart(mails.front(), "filename=\"attachment.png\"");
        EXPECT_EQ(attachment.substr(0, 8), std::string("\x89PNG\r\n\x1a\n", 8));
    }

    TEST(BreezeCodeTest, SendsHtmlAndMultipleAttachments)
    {
        SmtpReceiver receiver;
        MailClient client(receiver.settings());
        // 主题跨越编码词边界，包含多字节字符和空格。
        const std::string subject = "Chat2Data 报告：一二三四五六七八九十一二三四五六七八九十";
        const std::string binary("\0\xff\x10\r\n", 5);
        const MailMessage message{kRecipient, subject, "<p>分析结果</p>",
                                  {{"result.bin", binary, {}}, {"result.csv", "id,value\n1,2\n", "text/csv"}}};

        ASSERT_TRUE(client.sendMail(message));
        const auto mails = receiver.messages();
        ASSERT_EQ(mails.size(), 1u);
        EXPECT_EQ(readSubject(mails.front()), subject);
        EXPECT_EQ(readMimePart(mails.front(), "Content-Type: text/html"), message.html);
        EXPECT_NE(mails.front().find("Content-Type: application/octet-stream"), std::string::npos);
        EXPECT_NE(mails.front().find("Content-Type: text/csv"), std::string::npos);
        EXPECT_EQ(readMimePart(mails.front(), "filename=\"result.bin\""), binary);
        EXPECT_EQ(readMimePart(mails.front(), "filename=\"result.csv\""), "id,value\n1,2\n");
    }

    TEST(BreezeCodeTest, QueuesBothMailTypesAndReportsActualDelivery)
    {
        SmtpReceiver receiver(true);
        MailQueueOptions options;
        options.max_tasks = 2;
        options.task_timeout = 4s;
        MailClient client(receiver.settings(), options);

        auto code = client.sendAsync(kRecipient, "654321");
        ASSERT_TRUE(receiver.waitFor(1));
        EXPECT_EQ(code.wait_for(0ms), std::future_status::timeout);
        MailMessage message{kRecipient, "Report", "<p>original</p>", {}};
        auto mail = client.sendMailAsync(message);
        message.html = "<p>updated</p>";
        auto rejected = client.sendAsync(kRecipient, "111111");
        ASSERT_EQ(rejected.wait_for(0ms), std::future_status::ready);
        EXPECT_FALSE(rejected.get());
        receiver.releaseReply();

        EXPECT_TRUE(code.get());
        EXPECT_TRUE(mail.get());
        const auto mails = receiver.messages();
        ASSERT_EQ(mails.size(), 2u);
        EXPECT_NE(readMimePart(mails[0], "Content-Type: text/html").find("654321"), std::string::npos);
        EXPECT_EQ(readMimePart(mails[1], "Content-Type: text/html"), "<p>original</p>");
    }

    TEST(BreezeCodeTest, ReportsSmtpFailureAndRunsTheNextTask)
    {
        SmtpReceiver receiver(false, true);
        MailClient client(receiver.settings(), MailQueueOptions{});
        EXPECT_FALSE(client.sendAsync(kRecipient, "123456").get());
        EXPECT_TRUE(client.sendMailAsync({kRecipient, "Report", "<p>ok</p>", {}}).get());
        EXPECT_EQ(receiver.messages().size(), 2u);
    }

    TEST(BreezeCodeTest, EnforcesTaskTimeoutAndPayloadLimit)
    {
        SmtpReceiver receiver(true);
        MailClient synchronous(receiver.settings());
        EXPECT_FALSE(synchronous.sendAsync(kRecipient, "123456").get());

        MailQueueOptions options;
        options.max_payload_bytes = 128;
        options.task_timeout = 800ms;
        MailClient client(receiver.settings(), options);
        auto oversized = client.sendMailAsync({kRecipient, "Report", std::string(129, 'x'), {}});
        ASSERT_EQ(oversized.wait_for(0ms), std::future_status::ready);
        EXPECT_FALSE(oversized.get());

        auto delayed = client.sendAsync(kRecipient, "123456");
        ASSERT_TRUE(receiver.waitFor(1));
        const auto result = delayed.wait_for(3s);
        receiver.releaseReply();
        ASSERT_EQ(result, std::future_status::ready);
        EXPECT_FALSE(delayed.get());
    }

    TEST(BreezeCodeTest, CompletesConcurrentSubmissionsWhenLastClientIsDestroyed)
    {
        SmtpReceiver receiver;
        std::vector<std::future<bool>> results(8);
        {
            MailClient client(receiver.settings(), MailQueueOptions{});
            std::vector<std::thread> producers;
            for (std::size_t i = 0; i < results.size(); ++i)
            {
                producers.emplace_back([&, i] {
                    results[i] = client.sendMailAsync({kRecipient, "Report " + std::to_string(i), "<p>ok</p>", {}});
                });
            }
            for (auto &producer : producers)
                producer.join();
            // 客户端销毁时完成已接收的任务，调用方仍可读取结果。
        }
        for (auto &result : results)
        {
            ASSERT_EQ(result.wait_for(0ms), std::future_status::ready);
            EXPECT_TRUE(result.get());
        }
        EXPECT_EQ(receiver.messages().size(), results.size());
    }
}
