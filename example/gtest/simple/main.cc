#include <gtest/gtest.h>

// 返回整数的绝对值，供断言演示使用。
int abs(int x) {
    return x > 0 ? x : -x;
}

// 使用布尔断言和数值比较断言检查绝对值结果。
TEST(abs_test, test1) {
    ASSERT_TRUE(abs(1) == 1) << "abs(1)=1";
    ASSERT_TRUE(abs(-1) == 1);
    ASSERT_FALSE(abs(-2) == -2);
    ASSERT_EQ(abs(1), abs(-1));
    ASSERT_NE(abs(-1), 0);
    ASSERT_LT(abs(-1), 2);
    ASSERT_GT(abs(-1), 0);
    ASSERT_LE(abs(-1), 2);
    ASSERT_GE(abs(-1), 0);
}

// 初始化 GTest，返回所有测试的执行结果。
int main(int argc, char* argv[]) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
