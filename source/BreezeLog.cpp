#include "log.h"

#include <spdlog/async.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace Breezelog
{

    std::shared_ptr<spdlog::logger> g_logger;

    // 按同步或异步方式创建控制台日志器或文件日志器。
    void Breezelog_init(const log_settings &settings)
    {
        if (settings.async)
        {
            if (settings.path == "stdout")
            {
                g_logger = spdlog::stdout_color_mt<spdlog::async_factory>("stdout_logger");
            }
            else
            {
                g_logger = spdlog::basic_logger_mt<spdlog::async_factory>(
                    "file_logger", settings.path);
            }
        }
        else
        {
            if (settings.path == "stdout")
            {
                g_logger = spdlog::stdout_color_mt("stdout_logger");
            }
            else
            {
                g_logger = spdlog::basic_logger_mt("file_logger", settings.path);
            }
        }

        // 统一应用日志等级和正文格式。
        g_logger->set_level(static_cast<spdlog::level::level_enum>(settings.level));
        g_logger->set_pattern(settings.format);
    }

} // namespace bitelog
