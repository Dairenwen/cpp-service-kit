#include <iostream>
#include <string>
#include <cassert>
#include "init.hpp"

int main()
{
    auto redis = init();
    redis->flushall();                       // 清空 Redis 实例中的所有数据库，只能在临时 Redis 中运行
    assert(redis->set("name1", "zhangsan")); // 写入字符串键值对
    std::vector<std::pair<std::string, std::string>> names;
    names.push_back({"name2", "lisi"});
    names.push_back({"name3", "wangwu"});
    names.push_back({"name4", "zhaoliu"});
    names.push_back({"name5", "liba"});
    // mset 批量写入；mget 按给定键的顺序返回可选值。
    redis->mset(names.begin(), names.end());
    auto name1 = redis->get("name1");
    if (name1)
        std::cout << *name1 << std::endl;
    std::vector<std::string> keys = {
        "name1", "name2", "name3", "name4", "name5", "name6"};
    std::vector<sw::redis::OptionalString> vals; // 可选值类型，表示可能存在或不存在的字符串值
    redis->mget(keys.begin(), keys.end(), std::back_inserter(vals));
    for (auto &v : vals)
    {
        if (v)
            std::cout << *v << std::endl;
    }

    // APPEND 在已有字符串末尾追加内容，STRLEN 返回字符串的字节数。
    redis->append("name1", "_hello");
    auto updated_name = redis->get("name1");
    if (updated_name)
        std::cout << "追加后: " << *updated_name << std::endl;
    std::cout << "字节数: " << redis->strlen("name1") << std::endl;

    // INCRBY / DECRBY 对保存整数的字符串执行加减。
    redis->set("counter", "10");
    std::cout << "加 5: " << redis->incrby("counter", 5) << std::endl;
    std::cout << "减 3: " << redis->decrby("counter", 3) << std::endl;
    redis->del("counter");
    return 0;
}
