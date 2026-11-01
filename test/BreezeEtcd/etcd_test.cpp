#include "BreezeEtcd.hpp"
#include "BreezeUtil.hpp"

#include <gtest/gtest.h>
#include <chrono>
#include <future>
#include <iostream>
#include <string>

// 测试服务注册、发现和租约到期后的下线通知。
TEST(BreezeEtcdTest, RegisterAndWatch)
{
    const std::string url = "http://etcd-service:2379";
    const std::string suffix = BreezeUtil::Random::code();
    const std::string user_service = "breeze_user_" + suffix;
    const std::string order_service = "breeze_order_" + suffix;
    const std::string user_address = "127.0.0.1:9001";
    const std::string order_address = "127.0.0.1:9002";

    // 保存回调结果，并让测试线程等待监听线程完成通知。
    std::promise<std::string> user_online;
    std::promise<std::string> order_online;
    std::promise<std::string> user_offline;
    std::promise<std::string> order_offline;
    auto user_online_result = user_online.get_future();
    auto order_online_result = order_online.get_future();
    auto user_offline_result = user_offline.get_future();
    auto order_offline_result = order_offline.get_future();

    SvcWatcher watcher(url, [&](std::string name, std::string addr)
                       {
                            if (name == user_service || name == order_service)
                            {
                                std::cout << "服务上线: " << name << " -> " << addr << std::endl;
                                if (name == user_service)
                                    user_online.set_value(addr);
                                else
                                    order_online.set_value(addr);
                            } }, [&](std::string name, std::string addr)
                       {
                            if (name == user_service || name == order_service)
                            {
                                std::cout << "服务下线: " << name << " -> " << addr << std::endl;
                                if (name == user_service)
                                    user_offline.set_value(addr);
                                else
                                    order_offline.set_value(addr);
                            } });

    {
        // 键 /breeze_user_<随机UUID>/<节点UUID>，值 127.0.0.1:9001。
        SvcProvider user(url, user_service, user_address);
        ASSERT_TRUE(user.registry());
        // 键 /breeze_order_<随机UUID>/<节点UUID>，值 127.0.0.1:9002。
        SvcProvider order(url, order_service, order_address);
        ASSERT_TRUE(order.registry());

        // 发现对象先读取已有节点，再监听后续变化。
        ASSERT_TRUE(watcher.watch());
        ASSERT_EQ(user_online_result.wait_for(std::chrono::seconds(15)), std::future_status::ready);
        EXPECT_EQ(user_online_result.get(), user_address);
        ASSERT_EQ(order_online_result.wait_for(std::chrono::seconds(15)), std::future_status::ready);
        EXPECT_EQ(order_online_result.get(), order_address);
    } // 两个注册对象销毁后停止续租，租约到期时 etcd 删除对应的键。

    // 等待下线回调，确认删除事件中的地址。
    ASSERT_EQ(user_offline_result.wait_for(std::chrono::seconds(15)), std::future_status::ready);
    EXPECT_EQ(user_offline_result.get(), user_address);
    ASSERT_EQ(order_offline_result.wait_for(std::chrono::seconds(15)), std::future_status::ready);
    EXPECT_EQ(order_offline_result.get(), order_address);
}
