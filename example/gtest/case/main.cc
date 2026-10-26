#include <gtest/gtest.h>
#include <iostream>
#include <string>
#include <unordered_map>
#include <utility>

// 在每个用例开始前建立相同的词典数据。
class HashTestEnv2 : public testing::Test {
public:
    // 在整组用例开始前执行套件初始化。
    static void SetUpTestCase() {
        std::cout << "环境2第一个TEST之前被调用,进行总体环境配置\n";
    }

    // 在整组用例结束后执行套件清理。
    static void TearDownTestCase() {
        std::cout << "环境2最后一个TEST之后被调用,进行总体环境清理\n";
    }

    // 为当前用例填充两个初始词条。
    void SetUp() override {
        std::cout << "环境2测试前:提前准备数据!!\n";
        dict.insert(std::make_pair("bye", "再见"));
        dict.insert(std::make_pair("see you", "再见"));
    }

    // 在当前用例结束后清空词典。
    void TearDown() override {
        std::cout << "环境2测试结束后:清理数据!!\n";
        dict.clear();
    }

    std::unordered_map<std::string, std::string> dict;
};

// 检查插入一个词条后词典包含三个元素。
TEST_F(HashTestEnv2, insert_test) {
    std::cout << "环境2,中间测试\n";
    dict.insert(std::make_pair("hello", "你好"));
    ASSERT_EQ(dict.size(), 3);
}

// 检查当前用例仅包含 SetUp 准备的两个初始词条。
TEST_F(HashTestEnv2, size_test) {
    std::cout << "环境2,中间size测试\n";
    auto it = dict.find("hello");
    ASSERT_EQ(it, dict.end());
    ASSERT_EQ(dict.size(), 2);
}

// 初始化框架并运行逐用例环境演示。
int main(int argc, char* argv[]) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
