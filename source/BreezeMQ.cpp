#include "BreezeMQ.hpp"
#include "BreezeLog.hpp"

#include <cstdlib>

// 生成死信交换机名称。
std::string declare_settings::dlx_exchange() const
{
    return DLX_PREFIX + exchange;
}

// 生成死信队列名称。
std::string declare_settings::dlx_queue() const
{
    return DLX_PREFIX + queue;
}

// 生成死信队列使用的绑定键。
std::string declare_settings::dlx_binding_key() const
{
    return DLX_PREFIX + binding_key;
}

// 将字符串转换为 AMQP-CPP 交换机类型。
AMQP::ExchangeType exchange_type(const std::string &type)
{
    if (type == DIRECT)
        return AMQP::ExchangeType::direct;
    else if (type == FANOUT)
        return AMQP::ExchangeType::fanout;
    else if (type == TOPIC)
        return AMQP::ExchangeType::topic;
    else if (type == HEADERS)
        return AMQP::ExchangeType::headers;
    else if (type == DELAYED)
        return AMQP::ExchangeType::direct;

    ERR("交换机类型错误: {}", type);
    std::abort();
}

// 创建连接、通道和后台事件循环线程
MQClient::MQClient(const std::string &url)
    : _ev_loop(ev_loop_new(EVFLAG_AUTO)),
      _handler(_ev_loop.get()),
      _connection(&_handler, AMQP::Address(url)),
      _channel(&_connection)
{
    // 启动事件循环前注册异步监听器，供其他线程唤醒事件循环。
    ev_async_init(&_ev_async, MQClient::callback);
    _ev_async.data = this;
    ev_async_start(_ev_loop.get(), &_ev_async);
    start();
}

// 发送退出通知，等待后台线程结束后清理监听器。
MQClient::~MQClient()
{
    if (_async_thread.joinable())
    {
        ev_async_send(_ev_loop.get(), &_ev_async);
        _async_thread.join();
    }
    ev_async_stop(_ev_loop.get(), &_ev_async);
}

// 在事件循环线程中执行待处理操作，没有操作时结束事件循环。
void MQClient::callback(struct ev_loop *loop, ev_async *watcher, int32_t)
{
    auto *client = static_cast<MQClient *>(watcher->data);
    std::function<void()> task;
    {
        std::lock_guard<std::mutex> lock(client->_mtx);
        task = std::move(client->_task);
    }
    if (task)
        task();
    else
        ev_break(loop, EVBREAK_ALL); // 没有待处理操作时退出事件循环
}

// 提交操作，等待事件循环线程处理并返回结果。
void MQClient::_execute(std::function<void()> task)
{
    std::unique_lock<std::mutex> declared_lock(_declared_mtx);
    std::unique_lock<std::mutex> lock(_mtx);
    _done = false;
    _task = std::move(task);
    ev_async_send(_ev_loop.get(), &_ev_async);
    _cv.wait(lock, [this]
             { return _done; });
}

// 设置完成状态，通知调用线程继续执行。
void MQClient::_finish()
{
    std::lock_guard<std::mutex> lock(_mtx);
    _done = true;
    _cv.notify_all();
}

