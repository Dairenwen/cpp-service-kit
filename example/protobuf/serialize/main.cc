#include "person.pb.h"

#include <iostream>
#include <string>

// 将 Person 序列化为二进制字符串，再解析并读取全部字段。
int main() {
    example::Person person;
    person.set_name("Jack");
    person.set_age(28);
    person.set_email("qq.com");
    person.add_skills("C++");
    person.add_skills("Java");
    person.add_skills("Python");

    // mutable_scores 返回可写映射，用于添加各科成绩。
    auto score_map = person.mutable_scores();
    score_map->insert({"Ch", 88});
    score_map->insert({"Mt", 77});
    score_map->insert(google::protobuf::MapPair<std::string, float>("En", 99));

    std::string pstr = person.SerializeAsString();
    if (pstr.empty()) {
        std::cout << "SerializeAsString Failed!\n";
        return -1;
    }

    // 将二进制数据解析到独立对象，读取解析结果。
    example::Person tmp;
    bool ret = tmp.ParseFromString(pstr);
    if (ret == false) {
        std::cout << "ParseFromString Failed!\n";
        return -1;
    }

    std::cout << "name: " << tmp.name() << std::endl;
    std::cout << "age: " << tmp.age() << std::endl;
    std::cout << "email: " << tmp.email() << std::endl;
    for (int i = 0; i < tmp.skills_size(); ++i) {
        std::cout << "skills: " << tmp.skills(i) << std::endl;
    }
    for (const auto& score : tmp.scores()) {
        std::cout << score.first << " = " << score.second << std::endl;
    }
    auto it = tmp.scores().find("Ch");
    if (it != tmp.scores().end()) {
        std::cout << "Ch: " << it->second << std::endl;
    }
    return 0;
}
