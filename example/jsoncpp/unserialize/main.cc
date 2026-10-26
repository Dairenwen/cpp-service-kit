#include <jsoncpp/json/json.h>
#include <iostream>
#include <memory>
#include <string>

// 解析 JSON 字符串，并按字段类型读取基础值、数组和嵌套对象。
int main()
{
    std::string str = R"(
        {
            "address": {
                "city": "Anytown",
                "state": "CA",
                "street": "123 Main St"
            },
            "age": 30,
            "is_student": false,
            "name": "John Doe",
            "skills": ["JavaScript", "Python", "C++"]
        }
    )";

    Json::Value value;
    Json::CharReaderBuilder build;
    std::unique_ptr<Json::CharReader> pcr(build.newCharReader());
    std::string err;
    // 输入范围覆盖整个字符串，结束指针指向末尾后一位。
    bool ret = pcr->parse(str.data(), str.data() + str.size(), &value, &err);
    if (ret == false)
    {
        std::cout << "Error: " << err << std::endl;
        return -1;
    }

    // 字段存在时按对应类型读取，weight 用于演示可选字段访问。
    if (!value["name"].isNull())
        std::cout << "name: " << value["name"].asString() << std::endl;
    if (!value["age"].isNull())
        std::cout << "age: " << value["age"].asInt() << std::endl;
    if (!value["is_student"].isNull())
        std::cout << "is_student: " << value["is_student"].asBool() << std::endl;
    if (!value["weight"].isNull())
        std::cout << "weight: " << value["weight"].asFloat() << std::endl;

    // 数组通过下标逐项读取。
    if (!value["skills"].isNull() && value["skills"].isArray())
    {
        for (Json::ArrayIndex i = 0; i < value["skills"].size(); ++i)
        {
            std::cout << "skills: " << value["skills"][i].asString() << std::endl;
        }
    }
    // 字典按对象成员读取。
    if (!value["address"].isNull() && value["address"].isObject())
    {
        std::cout << "address.street: " << value["address"]["street"].asString() << std::endl;
        std::cout << "address.city: " << value["address"]["city"].asString() << std::endl;
        std::cout << "address.state: " << value["address"]["state"].asString() << std::endl;
    }
    return 0;
}
