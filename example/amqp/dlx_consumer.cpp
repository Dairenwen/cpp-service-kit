#include <amqpcpp.h>
#include <amqpcpp/libev.h>
#include <cstdint>
#include <iostream>
#include <string>

int main(int argc, char *argv[])
{
    // 使用与生产者相同的死信交换机、队列和路由键
    const std::string url = "amqp://admin:123456@rabbitmq-service:5672/";
    const std::string dead_exchange = "cpp-service-kit.dlx.exchange";
    const std::string dead_queue = "cpp-service-kit.dlx.queue";
    const std::string dead_key = "dead";
    bool failed = false;

    // 创建事件循环、连接和通道
    auto *loop = EV_DEFAULT;
    AMQP::LibEvHandler handler(loop);
    AMQP::TcpConnection connection(&handler, AMQP::Address(url));
    AMQP::TcpChannel channel(&connection);
    channel.onError([&](const char *error)
                    {
                        failed = true;
                        std::cerr << "AMQP 操作失败: " << error << std::endl;
                        connection.close(); });

    // 声明并绑定死信队列，生产者和消费者可按任意顺序启动。
    channel.declareExchange(dead_exchange, AMQP::direct)
        .onSuccess([&]()
                   { channel.declareQueue(dead_queue)
                         .onSuccess([&]()
                                    { channel.bindQueue(dead_exchange, dead_queue, dead_key)
                                          .onSuccess([&]()
                                                     {
                            // 消费过期转入的消息，并向 RabbitMQ 确认处理完成。
                            channel.consume(dead_queue)
                                .onSuccess([&]()
                                {
                                    std::cout << "开始消费死信队列，按 Ctrl+C 退出。" << std::endl;
                                })
                                .onReceived([&](const AMQP::Message &message, uint64_t delivery_tag, bool)
                                {
                                    std::cout << "收到死信: "
                                              << std::string(message.body(), message.bodySize()) << std::endl;
                                    channel.ack(delivery_tag);
                                }); }); }); });

    // 持续处理网络事件，等待死信消息。
    ev_run(loop, 0);
    return failed ? 1 : 0;
}
