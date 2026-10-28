#pragma once
#include <brpc/channel.h>
#include <brpc/server.h>
#include <google/protobuf/stubs/callback.h>

#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <thread>

// 管理一个服务的多个 brpc 通道，并以轮询方式选择节点。
class Channels
{
public:
    using ptr = std::shared_ptr<Channels>;
    using ChannelPtr = std::shared_ptr<brpc::Channel>;

    Channels();

    void insert(const std::string &addr);    // 新增节点
    void remove(const std::string &addr);    // 删除节点
    ChannelPtr select();                     // 轮询选择一个节点
    std::optional<std::string> selectAddr(); // 轮询选择一个节点的地址

private:
    std::mutex _mtx;
    uint32_t _idx;
    std::string _svc_name;
    std::unordered_map<std::string, ChannelPtr> _channels; // 地址 -> 通道
    std::vector<std::string> _channel_list;                // 地址列表，便于轮询
};

// 管理多个服务名称及其节点集合。
class SvcChannels
{
public:
    using ptr = std::shared_ptr<SvcChannels>;

    void setWatch(const std::string &svc_name);                          // 创建服务节点集合
    void addNode(const std::string &svc_name, const std::string &addr);  // 新增节点
    void delNode(const std::string &svc_name, const std::string &addr);  // 删除节点
    Channels::ChannelPtr getNode(const std::string &svc_name);           // 轮询选择一个节点
    std::optional<std::string> getNodeAddr(const std::string &svc_name); // 轮询选择一个节点的地址

private:
    Channels::ptr _channels(const std::string &svc_name);

    std::mutex _mtx;
    std::unordered_map<std::string, Channels::ptr> _maps;
};

// 将普通 C++ 回调封装成 protobuf Closure，供 brpc 异步 RPC 使用。
class ClosureFactor
{
public:
    using callback_t = std::function<void()>;

    static google::protobuf::Closure *create(callback_t cb);

private:
    struct Object // 用来在异步线程中保存回调函数，避免回调函数被提前释放
    {
        using ptr = std::shared_ptr<Object>;
        callback_t callback;
    };
    {
        using ptr = std::shared_ptr<Object>;
        callback_t callback;
    };

    class CallbackClosure;

    static void asyncCallback(const Object::ptr &obj);
};

// 创建并启动一个 brpc 服务端。
class RpcServerFactory
{
public:
    // svc 由创建出的 Server 接管；启动失败时返回空指针,svc必须在堆上创建
    static std::shared_ptr<brpc::Server> create(int port, google::protobuf::Service *svc);
};
