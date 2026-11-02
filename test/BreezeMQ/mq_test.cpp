#include "BreezeMQ.hpp"
#include <gtest/gtest.h>
#include <chrono>
#include <cstdlib>
#include <future>
#include <string>

// 测试普通消息的发布和订阅。
TEST(BreezeMQTest, DirectMessage)
{
    // 使用独立名称，避免读取其他演示或上次测试的消息。
    const auto id = std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    declare_settings settings{"breeze_test_direct_exchange_" + id, DIRECT, "breeze_test_direct_queue_" + id, "hello", 0};

    // 消息回调通过 promise 将收到的内容交给测试线程。
    std::promise<std::string> received;
    auto result = received.get_future();
    // 发布者和订阅者分别建立连接，模拟两个服务通信。
    auto publisher_client = MQFactory::create<MQClient>("amqp://admin:123456@rabbitmq-service:5672/");
    auto subscriber_client = MQFactory::create<MQClient>("amqp://admin:123456@rabbitmq-service:5672/");
    auto publisher = MQFactory::create<Publisher>(publisher_client, settings);
    auto subscriber = MQFactory::create<Subscriber>(subscriber_client, settings);
    subscriber->consume([&received](const char *body, size_t size)
                        { received.set_value(std::string(body, size)); });

    ASSERT_TRUE(publisher->publish("Hello RabbitMQ!"));
    ASSERT_EQ(result.wait_for(std::chrono::seconds(5)), std::future_status::ready);
    EXPECT_EQ(result.get(), "Hello RabbitMQ!");
}

// 测试消息过期后进入死信队列，再由订阅者接收。
TEST(BreezeMQTest, DelayedMessage)
{
    const auto id = std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    // 消息在普通队列等待 1000 毫秒后转入死信队列。
    declare_settings settings{"breeze_test_delay_exchange_" + id, DELAYED, "breeze_test_delay_queue_" + id, "hello", 1000};

    std::promise<std::string> received;
    auto result = received.get_future();
    auto client = MQFactory::create<MQClient>("amqp://admin:123456@rabbitmq-service:5672/");
    auto publisher = MQFactory::create<Publisher>(client, settings);
    auto subscriber = MQFactory::create<Subscriber>(client, settings);
    subscriber->consume([&received](const char *body, size_t size)
                        { received.set_value(std::string(body, size)); });

    ASSERT_TRUE(publisher->publish("Hello delayed message!"));
    // 先确认消息仍在等待，再确认延迟结束后收到内容。
    EXPECT_EQ(result.wait_for(std::chrono::milliseconds(500)), std::future_status::timeout);
    ASSERT_EQ(result.wait_for(std::chrono::seconds(5)), std::future_status::ready);
    EXPECT_EQ(result.get(), "Hello delayed message!");
}
