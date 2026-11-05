#include <iostream>
#include <string>
#include <unordered_map>
#include <cassert>
#include <sw/redis++/queued_redis.h>
#include "init.hpp"
// 读取并打印三个不同类型的键，观察过期前后的结果
void show(std::shared_ptr<sw::redis::Redis> &redis)
{
    auto username = redis->get("username");
    if (username)
        std::cout << *username << std::endl;
    auto nickname = redis->get("nickname");
    if (nickname)
        std::cout << *nickname << std::endl;
    std::vector<std::string> skills;
    redis->lrange("skills", 0, -1, std::back_inserter(skills)); // 获取列表所有元素
    for (auto &v : skills)
    {
        std::cout << v << std::endl;
    }
    std::unordered_map<std::string, std::string> properties;
    redis->hgetall("properties", std::inserter(properties, properties.begin())); // 获取哈希表所有字段
    for (auto &r : properties)
    {
        std::cout << r.first << "=" << r.second << std::endl;
    }
}
int main()
{
    auto redis = init();
    redis->flushall();
    std::vector<std::string> skills = {"C++", "Java", "Go", "Python"};
    std::unordered_map<std::string, std::string> fields = {
        {"age", "18"},
        {"sex", "male"},
        {"height", "175"},
        {"weight", "85"}};

    // tx 只在当前线程内使用，不能由多个线程同时操作。
    auto tx = redis->transaction(false, false); // false 表示从原连接池获取连接；事务对象不能由多个线程同时操作
    // 命令先进入事务队列，exec 后才可取得对应结果。
    tx.set("username", "zhangsan")
        .set("nickname", "xiaozhang")
        .rpush("skills", skills.begin(), skills.end())
        .hmset("properties", fields.begin(), fields.end())
        .exec();
    // 正常 exec 后连接可归还连接池；离开作用域时 tx 也会析构。
    show(redis);
    return 0;
}
