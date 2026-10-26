#include <gtest/gtest.h>
#include <iostream>
#include <string>
#include <unordered_map>
#include <utility>

std::unordered_map<std::string, std::string> dict;

// 为整个测试程序准备和清理共享词典。
class HashTestEnv : public testing::Environment {
public:
    // 在全部测试开始前填充共享数据。
    void SetUp() override {
        std::cout << "测试前:提前准备数据!!\n";
        dict.insert(std::make_pair("Hello", "你好"));
        dict.insert(std::make_pair("hello", "你好"));
        dict.insert(std::make_pair("雷吼", "你好"));
    }

    // 在全部测试结束后清空共享词典。
    void TearDown() override {
        std::cout << "测试结束后:清理数据!!\n";
        dict.clear();
    }
};

// 检查全局环境预先填充的 hello 词条。
TEST(hash_case_test, find_test) {
    auto it = dict.find("hello");
    ASSERT_NE(it, dict.end());
}

// 检查共享词典包含初始化数据。
TEST(hash_case_test, size_test) {
    ASSERT_GT(dict.size(), 0);
}

// 注册由 GTest 管理生命周期的全局环境，再运行测试。
int main(int argc, char* argv[]) {
    testing::AddGlobalTestEnvironment(new HashTestEnv);
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
