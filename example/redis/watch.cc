#include <sw/redis++/redis.h>
#include <sw/redis++/queued_redis.h>
#include <cassert>
#include <string>
#include "init.hpp"

using namespace sw::redis;
int main()
{
    // 容器网络中的服务地址和示例密码；程序会修改 key 键。
    auto redis = init();
    redis->flushall();
    redis->set("key", "0");
    // 第二个 false 表示从原连接池获取连接；事务对象不能由多个线程同时操作
    auto tx = redis->transaction(false, false);
    // r 与事务共享连接，保证 WATCH 和事务命令使用同一连接。r 和 tx 都离开作用域后，共享的连接才不再被它们持有。
    auto r = tx.redis();
    // 被监听的键若被其他客户端修改，则重试事务
    while (true)
    {
        try
        {
            r.watch("key"); // 监视指定的 Key，实现乐观锁，防止并发修改导致事务基于过期数据执行
            auto val = r.get("key");
            auto num = 0;
            if (val)
            {
                num = std::stoi(*val);
            }
            ++num;
            auto replies = tx.set("key", std::to_string(num)).exec();
            assert(replies.size() == 1 && replies.get<bool>(0) == true);
            break;
        }
        catch (const WatchError &err)
        {
            continue;
        }
        catch (const Error &err)
        {
            throw;
        }
    }
    redis->del("key");
    return 0;
}
