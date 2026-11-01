#include "BreezeEtcd.hpp"
#include "BreezeRPC.hpp"
#include "BreezeUtil.hpp"
#include "cal.pb.h"

#include <brpc/controller.h>
#include <gtest/gtest.h>
#include <iostream>
#include <memory>
#include <string>

// 复用 BreezeRPC 测试中的加法服务，端口由系统分配。
std::shared_ptr<brpc::Server> start_test_server();

// 通过 etcd 发现加法服务，再使用 BreezeRPC 发起调用。
TEST(BreezeEtcdRpcTest, DiscoverAndCall)
{
    auto server = start_test_server(); // 启动brpc服务，端口由系统分配
    ASSERT_NE(server, nullptr);

    const std::string url = "http://etcd-service:2379";
    const std::string service = "brpc_serivce_" + BreezeUtil::Random::code();
    const std::string address = "127.0.0.1:" + std::to_string(server->listen_address().port); // 获取brpc服务地址

    // 写入 etcd：键 /brpc_serivce_<随机UUID>/<节点UUID>，值为 RPC 服务地址。
    SvcProvider provider(url, service, address);
    ASSERT_TRUE(provider.registry());

    // 发现节点时加入 RPC 通道，节点下线时移除对应通道。
    SvcChannels channels;
    channels.setWatch(service);
    SvcWatcher watcher(url, [&](std::string name, std::string addr)
                       {
                            if (name == service)// 发现rbpc 服务节点，加入通道
                            {
                                std::cout << "服务上线: " << name << " -> " << addr << std::endl;
                                channels.addNode(name, addr);
                            } }, [&](std::string name, std::string addr)
                       {
                            if (name == service)
                            {
                                std::cout << "服务下线: " << name << " -> " << addr << std::endl;
                                channels.delNode(name, addr);
                            } });
    ASSERT_TRUE(watcher.watch());

    // 通过发现的地址获取通道，调用已有加法服务。
    auto channel = channels.getNode(service);
    ASSERT_NE(channel, nullptr);
    breeze_test::Calculator_Stub client(channel.get()); // 创建客户端存根
    breeze_test::AddRequest request;
    request.set_a(10);
    request.set_b(20);
    breeze_test::AddResponse response;
    brpc::Controller controller;
    controller.set_timeout_ms(3000);                       // 设置请求超时时间
    client.Add(&controller, &request, &response, nullptr); // 发起同步 RPC 调用
    ASSERT_FALSE(controller.Failed()) << controller.ErrorText();
    EXPECT_EQ(response.result(), 30);
    std::cout << "通过 etcd 发现 RPC 服务: " << address << std::endl;
    std::cout << "10 + 20 = " << response.result() << std::endl;

    // 关闭加法服务，注册对象销毁后租约停止续期。
    server->Stop(0);
    server->Join();
}
