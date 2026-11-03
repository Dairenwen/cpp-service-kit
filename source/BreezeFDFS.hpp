#pragma once
#include <iostream>
#include <vector>
#include <optional>
extern "C"
{
#include <fastdfs/fdfs_client.h>
#include <fastcommon/logger.h>
}

// FastCommon 的 byte 宏会与后续包含的 C++17 标准库头文件冲突。
#ifdef byte
#undef byte
#endif

namespace Breezefdfs
{
    // 客户端全局配置；时间单位均为秒，这里直接采用缓冲区方式初始化 SDK
    struct fdfs_settings
    {
        std::vector<std::string> trackers;        // Tracker 地址列表，例如 dev-tracker:22122。
        int connect_timeout = 30;                 // 连接超时。
        int network_timeout = 30;                 // 网络读写超时。
        bool use_connection_pool = true;          // 默认启用连接池
        int connection_pool_max_idle_time = 3600; // 连接最大空闲时间。
    };

    // 程序开始时初始化一次，操作结束后销毁一次。不要在其他线程仍执行操作时调用 init 或 destroy。
    class FDFSClient
    {
    public:
        // 初始化客户端全局配置
        static void init(const fdfs_settings &settings);
        // 释放客户端全局配置；需等待全部调用完成。
        static void destroy();
        // 上传内存中的完整数据；成功返回文件 ID，失败返回空 optional。
        static std::optional<std::string> upload_from_buff(const std::string &buff);
        // 上传本地文件；成功返回文件 ID，失败返回空 optional。
        static std::optional<std::string> upload_from_file(const std::string &path);
        // 下载到 buff（按字节保存）；成功返回 true。
        static bool download_to_buff(const std::string &file_id, std::string &buff);
        // 下载到本地 path；该路径已有文件时会被覆盖。
        static bool download_to_file(const std::string &file_id, const std::string &path);
        // 按完整文件 ID 删除远端文件。
        static bool remove(const std::string &file_id);
    };
}
