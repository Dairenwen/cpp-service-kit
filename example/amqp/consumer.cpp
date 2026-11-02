
#include <amqpcpp.h>
#include <amqpcpp/libev.h>
#include <cstdint>
#include <iostream>
#include <string>

int main(int argc, char *argv[])
{
    // 1. 默认连接 Docker 网络中的 RabbitMQ，也可传入其他地址。
    const std::string url = "amqp://admin:123456@rabbitmq-service:5672/";
    const std::string exchange = "cpp-service-kit.example.exchange";
    const std::string queue = "cpp-service-kit.example.queue";
    const std::string routing_key = "hello";
    bool failed = false;

    // 2. 创建事件循环、网络处理器、连接和通道。
    auto *loop = EV_DEFAULT;
    AMQP::LibEvHandler handler(loop);
    AMQP::TcpConnection connection(&handler, AMQP::Address(url));
    AMQP::TcpChannel channel(&connection);
    channel.onError([&](const char *error)
                    {
        failed = true;
        std::cerr << "AMQP 操作失败: " << error << std::endl;
        connection.close(); });

    // 3. 声明与生产者相同的交换机、队列和绑定。
    channel.declareExchange(exchange, AMQP::direct)
        .onSuccess([&]()
                   {
            std::cout << "声明交换机成功: " << exchange << std::endl;
            channel.declareQueue(queue)
                .onSuccess([&]()
                {
                    std::cout << "声明队列成功: " << queue << std::endl;
                    channel.bindQueue(exchange, queue, routing_key)
                        .onSuccess([&]()
                        {
                            std::cout << "绑定成功，路由键: " << routing_key << std::endl;

                            // 4. 绑定成功后订阅队列，收到消息时输出并确认。
                            channel.consume(queue)
                                .onSuccess([&]()
                                {
                                    std::cout << "开始消费消息，按 Ctrl+C 退出。" << std::endl;
                                })
                                .onReceived([&](const AMQP::Message &message, uint64_t delivery_tag, bool)
                                {
                                    std::cout << "收到消息: " << std::string(message.body(), message.bodySize()) << std::endl;
                                    channel.ack(delivery_tag); // 告知 RabbitMQ 当前消息已处理。
                                });
                        });
                }); });

    // 5. 持续处理网络事件，等待消息到来。
    ev_run(loop, 0);
    return failed ? 1 : 0;
}
