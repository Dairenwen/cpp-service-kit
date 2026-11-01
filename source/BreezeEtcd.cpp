#include "BreezeEtcd.hpp"
#include "BreezeLog.hpp"
#include "BreezeUtil.hpp"
#include <chrono>
#include <exception>
#include <sstream>
#include <thread>
#include <utility>
#include <vector>

// 等待 etcd 请求成功，再执行注册或监听。
void wait_for_connection(etcd::Client &client)
{
    while (!client.head().get().is_ok())
    {
        WRN("连接etcd服务器失败...");
        std::this_thread::sleep_for(std::chrono::seconds(1)); // 等待 1 秒后重试
    }
}

// 生成当前实例标识，保存服务注册所需的信息。
SvcProvider::SvcProvider(const std::string &reg_center_addr, const std::string &svc_name, const std::string &svc_addr)
    : _reg_center_addr(reg_center_addr),
      _instance_id(BreezeUtil::Random::code()),
      _svc_name(svc_name),
      _svc_addr(svc_addr)
{
}

// 使用服务名称和实例标识生成注册键。
std::string SvcProvider::make_key()
{
    std::stringstream ss;
    ss << "/" << _svc_name << "/" << _instance_id;
    return ss.str();
}

// 创建租约并注册节点，保活失败时尝试重新注册。
bool SvcProvider::registry()
{
    // 1. 创建客户端并等待 etcd 响应。
    etcd::Client client(_reg_center_addr);
    wait_for_connection(client);

    // 2. 创建 3 秒租约，取得写入数据时需要的租约 ID。
    auto lease_resp = client.leasegrant(3).get();
    if (!lease_resp.is_ok())
    {
        ERR("创建租约失败: {}", lease_resp.error_message());
        return false;
    }
    auto lease_id = lease_resp.value().lease();

    // 3. 写入服务节点地址，并将键关联到租约。
    auto resp = client.put(make_key(), _svc_addr, lease_id).get();
    if (!resp.is_ok())
    {
        ERR("添加数据失败: {}", resp.error_message());
        return false;
    }

    // 4. 保活对象持续续租，续租失败时尝试重新注册。
    auto handler = [this](const std::exception_ptr &)
    {
        this->registry();
    };
    _keepalive.reset(new etcd::KeepAlive(_reg_center_addr, handler, 3, lease_id)); // 续租间隔为 3 秒
    return true;
}

// 保存外部提供的服务上线和下线处理函数。
SvcWatcher::SvcWatcher(const std::string &reg_center_addr, SvcWatcher::ModCallbck &&online_callback, SvcWatcher::ModCallbck &&offline_callback)
    : _reg_center_addr(reg_center_addr),
      _online_callback(std::move(online_callback)), // 移动构造函数，避免拷贝
      _offline_callback(std::move(offline_callback))
{
}

// 根据注册键 /服务名称/节点标识 提取服务名称。
std::string SvcWatcher::parse_key(const std::string &key)
{
    std::vector<std::string> parts;
    BreezeUtil::STR::split(key, "/", parts);
    return parts.size() == 2 ? parts[0] : "";
}

// 将节点写入和删除事件交给对应的外部回调。
void SvcWatcher::callback(const etcd::Response &resp)
{
    if (!resp.is_ok())
    {
        ERR("监控出错: {}", resp.error_message());
        return;
    }
    const auto &events = resp.events();
    for (const auto &e : events)
    {
        if (e.event_type() == etcd::Event::EventType::PUT)
        {
            std::string svc_name = parse_key(e.kv().key());
            if (svc_name.empty())
                continue;
            std::string svc_addr = e.kv().as_string();
            DBG(" {} 服务上线节点: {}", svc_name, svc_addr);
            if (_online_callback)
                _online_callback(svc_name, svc_addr);
        }
        else if (e.event_type() == etcd::Event::EventType::DELETE_)
        {
            std::string svc_name = parse_key(e.kv().key());
            if (svc_name.empty())
                continue;
            std::string svc_addr = e.prev_kv().as_string();
            DBG(" {} 服务下线节点: {}", svc_name, svc_addr);
            if (_offline_callback)
                _offline_callback(svc_name, svc_addr);
        }
        else
        {
            WRN("无效通知类型");
        }
    }
}

// 读取当前服务节点，并监听以 / 开头的键的变化。
bool SvcWatcher::watch()
{
    // 1. 创建客户端并等待 etcd 响应。
    etcd::Client client(_reg_center_addr);
    wait_for_connection(client);

    // 2. 遍历当前节点，逐个调用服务上线回调。
    auto resp = client.ls("/").get();
    if (!resp.is_ok())
    {
        ERR("读取服务节点失败: {}", resp.error_message());
    }
    else
    {
        for (const auto &v : resp.values())
        {
            std::string svc_name = parse_key(v.key());
            if (svc_name.empty())
                continue;
            std::string svc_addr = v.as_string();
            DBG("已有 {} 节点: {}", svc_name, svc_addr);
            if (_online_callback)
                _online_callback(svc_name, svc_addr);
        }
    }

    // 3. 读取成功时从下一版本监听，读取失败时从当前开始监听，绑定用户回调处理事件
    auto cb = std::bind(&SvcWatcher::callback, this, std::placeholders::_1);
    _watcher.reset(new etcd::Watcher(_reg_center_addr, "/", cb, true)); // 从当前版本开始监听，true表示递归监控目录下所有数据
    _watcher->Wait([this](bool cond)
                   {
                       if (cond)
                           return;
                       // 如果监听异常结束，重新发起监听
                       this->watch(); });
    return true;
}
