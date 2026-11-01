#include <etcd/Client.hpp>
#include <etcd/KeepAlive.hpp>
#include <etcd/Watcher.hpp>
#include <etcd/Response.hpp>
#include <etcd/Value.hpp>

void callback(const etcd::Response &resp)
{
    if (resp.is_ok() == false)
    {
        std::cout << "监控出错: " << resp.error_message() << std::endl;
        return;
    }
    const auto &events = resp.events(); // 可能包含多个事件
    for (auto &e : events)
    {
        if (e.event_type() == etcd::Event::EventType::PUT)
        {
            std::cout << e.kv().key() << "数据改变:" << e.prev_kv().as_string() << "->" << e.kv().as_string() << std::endl;
        }
        else if (e.event_type() == etcd::Event::EventType::DELETE_)
        {
            std::cout << e.prev_kv().key() << "数据删除!" << std::endl;
        }
        else
        {
            std::cout << "无效类型\n";
        }
    }
}

void get(std::shared_ptr<etcd::Watcher> &watcher, const std::string &url, const std::string &key)
{
    etcd::Client client(url);
    // 浏览目录下的数据 ls，读取一组前缀相同的键值对数据
    auto resp = client.ls(key).get();
    if (resp.is_ok() == true)
    {
        auto values = resp.values(); // 获取结果数据，结果可能包含多个键值对
        for (auto &v : values)
        {
            std::cout << v.key() << " = " << v.as_string() << std::endl;
        }
    }
    // 数据监控
    watcher.reset(new etcd::Watcher(url, "/", callback, true)); // true表示递归监控目录下所有数据
    watcher->Wait([&](bool cond)
                  {
                    // 监听任务结束后执行
                    if (cond == true) {
                        //当前任务是被主动取消的
                        return;
                    }
                    // 异常结束后重新发起监听
                    get(watcher, url, key); });
}

int main()
{
    const std::string url = "http://etcd-service:2379";
    const std::string key = "/name";
    std::shared_ptr<etcd::Watcher> watcher;
    get(watcher, url, key);
    std::cout << "回车退出...\n";
    getchar();
    return 0;
}
