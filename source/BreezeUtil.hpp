#pragma once

#include <json/json.h>
#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace BreezeUtil
{
    // 提供 JSON 值和字符串之间的转换。
    class JSON
    {
    public:
        // styled 为 true 时输出带缩进的 JSON。
        static std::optional<std::string> serialize(const Json::Value &value, bool styled = false);

        // 解析完整 JSON 文本，支持对象、数组和其他 JSON 值。
        static std::optional<Json::Value> unserialize(const std::string &text);
    };

    // 以二进制方式读取和覆盖写入整个文件。
    class FUTIL
    {
    public:
        // 读取成功后将文件内容写入 body。
        static bool read(const std::string &filename, std::string &body);

        // 将 body 写入文件，原有内容会被覆盖。
        static bool write(const std::string &filename, const std::string &body);
    };

    // 提供字符串分割功能。
    class STR
    {
    public:
        // 将非空片段追加到 result，返回 result 中的元素总数；空分隔符保留原字符串。
        static std::size_t split(const std::string &text, const std::string &separator, std::vector<std::string> &result);
    };
}
