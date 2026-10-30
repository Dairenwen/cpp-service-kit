#include "BreezeRPC.hpp"
#include "cal.pb.h"
#include <brpc/controller.h>
#include <gtest/gtest.h>
#include <atomic>

std::shared_ptr<brpc::Server> start_test_server();

// 验证节点按顺序轮询，删除后只选择剩余节点。
TEST(ChannelsTest, SelectAndRemove)
{
    Channels channels;
    channels.insert("127.0.0.1:9000");
    channels.insert("127.0.0.1:9001");

    auto first = channels.select();
    auto second = channels.select();
    ASSERT_NE(first, nullptr);
    ASSERT_NE(second, nullptr);
    EXPECT_NE(first, second);
    EXPECT_EQ(channels.select(), first);

    channels.remove("127.0.0.1:9000");
    EXPECT_EQ(channels.select(), second);
}

// 验证服务节点的添加、选择和删除。
TEST(SvcChannelsTest, ManageNodes)
{
    SvcChannels services;
    services.addNode("calculator", "127.0.0.1:9000");
    ASSERT_NE(services.getNode("calculator"), nullptr);

    services.delNode("calculator", "127.0.0.1:9000");
    EXPECT_EQ(services.getNode("calculator"), nullptr);
}

// 通过服务端工厂和通道管理器完成同步及异步加法调用。
TEST(BreezeRpcTest, SyncAndAsyncCall)
{
    auto server = start_test_server();
    ASSERT_NE(server, nullptr);

    SvcChannels services;
    services.addNode("calculator", "127.0.0.1:" + std::to_string(server->listen_address().port));
    auto channel = services.getNode("calculator");
    ASSERT_NE(channel, nullptr);
    breeze_test::Calculator_Stub client(channel.get()); // 创建客户端存根对象，使用通道进行RPC调用

    breeze_test::AddRequest request;
    request.set_a(10);
    request.set_b(20);
    breeze_test::AddResponse response;
    brpc::Controller controller;
    client.Add(&controller, &request, &response, nullptr);
    ASSERT_FALSE(controller.Failed()) << controller.ErrorText();
    EXPECT_EQ(response.result(), 30);

    // 等待异步回调结束，确保请求和响应在调用期间有效。
    brpc::Controller async_controller;
    breeze_test::AddResponse async_response;
    auto done = ClosureFactor::create(
        [cntl = &async_controller, req = &request, response = &async_response]()
        {
            if (cntl->Failed())
            {
                LOG(ERROR) << "RPC failed: " << cntl->ErrorText();
            }
            else
            {
                LOG(INFO) << req->a() << " + " << req->b() << " = " << response->result();
            }
        });
    client.Add(&async_controller, &request, &async_response, done);
    brpc::Join(async_controller.call_id()); // 等待异步调用完成
    ASSERT_FALSE(async_controller.Failed()) << async_controller.ErrorText();
    EXPECT_EQ(async_response.result(), 30);

    server->Stop(0);
    server->Join();
}
