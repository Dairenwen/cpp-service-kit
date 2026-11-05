#include "BreezeRedis.hpp"

namespace Breezeredis
{
    std::shared_ptr<sw::redis::Redis> RedisFactory::create(const redis_settings &settings)
    {
        // 将服务器和认证信息传给 Redis++。
        sw::redis::ConnectionOptions conn_opts;
        conn_opts.host = settings.host;
        conn_opts.port = settings.port;
        conn_opts.user = settings.user;
        conn_opts.password = settings.passwd;
        conn_opts.db = settings.db;

        // Redis++ 使用连接池管理客户端连接。
        sw::redis::ConnectionPoolOptions pool_opts;
        pool_opts.size = settings.connection_pool_size;
        return std::make_shared<sw::redis::Redis>(conn_opts, pool_opts);
    }
}
