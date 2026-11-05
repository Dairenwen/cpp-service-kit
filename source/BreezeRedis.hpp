#pragma once

#include <sw/redis++/redis.h>
#include <cstddef>
#include <memory>
#include <string>

namespace Breezeredis
{
    // Redis 连接和连接池配置。
    struct redis_settings
    {
        int db = 0;                           // 数据库编号
        int port = 6379;                      // 服务器端口
        std::string host;                     // 服务器地址
        std::string user = "default";         // Redis 用户名
        std::string passwd;                   // 访问密码
        std::size_t connection_pool_size = 3; // 连接池最大连接数
    };

    // 根据配置创建 Redis 客户端。
    class RedisFactory
    {
    public:
        static std::shared_ptr<sw::redis::Redis> create(const redis_settings &settings);
    };
}