// 声明交换机和队列，完成绑定后通知等待线程。
void MQClient::_declared(const declare_settings &settings, AMQP::Table &args, bool is_dlx)
{
    // 根据拓扑类型选择交换机、队列和绑定键。
    std::string exchange;
    std::string queue;
    std::string binding_key;
    // 死信拓扑使用死信交换机、死信队列和死信绑定键
    if (is_dlx)
    {
        exchange = settings.dlx_exchange();
        queue = settings.dlx_queue();
        binding_key = settings.dlx_binding_key();
    }
    else
    {
        exchange = settings.exchange;
        queue = settings.queue;
        binding_key = settings.binding_key;
    }
    AMQP::ExchangeType type = exchange_type(settings.exchange_type);

    // 声明交换机、声明队列，再绑定两者。
    _execute([=]
             { _channel.declareExchange(exchange, type)
                   .onSuccess([=]
                              { _channel.declareQueue(queue, args)
                                    .onSuccess([=](const std::string &, uint32_t, uint32_t)
                                               { _channel.bindQueue(exchange, queue, binding_key)
                                                     .onSuccess([=]
                                                                { _finish(); })
                                                     .onError([=](const char *message)
                                                              {
                                ERR("绑定队列失败: {} - {} - {}", exchange, queue, message);
                                std::abort(); }); })
                                    .onError([=](const char *message)
                                             {
                        ERR("声明队列失败: {} - {}", queue, message);
                        std::abort(); }); })
                   .onError([=](const char *message)
                            {
                ERR("声明交换机失败: {} - {}", exchange, message);
                std::abort(); }); });
}

// 延迟模式先声明死信拓扑，再声明配置了过期时间的普通队列。
void MQClient::declare(const declare_settings &settings)
{
    AMQP::Table args;
    if (settings.exchange_type == DELAYED) // 延迟模式声明死信拓扑
    {
        _declared(settings, args, true);
        args[DEAD_LETTER_EXCHANGE] = settings.dlx_exchange();
        args[DEAD_LETTER_BINDING_KEY] = settings.dlx_binding_key();
        args[MESSAGE_TTL] = settings.delayed_ttl;
    }
    _declared(settings, args, false);
}

// 向指定交换机和路由键发布消息。
bool MQClient::publish(const std::string &exchange, const std::string &routing_key, const std::string &body)
{
    // 消息回调已在事件循环线程中，可以直接发布。
    if (std::this_thread::get_id() == _async_thread.get_id())
        return _channel.publish(exchange, routing_key, body);

    bool result = false;
    _execute([this, exchange, routing_key, body, &result]
             {
        result = _channel.publish(exchange, routing_key, body);
        _finish(); });
    return result;
}

// 订阅队列，并在业务回调处理完成后确认消息。
void MQClient::consume(const std::string &queue, const MessageCallback &callback)
{
    _execute([this, queue, callback]
             { _channel.consume(queue)
                   .onMessage([this, callback](const AMQP::Message &message, uint64_t delivery_tag, bool)
                              {
                callback(message.body(), message.bodySize());
                _channel.ack(delivery_tag); })
                   .onError([](const char *message)
                            {
                ERR("订阅队列消息失败: {}", message);
                std::abort(); })
                   .onSuccess([this, queue]
                              {
                DBG("订阅队列消息成功: {}", queue);
                _finish(); }); });
}

// 启动事件循环线程；构造函数调用后可继续复用。
void MQClient::start()
{
    if (!_async_thread.joinable())
        _async_thread = std::thread([this] { ev_run(_ev_loop.get()); });
}

// 等待事件循环线程退出，保持消费者运行。
void MQClient::wait()
{
    _async_thread.join();
}

// 创建发布者时声明对应的交换机、队列和绑定。
Publisher::Publisher(const MQClient::ptr &mq_client, const declare_settings &settings)
    : _mq_client(mq_client), _settings(settings)
{
    _mq_client->declare(_settings);
}

// 普通消息和延迟消息都先发布到普通交换机。
bool Publisher::publish(const std::string &body)
{
    return _mq_client->publish(_settings.exchange, _settings.binding_key, body);
}

// 创建订阅者时声明对应的交换机、队列和绑定。
Subscriber::Subscriber(const MQClient::ptr &mq_client, const declare_settings &settings)
    : _mq_client(mq_client), _settings(settings)
{
    _mq_client->declare(_settings);
}

// 延迟模式订阅死信队列，普通模式订阅普通队列。
void Subscriber::consume(MessageCallback &&cb)
{
    _callback = std::move(cb);
    if (_settings.exchange_type == DELAYED)
        _mq_client->consume(_settings.dlx_queue(), _callback);
    else
        _mq_client->consume(_settings.queue, _callback);
}
