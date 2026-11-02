#include <amqpcpp.h>
#include <amqpcpp/libev.h>
#include <iostream>
#include <string>

int main(int argc, char *argv[])
{
    // 默认使用 Docker 网络中的 RabbitMQ，也可传入其他连接地址。
    const std::string url = "amqp://admin:123456@rabbitmq-service:5672/";
    const std::string delay_exchange = "cpp-service-kit.dlx.delay.exchange";
    const std::string delay_queue = "cpp-service-kit.dlx.delay.queue";
    const std::string delay_key = "delay";
    const std::string dead_exchange = "cpp-service-kit.dlx.exchange";
    const std::string dead_queue = "cpp-service-kit.dlx.queue";
    const std::string dead_key = "dead";
    const std::string message = "Hello dead letter!";
    bool sent = false;

    // 创建事件循环、连接和通道。
    auto *loop = EV_DEFAULT;
    AMQP::LibEvHandler handler(loop);
    AMQP::TcpConnection connection(&handler, AMQP::Address(url));
    AMQP::TcpChannel channel(&connection);
    channel.onError([&](const char *error)
                    {
                        sent = false;
                        std::cerr << "AMQP 操作失败: " << error << std::endl;
                        connection.close(); });

    // 1. 创建死信交换机和死信队列，按死信路由键绑定。
    channel.declareExchange(dead_exchange, AMQP::direct)
        .onSuccess([&]()
                   { channel.declareQueue(dead_queue)
                         .onSuccess([&]()
                                    { channel.bindQueue(dead_exchange, dead_queue, dead_key)
                                          .onSuccess([&]()
                                                     {
                                                        // 2. 创建普通交换机，接收生产者发送的消息。
                                                        channel.declareExchange(delay_exchange, AMQP::direct)
                                                            .onSuccess([&]()
                                                            {
                                                                // 消息在普通队列中等待 5 秒后，转发到死信交换机。
                                                                AMQP::Table args;
                                                                args["x-message-ttl"] = 5000;// 消息过期时间 5 秒
                                                                args["x-dead-letter-exchange"] = dead_exchange;// 指定过期消息转发的死信交换机
                                                                args["x-dead-letter-routing-key"] = dead_key;// 指定过期消息转发的死信路由键
                                                                channel.declareQueue(delay_queue, args)
                                                                    .onSuccess([&]()
                                                                    {
                                                                        channel.bindQueue(delay_exchange, delay_queue, delay_key)
                                                                            .onSuccess([&]()
                                                                            {
                                                                                // 3. 消息先进入普通队列，再等待过期转入死信队列。
                                                                                sent = channel.publish(delay_exchange, delay_key, message);
                                                                                if (sent)
                                                                                    std::cout << "已提交消息，等待约 5 秒进入死信队列: " << message << std::endl;
                                                                                else
                                                                                    std::cerr << "提交消息失败" << std::endl;
                                                                                connection.close();
                                                                            });
                                                                    });
                                                            }); }); }); });

    // 处理网络事件，连接关闭后结束事件循环。
    ev_run(loop, 0);
    return sent ? 0 : 1;
}
