#include <iostream>
#include <string>
#include <unordered_map>
#include <cassert>
#include <thread>
#include <chrono>
#include "init.hpp"
// 读取并打印三个不同类型的键，观察过期前后的结果。
void show(std::shared_ptr<sw::redis::Redis> &redis)
{
    auto username = redis->get("username");
    if (username)
        std::cout << *username << std::endl;
    auto nickname = redis->get("nickname");
    if (nickname)
        std::cout << *nickname << std::endl;
    std::vector<std::string> skills;
    redis->lrange("skills", 0, -1, std::back_inserter(skills));
    for (auto &v : skills)
    {
        std::cout << v << std::endl;
    }
    std::unordered_map<std::string, std::string> properties;
    redis->hgetall("properties", std::inserter(properties, properties.begin()));
    for (auto &r : properties)
    {
        std::cout << r.first << "=" << r.second << std::endl;
    }
}
int main()
{
    auto redis = init();
    redis->flushall();
    assert(redis->set("username", "zhangsan"));
    assert(redis->set("nickname", "xiaozhang", std::chrono::milliseconds(3000))); // 设置 3 秒有效期
    std::vector<std::string> skills = {"C++", "Java", "Go", "Python"};
    redis->rpush("skills", skills.begin(), skills.end());
    std::unordered_map<std::string, std::string> fields = {
        {"age", "18"},
        {"sex", "male"},
        {"height", "175"},
        {"weight", "85"}};
    redis->hmset("properties", fields.begin(), fields.end());
    // 为三个键设置 3 秒有效期，再取消 username 的过期时间。
    assert(redis->expire("username", std::chrono::seconds(3)));
    assert(redis->expire("skills", std::chrono::seconds(3)));
    assert(redis->expire("properties", std::chrono::seconds(3)));
    assert(redis->persist("username")); // 取消 username 的过期时间
    // EXISTS 判断 Redis 键是否存在：1 表示存在，0 表示不存在。
    std::cout << "过期前 nickname 是否存在: " << redis->exists("nickname") << std::endl;
    show(redis);
    std::this_thread::sleep_for(std::chrono::seconds(4));
    std::cout << "-------------4s later-------------------";
    show(redis);
    // nickname 已过期；username 因为调用过 PERSIST，仍然存在。
    std::cout << "过期后 nickname 是否存在: " << redis->exists("nickname") << std::endl;
    std::cout << "过期后 username 是否存在: " << redis->exists("username") << std::endl;
    return 0;
}
