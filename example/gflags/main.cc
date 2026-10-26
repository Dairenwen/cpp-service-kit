#include <gflags/gflags.h>
#include <iostream>

// 参数名同时用于命令行选项和 flagfile 配置项。
DEFINE_bool(reuse_addr, true, "启用网络地址重用选项");
DEFINE_int32(log_level, 1, "日志等级：1-DEBUG, 2-WARN, 3-ERROR");
DEFINE_string(log_file, "stdout", "日志输出位置，默认为标准输出");

// 解析命令行及配置文件，输出三个参数的最终取值。
int main(int argc, char* argv[]) {
    google::ParseCommandLineFlags(&argc, &argv, true);
    std::cout << "reuse: " << FLAGS_reuse_addr << std::endl;
    std::cout << "level: " << FLAGS_log_level << std::endl;
    std::cout << "file: " << FLAGS_log_file << std::endl;
    return 0;
}
