#include <etcd/Client.hpp>
#include <etcd/KeepAlive.hpp>
#include <exception>
#include <iostream>
#include <string>

// 写入带租约的键值，并在程序运行期间为租约续期。
int main(int argc, char *argv[])
{
    const std::string key = argc > 1 ? argv[1] : "/cpp-service-kit/example/lease/name";
    const int ttl = 3;

    // 1. 通过 Docker 网络连接 etcd 服务。
    etcd::Client client("http://etcd-service:2379");

    // 2. 创建有效期为 3 秒的租约，并取得租约 ID。
    auto lease_resp = client.leasegrant(ttl).get();
    if (!lease_resp.is_ok())
    {
        std::cout << "创建租约失败: " << lease_resp.error_message() << std::endl;
        return 1;
    }
    auto lease_id = lease_resp.value().lease(); // 获取租约 ID

    // 3. 写入键值并关联租约，租约到期后 etcd 会删除该键。
    auto resp = client.put(key, "xiaoming", lease_id).get();
    if (!resp.is_ok())
    {
        std::cout << "添加数据失败: " << resp.error_message() << std::endl;
        return 1;
    }

    // 续租发生异常时，输出客户端提供的原因。
    auto handler = [](const std::exception_ptr &error)
    {
        try
        {
            if (error)
                std::rethrow_exception(error); // 重新抛出异常
        }
        catch (const std::exception &e)
        {
            std::cerr << "租约续期失败: " << e.what() << std::endl;
        }
    };

    // 4. KeepAlive 持续续租，程序退出，alive 被销毁，续期停止，租约到期后键值被删除。
    etcd::KeepAlive alive(client, handler, ttl, lease_id);
    std::cout << "写入成功: " << key << " = xiaoming，租约 ID: " << lease_id << std::endl;
    std::cout << "按回车停止续租，租约到期后键值自动删除。" << std::endl;
    std::cin.get();
    return 0;
}
