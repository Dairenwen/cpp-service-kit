#pragma once

#include <amqpcpp.h>
#include <amqpcpp/libev.h>
#include <ev.h>

#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <utility>

// 使用字符串描述交换机类型，delayed 表示通过死信队列实现延迟。
const std::string DIRECT = "direct";
const std::string FANOUT = "fanout";
const std::string TOPIC = "topic";
const std::string HEADERS = "headers";
const std::string DELAYED = "delayed";

// 死信拓扑的名称前缀，以及延迟队列使用的参数名。
const std::string DLX_PREFIX = "dlx_";
const std::string DEAD_LETTER_EXCHANGE = "x-dead-letter-exchange";
const std::string DEAD_LETTER_BINDING_KEY = "x-dead-letter-routing-key";
const std::string MESSAGE_TTL = "x-message-ttl";

// 保存交换机、队列、绑定关系和延迟时间。
struct declare_settings
{
    std::string exchange;      // 交换机名称
    std::string exchange_type; // 交换机类型
    std::string queue;         // 队列名称
    std::string binding_key;   // 队列绑定使用的路由键
    size_t delayed_ttl = 0;    // 消息在延迟队列中的存活时间，单位为毫秒

    // 根据当前配置生成对应的死信拓扑名称。
    std::string dlx_exchange() const;
    std::string dlx_queue() const;
    std::string dlx_binding_key() const;
};

// 将字符串交换机类型转换为 AMQP-CPP 的类型。
extern AMQP::ExchangeType exchange_type(const std::string &type);

// 处理消息内容和长度的回调函数；回调运行在事件循环线程。
// 回调中可以发布消息，声明和订阅操作由其他线程调用。
using MessageCallback = std::function<void(const char *, size_t)>;

// 封装连接、通道、消息声明、发布和订阅。
class MQClient
{
public:
    using ptr = std::shared_ptr<MQClient>;

    // 创建连接和通道，并启动后台事件循环。
    MQClient(const std::string &url);
    // 发送异步通知，等待后台事件循环结束。
    ~MQClient();

    // 声明交换机、队列及其绑定，延迟模式同时声明死信拓扑。
    void declare(const declare_settings &settings);
    // 向交换机发布消息。
    bool publish(const std::string &exchange, const std::string &routing_key, const std::string &body);
    // 订阅队列并设置消息回调。
    void consume(const std::string &queue, const MessageCallback &callback);
    // 启动事件循环；构造时已经启动，重复调用会复用现有线程。
    void start();
    // 等待后台事件循环线程退出。
    void wait();

private:
    // 事件循环在线程结束后释放。
    struct LoopDeleter
    {
        void operator()(struct ev_loop *loop) const { ev_loop_destroy(loop); }
    };

    // 接收异步通知，在事件循环线程中执行待处理操作。
    static void callback(struct ev_loop *loop, ev_async *watcher, int32_t revents);
    // 提交操作并等待它完成。
    void _execute(std::function<void()> task);
    // 保存完成状态并唤醒等待线程。
    void _finish();
    // 声明一组交换机、队列和绑定，并等待声明完成。
    void _declared(const declare_settings &settings, AMQP::Table &args, bool is_dlx = false);

private:
    std::mutex _declared_mtx;                              // 保证各项操作依次进行
    std::mutex _mtx;                                       // 保护待处理操作和完成状态
    std::condition_variable _cv;                           // 通知等待线程操作已完成
    std::function<void()> _task;                           // 等待事件循环线程执行的操作
    bool _done = false;                                    // 当前操作是否完成
    std::unique_ptr<struct ev_loop, LoopDeleter> _ev_loop; // 客户端独立的事件循环，销毁时调用 ev_loop_destroy()释放
    struct ev_async _ev_async;                             // 唤醒事件循环执行操作或退出
    AMQP::LibEvHandler _handler;                           // 将网络事件接入 libev
    AMQP::TcpConnection _connection;                       // RabbitMQ 连接
    AMQP::TcpChannel _channel;                             // 执行消息操作的通道
    std::thread _async_thread;                             // 运行事件循环的后台线程
};

// 使用指定配置发布消息。
class Publisher
{
public:
    using ptr = std::shared_ptr<Publisher>;
    // 保存配置，并声明交换机和队列。
    Publisher(const MQClient::ptr &mq_client, const declare_settings &settings);
    // 向配置的交换机发布消息。
    bool publish(const std::string &body);

private:
    MQClient::ptr _mq_client;   // 消息队列客户端
    declare_settings _settings; // 交换机和队列配置
};

// 使用指定配置订阅消息。
class Subscriber
{
public:
    using ptr = std::shared_ptr<Subscriber>;
    // 保存配置，并声明交换机和队列。
    Subscriber(const MQClient::ptr &mq_client, const declare_settings &settings);
    // 设置消息处理回调并订阅对应队列。
    void consume(MessageCallback &&cb);

private:
    MQClient::ptr _mq_client;   // 消息队列客户端
    declare_settings _settings; // 交换机和队列配置
    MessageCallback _callback;  // 收到消息后的处理回调
};

// 采用工厂模式创建 MQClient、Publisher 和 Subscriber 对象。
class MQFactory
{
public:
    template <typename R, typename... Args>
    static std::shared_ptr<R> create(Args &&...args)
    {
        return std::make_shared<R>(std::forward<Args>(args)...);
    }
};
