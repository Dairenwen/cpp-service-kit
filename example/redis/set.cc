#include <sw/redis++/redis.h>
#include <iostream>
#include <chrono>
#include <unordered_set>
#include "init.hpp"

int main()
{
    try
    {
        // 1. 实例化redis客户端对象
        auto redis = init();
        redis->flushall();

        // 对无序集合set的操作
        // 1. 向集合中添加一个元素
        redis->sadd("skills", "C++");
        // 2. 批量向集合中添加多个元素
        std::unordered_set<std::string> skills = {"Python", "Java", "Go"};
        redis->sadd("skills", skills.begin(), skills.end());
        // 3. 获取集合中元素的个数
        auto count = redis->scard("skills");
        std::cout << "skills size: " << count << std::endl;
        // 4. 判断集合中是否包含某个元素
        bool ret = redis->sismember("skills", "C++");
        std::cout << "skills contains C++: " << ret << std::endl;
        ret = redis->sismember("skills", "C");
        std::cout << "skills contains C: " << ret << std::endl;
        // 5. 获取集合中所有元素
        std::unordered_set<std::string> skills2;
        redis->smembers("skills", std::inserter(skills2, skills2.begin()));
        for (auto &val : skills2)
        {
            std::cout << val << " ";
        }
        std::cout << std::endl;

        // SREM 删除指定成员；重复删除不存在的成员时返回 0。
        std::cout << "删除 Go 的数量: " << redis->srem("skills", "Go") << std::endl;
        std::cout << "删除后的数量: " << redis->scard("skills") << std::endl;

        redis->del("skills");
    }
    catch (const sw::redis::Error &e)
    {
        std::cout << "redis error: " << e.what() << std::endl;
    }
    return 0;
}
