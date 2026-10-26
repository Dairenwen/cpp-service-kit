#pragma once

#include <spdlog/spdlog.h>
#include <memory>
#include <string>

namespace bitelog {

// 配置日志的执行方式、过滤等级、输出格式及输出位置。
struct log_settings {
    bool async = false;
    // spdlog 等级：1-debug、2-info、3-warn、4-error、6-off。
    int level = 1;
    std::string format = "[%H:%M:%S][%-7l]: %v";
    // stdout 表示控制台，其余取值作为日志文件路径。
    std::string path = "stdout";
};

extern std::shared_ptr<spdlog::logger> g_logger;

// 根据配置创建全局日志器，并设置日志过滤等级及输出格式。
void bitelog_init(const log_settings& settings = log_settings());

}  // namespace bitelog

// 在日志正文前添加调用位置的文件名和行号。
#define FMT_PREFIX std::string("[{}:{}]: ")
#define DBG(fmt, ...) \
    bitelog::g_logger->debug(FMT_PREFIX + fmt, __FILE__, __LINE__, ##__VA_ARGS__)
#define INF(fmt, ...) \
    bitelog::g_logger->info(FMT_PREFIX + fmt, __FILE__, __LINE__, ##__VA_ARGS__)
#define WRN(fmt, ...) \
    bitelog::g_logger->warn(FMT_PREFIX + fmt, __FILE__, __LINE__, ##__VA_ARGS__)
#define ERR(fmt, ...) \
    bitelog::g_logger->error(FMT_PREFIX + fmt, __FILE__, __LINE__, ##__VA_ARGS__)
