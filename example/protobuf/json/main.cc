#include "person.pb.h"
#include <google/protobuf/util/json_util.h>
#include <iostream>
#include <string>

// class Person : public google::protobuf::Message {
//  生成的字段访问函数和消息处理函数。
//};Person类继承了google::protobuf::Message

// 将消息转换为带缩进的 JSON，输出并返回转换结果。
std::string p2j(google::protobuf::Message &msg)
{
    google::protobuf::util::JsonPrintOptions opts;
    opts.add_whitespace = true;
    std::string json_str;
    auto ret = google::protobuf::util::MessageToJsonString(msg, &json_str, opts);
    if (ret.ok() == false)
    {
        std::cout << ret.ToString() << std::endl;
        return "";
    }
    std::cout << json_str << std::endl;
    return json_str;
}

// 将 JSON 转为 Person，读取字段并再次转回 JSON
void j2p(const std::string &json_str)
{
    example::Person person;
    auto ret = google::protobuf::util::JsonStringToMessage(json_str, &person);
    if (ret.ok() == false)
    {
        std::cout << ret.ToString() << std::endl;
        return;
    }

    // optional 字段通过 has 接口查询显式设置状态。
    if (person.has_name())
        std::cout << "person.name:" << person.name() << std::endl;
    if (person.has_age())
        std::cout << "person.age:" << person.age() << std::endl;
    if (person.has_email())
        std::cout << "person.email:" << person.email() << std::endl;

    // 列表和映射通过元素数量及迭代器访问。
    for (int i = 0; i < person.skills_size(); ++i)
        std::cout << "person.skills:" << person.skills(i) << std::endl;
    for (const auto &score : person.scores())
        std::cout << score.first << " = " << score.second << std::endl;
}

// 构造包含显式零值年龄的消息，演示 Protobuf 与 JSON 双向转换。
int main()
{
    example::Person person;
    person.set_name("Jack");
    person.set_age(0);
    person.set_email("qq.com");
    person.add_skills("C++");
    person.add_skills("Java");
    person.add_skills("Python");
    auto score_map = person.mutable_scores();
    score_map->insert({"Ch", 88});
    score_map->insert({"Mt", 77});
    score_map->insert({"En", 99});

    std::cout << "---------------------------------------\n";
    std::string str = p2j(person);
    std::cout << "---------------------------------------\n";
    j2p(str);
    std::cout << "---------------------------------------\n";
    return 0;
}
