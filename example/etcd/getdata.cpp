#include <etcd/Client.hpp>
#include <etcd/Watcher.hpp>
#include <iostream>
#include <string>

// 收到键的变化事件后，输出修改前后的值或删除信息。
void callback(const etcd::Response &resp)
{
    if (!resp.is_ok())
    {
        std::cout << "监控出错: " << resp.error_message() << std::endl;
        return;
    }

    for (const auto &event : resp.events()) // 循环处理每个事件
    {
        // PUT 包含新值和修改前的值，首次创建时旧值为空。
        if (event.event_type() == etcd::Event::EventType::PUT)
            std::cout << event.kv().key() << " 数据改变: " << event.prev_kv().as_string() << " -> " << event.kv().as_string() << std::endl;
        else if (event.event_type() == etcd::Event::EventType::DELETE_)
            std::cout << event.kv().key() << " 数据删除" << std::endl;
    }
}

// 读取当前键值，然后持续监听这个键。
int main(int argc, char *argv[])
{
    const std::string key = argc > 1 ? argv[1] : "/cpp-service-kit/example/lease/name";
    etcd::Client client("http://etcd-service:2379");

    // 1. 读取当前值，并记录读取时的 etcd 版本号。
    auto resp = client.get(key).get();
    if (!resp.is_ok())
    {
        std::cout << "获取数据失败: " << resp.error_message() << std::endl;
        return 1;
    }
    std::cout << "当前值: " << resp.value().as_string() << std::endl;

    // 2. 从监听到的数据的下一个版本开始监听，接收读取后发生的 PUT 和 DELETE 事件。
    etcd::Watcher watcher(client, key, resp.index() + 1, callback, false);
    std::cout << "开始监听，按 Ctrl+C 退出。" << std::endl;
    watcher.Wait(); // 阻塞等待事件发生
    return 0;
}
