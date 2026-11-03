#pragma once
#include <iostream>
#include <fastcommon/logger.h>
#include <fastdfs/fdfs_client.h>

// 这些是配置文本，不是文件路径，还可以直接通过配置字符串初始化客户端
std::string config =
    "connect_timeout = 5\n"
    "network_timeout = 60\n"
    "tracker_server = dev-tracker:22122\n"
    "use_connection_pool = true\n"
    "connection_pool_max_idle_time = 3600\n"
    "base_path = /tmp\n";

// FastCommon 定义的 byte 宏会与 C++17 的 std::byte 冲突。
#ifdef byte
#undef byte
#endif

// 管理一次 SDK 使用所需的日志、全局配置和 Tracker 连接。
class FastDfsClient
{
public:
    FastDfsClient(const char *config)
    {
        // 初始化 FastCommon 的全局日志，再把日志级别设为仅输出错误及更严重的信息，日志无法关闭
        log_init();
        g_log_context.log_level = LOG_ERR;
        // 从指定配置文件初始化 FastDFS 全局客户端配置；返回 0 表示成功。
        int result = fdfs_client_init(config);
        if (result != 0)
        {
            std::cerr << "初始化客户端失败: " << STRERROR(result) << '\n';
            return;
        }
        initialized_ = true;
        // 根据全局配置取得 Tracker 连接；返回 nullptr 表示连接失败。
        tracker_ = tracker_get_connection();
        if (!tracker_)
            std::cerr << "连接 tracker 失败，请检查配置和服务状态\n";
    }
    ~FastDfsClient()
    {
        // 先归还或关闭 Tracker 连接，再销毁客户端全局配置，最后销毁日志。
        if (tracker_)
            tracker_close_connection(tracker_);
        if (initialized_)
            fdfs_client_destroy();
        log_destroy();
    }
    // 禁止复制，避免多个对象重复释放同一个连接和全局配置。
    FastDfsClient(const FastDfsClient &) = delete;
    FastDfsClient &operator=(const FastDfsClient &) = delete;
    ConnectionInfo *tracker() const { return tracker_; }

private:
    bool initialized_ = false;
    ConnectionInfo *tracker_ = nullptr;
};

inline int report_error(const char *operation, int result)
{
    // FastDFS 操作通常返回 0 或错误码；STRERROR 将错误码转换为可读信息。
    std::cerr << operation << "失败: " << STRERROR(result) << " (" << result << ")\n";
    return 1;
}
