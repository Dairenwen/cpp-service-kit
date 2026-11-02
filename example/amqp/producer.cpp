#include <amqpcpp.h>
#include <amqpcpp/libev.h>
#include <iostream>
#include <string>

int main()
{
    // 1. 使用 Docker 网络连接 RabbitMQ。
    const std::string url = "amqp://admin:123456@rabbitmq-service:5672/"; // 默认连接到 Docker Compose 中的 rabbitmq-service 服务
    const std::string exchange = "cpp-service-kit.example.exchange";      // 交换机名称
    const std::string queue = "cpp-service-kit.example.queue";            // 队列名称
    const std::string routing_key = "hello";                              // 路由键
    const std::string message = "Hello RabbitMQ!";                        // 消息内容
    bool sent = false;

    // 2. 创建 libev 事件循环和网络处理器。
    auto *loop = EV_DEFAULT; // 网络通信循环句柄
    AMQP::LibEvHandler handler(loop);

    // 3. 创建连接和通道，后续操作通过通道发送给 RabbitMQ。
    AMQP::TcpConnection connection(&handler, AMQP::Address(url));
    AMQP::TcpChannel channel(&connection); // 用一个连接可以创建多个通道，connection只负责建立tcp连接，channel负责amqp协议的操作
    // 处理通道错误
    channel.onError([&](const char *error)
                    {
        sent = false;
        std::cerr << "AMQP 操作失败: " << error << std::endl;
        connection.close(); });

    // 4. 交换机声明成功后，再声明队列。
    channel.declareExchange(exchange, AMQP::direct)
        .onSuccess([&]()
                   {
            std::cout << "声明交换机成功: " << exchange << std::endl;
            channel.declareQueue(queue)
                .onSuccess([&]()
                {
                    std::cout << "声明队列成功: " << queue << std::endl;

                    // 5. 队列声明成功后，通过路由键绑定交换机和队列。
                    channel.bindQueue(exchange, queue, routing_key)//根据路由键绑定交换机和队列
                        .onSuccess([&]()
                        {
                            std::cout << "绑定成功，路由键: " << routing_key << std::endl;

                            // 6. 绑定成功后提交消息，然后等待连接完成关闭。
                            sent = channel.publish(exchange, routing_key, message);
                            if (sent)
                                std::cout << "已提交消息: " << message << std::endl;
                            else
                                std::cerr << "提交消息失败" << std::endl;
                            connection.close();
                        });
                }); });

    // 7. 处理网络事件，退出时依次释放局部对象。
    ev_run(loop, 0);
    return sent ? 0 : 1;
}
