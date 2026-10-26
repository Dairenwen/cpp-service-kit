#include "log.h"

#include <gflags/gflags.h>

DEFINE_bool(log_async, true, "启用异步日志");
DEFINE_int32(log_level, 1, "日志输出等级：1-debug;2-info;3-warn;4-error;6-off");
DEFINE_string(log_format, "[%H:%M:%S][%-7l]%v", "日志输出格式");
DEFINE_string(log_path, "stdout", "日志输出路径");

// 将命令行配置传给日志封装，并演示四种等级及空可变参数调用。
int main(int argc, char* argv[]) {
    google::ParseCommandLineFlags(&argc, &argv, true);
    bitelog::log_settings settings{
        FLAGS_log_async, FLAGS_log_level, FLAGS_log_format, FLAGS_log_path};
    bitelog::bitelog_init(settings);

    DBG("{}今年{}岁", "小明", 18);
    INF("{}今年{}岁", "小红", 19);
    WRN("{}今年{}岁", "小刚", 20);
    ERR("{}今年{}岁", "小李", 21);
    DBG("HELLO");
    // 关闭日志系统时完成异步队列中的写入任务。
    spdlog::shutdown();
    return 0;
}
