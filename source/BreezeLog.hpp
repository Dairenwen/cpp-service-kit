#pragma once

#include <spdlog/spdlog.h>

namespace BreezeLog
{
    // 管理共用的异步终端日志，首次使用时自动创建。
    class Logger
    {
    public:
        // 获取日志对象，可通过 spdlog 接口调整其他设置。
        static spdlog::logger &getLogger();

        // 设置最低输出级别，例如 spdlog::level::info。
        static void setLevel(spdlog::level::level_enum level);
    };
}

// 日志宏记录调用位置，默认输出 WARN 及以上级别。
#define BREEZE_LOG_TRACE(...) SPDLOG_LOGGER_CALL(&::BreezeLog::Logger::getLogger(), ::spdlog::level::trace, __VA_ARGS__)
#define BREEZE_LOG_DEBUG(...) SPDLOG_LOGGER_CALL(&::BreezeLog::Logger::getLogger(), ::spdlog::level::debug, __VA_ARGS__)
#define BREEZE_LOG_INFO(...) SPDLOG_LOGGER_CALL(&::BreezeLog::Logger::getLogger(), ::spdlog::level::info, __VA_ARGS__)
#define BREEZE_LOG_WARN(...) SPDLOG_LOGGER_CALL(&::BreezeLog::Logger::getLogger(), ::spdlog::level::warn, __VA_ARGS__)
#define BREEZE_LOG_ERROR(...) SPDLOG_LOGGER_CALL(&::BreezeLog::Logger::getLogger(), ::spdlog::level::err, __VA_ARGS__)
#define BREEZE_LOG_CRITICAL(...) SPDLOG_LOGGER_CALL(&::BreezeLog::Logger::getLogger(), ::spdlog::level::critical, __VA_ARGS__)

// 保留项目中使用的简短日志宏。
#define DBG(...) BREEZE_LOG_DEBUG(__VA_ARGS__)
#define INF(...) BREEZE_LOG_INFO(__VA_ARGS__)
#define WRN(...) BREEZE_LOG_WARN(__VA_ARGS__)
#define ERR(...) BREEZE_LOG_ERROR(__VA_ARGS__)
