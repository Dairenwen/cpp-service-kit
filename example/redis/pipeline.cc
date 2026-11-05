#include <iostream>
#include <string>
#include <unordered_map>
#include <cassert>
#include <sw/redis++/queued_redis.h>
#include "init.hpp"

// Transaction 和 Pipeline 都可能独占连接，因此建议连接池大于 1，以避免其他 Redis 操作因获取不到连接而阻塞。
int main()
{
    // 使用前一个 transaction 示例添加的数据进行测试
    // pipeline将命令打包，减少网络往返次数，提高性能，但不保证原子性，适用于对性能要求高且不需要事务的场景
    auto redis = init();
    // 用局部作用域限制管道对象的生命周期。
    {
        // false 表示从原连接池获取连接；管道对象不能由多个线程同时操作。
        auto pl = redis->pipeline(false);
        try
        {
            // 命令先进入管道，exec 后才能按命令顺序通过下标读取结果。
            auto replies = pl.get("username")
                               .get("nickname")
                               .get("xxxxx")
                               .lrange("skills", 0, -1)
                               .hgetall("properties")
                               .exec();
            auto username = replies.get<sw::redis::OptionalString>(0); // 获取第一个命令的结果
            if (username)
                std::cout << *username << std::endl;
            auto nickname = replies.get<sw::redis::OptionalString>(1); // 获取第二个命令的结果
            if (nickname)
                std::cout << *nickname << std::endl;
            auto xxxxxxxx = replies.get<sw::redis::OptionalString>(2);
            if (xxxxxxxx)
                std::cout << *xxxxxxxx << std::endl;
            else
                std::cout << "xxxxxxxx not exists!\n";
            std::vector<std::string> skills;
            replies.get(3, std::back_inserter(skills)); // 获取第四个命令的结果
            for (auto &v : skills)
            {
                std::cout << v << std::endl;
            }
            std::unordered_map<std::string, std::string> properties;
            // 获取第五个命令的结果
            replies.get(4, std::inserter(properties, properties.begin()));
            for (auto &v : properties)
            {
                std::cout << v.first << "=" << v.second << std::endl;
            }
        }
        catch (sw::redis::Error &e)
        {
            std::cout << e.what() << std::endl;
            pl.discard();
        }
    } // pl 在此析构；正常 exec 后连接也可能提前归还连接池。
    return 0;
}
