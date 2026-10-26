#include <gtest/gtest.h>
#include <iostream>
#include <string>
#include <unordered_map>
#include <utility>

// 套件事件围绕整组测试执行，每个用例各自持有一个实例。
class HashTestEnv1 : public testing::Test {
public:
    // 在套件的第一个用例开始前执行。
    static void SetUpTestCase() {
        std::cout << "环境1第一个TEST之前调用\n";
    }

    // 在套件的最后一个用例结束后执行。
    static void TearDownTestCase() {
        std::cout << "环境1最后一个TEST之后调用\n";
    }

    std::unordered_map<std::string, std::string> dict;
};

// 向当前用例的词典插入数据并检查查询结果。
TEST_F(HashTestEnv1, insert_test) {
    std::cout << "环境1,中间insert测试\n";
    dict.insert(std::make_pair("Hello", "你好"));
    dict.insert(std::make_pair("hello", "你好"));
    dict.insert(std::make_pair("雷吼", "你好"));
    auto it = dict.find("hello");
    ASSERT_NE(it, dict.end());
}

// 保留 PDF 的失败演示：本用例获得新实例，dict 初始大小为 0。
TEST_F(HashTestEnv1, sizeof) {
    std::cout << "环境1,中间size测试\n";
    ASSERT_GT(dict.size(), 0);
}

// 运行套件，失败断言使程序返回 1。
int main(int argc, char* argv[]) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
