#include <iostream>
#include <string>
#include <unordered_map>
#include <cassert>
#include "init.hpp"

int main()
{
    auto redis = init();
    redis->flushall();
    // hset 设置一个字段，hmset 批量设置字段。
    int sz = redis->hset("student", "name", "zhangsan");
    std::unordered_map<std::string, std::string> fields = {
        {"age", "18"},
        {"sex", "male"},
        {"height", "175"},
        {"weight", "85"}};
    redis->hmset("student", fields.begin(), fields.end());
    auto name = redis->hget("student", "name");
    if (name)
        std::cout << *name << std::endl;
    std::vector<std::string> keys = {
        "name", "age", "sex", "height", "weight", "xxxx"};
    std::vector<sw::redis::OptionalString> vals;
    redis->hmget("student", keys.begin(), keys.end(), std::back_inserter(vals));
    for (auto &v : vals)
    {
        std::cout << *v << std::endl;
    }

    std::unordered_map<std::string, std::string> results;
    redis->hgetall("student", std::inserter(results, results.begin()));
    for (auto &r : results)
    {
        std::cout << r.first << "=" << r.second << std::endl;
    }

    // HEXISTS 检查字段，HINCRBY 对保存整数的字段加 1。
    std::cout << "是否有 age 字段: " << redis->hexists("student", "age") << std::endl;
    std::cout << "年龄加 1: " << redis->hincrby("student", "age", 1) << std::endl;
    // HDEL 删除指定字段，返回实际删除的字段数量。
    std::cout << "删除 height 字段数: " << redis->hdel("student", "height") << std::endl;
    return 0;
}
