#include <spdlog/async.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>

// 通过单个后台线程将一百条格式化日志写入文件。
void async_example() {
    // 队列容量为 32768，后台工作线程数量为 1。
    spdlog::init_thread_pool(32768, 1);
    auto async_logger = spdlog::basic_logger_mt<spdlog::async_factory>(
        "async_file_logger", "logs/async_log.txt");
    async_logger->set_pattern("%Y-%m-%d %H:%M:%S [%l] %v");
    for (int i = 1; i < 101; ++i) {
        // 花括号占位符按照参数顺序完成格式化。
        async_logger->info("Async message #{} {}", i, "hello");
    }
}

// 执行异步日志演示，退出时等待后台日志写入完成。
int main() {
    async_example();
    spdlog::shutdown();
    return 0;
}
