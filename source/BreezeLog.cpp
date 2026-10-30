#include "BreezeLog.hpp"

#include <memory>
#include <spdlog/async.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace BreezeLog
{
    // 创建独立的异步队列，后台线程按顺序输出日志。
    spdlog::logger &Logger::getLogger()
    {
        static auto logger = []
        {
            auto sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
            static auto pool = std::make_shared<spdlog::details::thread_pool>(8192, 1);
            auto instance = std::make_shared<spdlog::async_logger>(
                "Breeze", sink, pool, spdlog::async_overflow_policy::block);
            instance->set_level(spdlog::level::warn);
            instance->set_pattern("[%Y-%m-%d %H:%M:%S] [%^%l%$] [%s:%#] %v");
            instance->flush_on(spdlog::level::warn);
            return instance;
        }();
        return *logger;
    }

    // 调整共用日志的最低输出级别。
    void Logger::setLevel(spdlog::level::level_enum level)
    {
        getLogger().set_level(level);
    }
}
