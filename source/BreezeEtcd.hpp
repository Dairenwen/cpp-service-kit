#pragma once
#include <etcd/Client.hpp>
#include <etcd/KeepAlive.hpp>
#include <etcd/Watcher.hpp>
#include <functional>
#include <memory>
#include <string>

// 等待 etcd 响应，连接失败时每秒重试。
void wait_for_connection(etcd::Client &client);

// 将当前服务节点注册到 etcd，并为租约续期
class SvcProvider
{
public:
    using ptr = std::shared_ptr<SvcProvider>;

    SvcProvider(const std::string &reg_center_addr, const std::string &svc_name, const std::string &svc_addr);

    // 创建租约，写入 /服务名称/节点标识 和节点地址。
    bool registry();

private:
    // 按 /服务名称/节点标识 组织注册键。
    std::string make_key();

    std::string _reg_center_addr;                // 注册中心地址
    std::string _instance_id;                    // 当前节点标识
    std::string _svc_name;                       // 服务名称，例如 user
    std::string _svc_addr;                       // 当前节点的访问地址
    std::shared_ptr<etcd::KeepAlive> _keepalive; // 租约保活对象
};

// 读取和监听 etcd 中的服务节点，并将变化交给外部处理
class SvcWatcher
{
public:
    using ptr = std::shared_ptr<SvcWatcher>;
    using ModCallbck = std::function<void(std::string, std::string)>; // 两个参数分别是服务名称和节点地址

    SvcWatcher(const std::string &reg_center_addr, ModCallbck &&online_callback, ModCallbck &&offline_callback);
    // 读取已有节点，并监听以 / 开头的键
    bool watch();

private:
    // 将 etcd 事件转换为服务上线或下线回调
    void callback(const etcd::Response &resp);
    // 从 /服务名称/节点标识 中提取服务名称
    std::string parse_key(const std::string &key);

    std::string _reg_center_addr;            // 注册中心地址
    ModCallbck _online_callback;             // 服务上线回调
    ModCallbck _offline_callback;            // 服务下线回调
    std::shared_ptr<etcd::Watcher> _watcher; // 服务变化监听对象
};
