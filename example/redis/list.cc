#include <iostream>
#include <string>
#include <cassert>
#include "init.hpp"

int main()
{
    size_t sz = 0;
    auto redis = init();
    redis->flushall();
    // 从列表右侧插入，返回当前列表长度。
    sz = redis->rpush("names", "name1");
    std::vector<std::string> elements = {"name2", "name3", "name4", "name5"};
    sz = redis->rpush("names", elements.begin(), elements.end());
    std::cout << "cur count: " << sz << std::endl;
    // 从左侧弹出一个元素，然后读取剩余的全部元素
    auto name1 = redis->lpop("names");
    if (name1)
        std::cout << *name1 << std::endl;
    std::vector<std::string> vals;
    redis->lrange("names", 0, -1, std::back_inserter(vals)); // 读取全部元素，下标从 0 开始，-1 表示最后一个元素
    for (auto &v : vals)
    {
        std::cout << v << std::endl;
    }

    // LLEN 获取长度，LINDEX 按下标读取；不存在的下标返回空值。
    std::cout << "列表长度: " << redis->llen("names") << std::endl;
    auto first = redis->lindex("names", 0);
    if (first)
        std::cout << "第一个元素: " << *first << std::endl;

    // LPUSH 从左侧插入，RPOP 从右侧弹出。
    redis->lpush("names", "new_name");
    auto last = redis->rpop("names");
    if (last)
        std::cout << "右侧弹出: " << *last << std::endl;
    return 0;
}
