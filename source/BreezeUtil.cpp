#include "BreezeUtil.hpp"
#include "BreezeLog.hpp"

#include <fstream>
#include <memory>
#include <sstream>
#include <utility>

namespace BreezeUtil
{
    // 将 JSON 值转换为紧凑文本或带缩进的文本。
    std::optional<std::string> JSON::serialize(const Json::Value &value, bool styled)
    {
        Json::StreamWriterBuilder builder;
        builder["commentStyle"] = "None";
        builder["indentation"] = styled ? "  " : "";
        std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
        std::ostringstream output;
        if (writer->write(value, &output) != 0 || !output)
        {
            ERR("JSON 序列化失败");
            return std::nullopt;
        }
        return output.str();
    }

    // 解析完整 JSON 文本，并记录解析错误。
    std::optional<Json::Value> JSON::unserialize(const std::string &text)
    {
        Json::CharReaderBuilder builder;
        builder["allowComments"] = false;
        builder["allowTrailingCommas"] = false;
        builder["failIfExtra"] = true;
        std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
        Json::Value value;
        std::string error;
        if (!reader->parse(text.data(), text.data() + text.size(), &value, &error))
        {
            ERR("JSON 反序列化失败: {}", error);
            return std::nullopt;
        }
        return value;
    }

    // 读取完整文件，保留二进制内容。
    bool FUTIL::read(const std::string &filename, std::string &body)
    {
        std::ifstream file(filename, std::ios::binary);
        if (!file)
        {
            ERR("打开文件失败: {}", filename);
            return false;
        }

        std::string content;
        char buffer[4096];
        while (file.read(buffer, sizeof(buffer)) || file.gcount() > 0)
        {
            content.append(buffer, file.gcount());
        }
        if (file.bad())
        {
            ERR("读取文件失败: {}", filename);
            return false;
        }
        body = std::move(content);
        return true;
    }

    // 覆盖写入完整文件，关闭文件后确认写入结果。
    bool FUTIL::write(const std::string &filename, const std::string &body)
    {
        std::ofstream file(filename, std::ios::binary | std::ios::trunc);
        if (!file)
        {
            ERR("打开文件失败: {}", filename);
            return false;
        }
        file.write(body.data(), body.size());
        file.close();
        if (!file)
        {
            ERR("写入文件失败: {}", filename);
            return false;
        }
        return true;
    }

    // 根据分隔符依次提取非空片段，支持多字符分隔符。
    std::size_t STR::split(const std::string &text, const std::string &separator, std::vector<std::string> &result)
    {
        if (separator.empty())
        {
            if (!text.empty())
                result.push_back(text);
            return result.size();
        }

        std::size_t start = 0;
        std::size_t position;
        while ((position = text.find(separator, start)) != std::string::npos)
        {
            if (position > start)
                result.push_back(text.substr(start, position - start));
            start = position + separator.size();
        }
        if (start < text.size())
            result.push_back(text.substr(start));
        return result.size();
    }
}
