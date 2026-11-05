#pragma once
#include <sw/redis++/redis.h>

std::shared_ptr<sw::redis::Redis> init()
{
    sw::redis::ConnectionPoolOptions pool_opts; // 连接池选项。
    pool_opts.size = 3;                         // 最大连接总数，由于开启了连接池，数量不能为 0
    sw::redis::ConnectionOptions opts;          // 连接选项，host、port、password、db 等。
    opts.host = "dev-redis";                    // Redis 服务名
    opts.port = 6379;                           // Redis 服务器端口
    opts.password = "123456";                   // Redis 服务器密码
    opts.db = 0;                                // Redis 服务器数据库
    opts.keep_alive = true;                     // 是否保持连接
    return std::make_shared<sw::redis::Redis>(opts, pool_opts);
}
