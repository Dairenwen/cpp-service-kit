#include <jsoncpp/json/json.h>
#include <iostream>
#include <memory>
#include <sstream>

// 构造包含数组和嵌套地址对象的数据，序列化为 JSON 字符串。
int main() {
    Json::Value item;
    item["street"] = "123 Main St";
    item["city"] = "Anytown";
    item["state"] = "CA";

    Json::Value value;
    value["name"] = "John Doe";
    value["age"] = 30;
    value["is_student"] = false;
    value["skills"].append("JavaScript");
    value["skills"].append("Python");
    value["skills"].append("C++");
    value["address"] = item;

    // 通过工厂创建写入器，默认缩进便于阅读输出内容。
    Json::StreamWriterBuilder swb;
    std::unique_ptr<Json::StreamWriter> psw(swb.newStreamWriter());
    std::stringstream ss;
    try {
        psw->write(value, &ss);
        std::cout << ss.str() << std::endl;
    } catch (const Json::Exception& e) {
        std::cout << "Error:" << e.what() << std::endl;
    }
    return 0;
}
