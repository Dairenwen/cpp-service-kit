#include "BreezeRedis.hpp"

#include <gtest/gtest.h>
#include <chrono>
#include <string>
#include <unistd.h>

// 使用工厂创建客户端，验证连接和字符串读写。
TEST(BreezeRedisTest, CreateAndReadWrite)
{
    Breezeredis::redis_settings settings;
    settings.host = "dev-redis";
    settings.passwd = "123456";
    settings.port = 6379;
    settings.db = 0;
    auto redis = Breezeredis::RedisFactory::create(settings);

    ASSERT_NE(redis, nullptr);
    EXPECT_EQ(redis->ping(), "PONG");

    // 使用独立的键，并设置过期时间，避免影响已有数据。
    const std::string key = "breeze_redis_test_" + std::to_string(getpid()) + "_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    ASSERT_TRUE(redis->set(key, "Hello Redis!", std::chrono::seconds(60), sw::redis::UpdateType::NOT_EXIST));
    auto value = redis->get(key);
    EXPECT_TRUE(value.has_value());
    if (value)
        EXPECT_EQ(*value, "Hello Redis!");

    // 只删除本次测试创建的键。
    EXPECT_EQ(redis->del(key), 1);
    EXPECT_FALSE(redis->get(key).has_value());
}
