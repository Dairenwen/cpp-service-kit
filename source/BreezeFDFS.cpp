#include <sstream>
#include "BreezeLog.hpp"
#include "BreezeFDFS.hpp"

namespace Breezefdfs
{
    void FDFSClient::init(const fdfs_settings &settings)
    {
        // log_init 会重置日志级别，必须先初始化，再设置级别。
        log_init();
        g_log_context.log_level = LOG_ERR;
        // 将配置项拼成 INI 格式的内存字符串；每个 Tracker 对应一行。
        std::stringstream ss;
        for (auto &tracker : settings.trackers)
        {
            ss << "tracker_server = " << tracker << "\n";
        }
        ss << "connect_timeout = " << settings.connect_timeout << "\n";
        ss << "network_timeout = " << settings.network_timeout << "\n";
        ss << "use_connection_pool = " << settings.use_connection_pool << "\n";
        ss << "connection_pool_max_idle_time = " << settings.connection_pool_max_idle_time << "\n";
        // 从字符串而不是配置文件初始化 SDK 的全局客户端配置。
        int ret = fdfs_client_init_from_buffer(ss.str().c_str());
        if (ret != 0)
        {
            ERR("初始化全局配置失败: {}", STRERROR(ret));
            abort();
        }
    }
    void FDFSClient::destroy()
    {
        // 必须在所有上传、下载和删除操作完成后销毁全局客户端配置。
        fdfs_client_destroy();
    }
    std::optional<std::string> FDFSClient::upload_from_buff(const std::string &buff)
    {
        // 从 SDK 获取 Tracker 连接；开启连接池时可以复用空闲连接。
        auto tracker_server = tracker_get_connection();
        if (tracker_server == nullptr)
        {
            ERR("获取tracker服务器连接句柄失败");
            return std::optional<std::string>();
        }
        // 上传内存数据，SDK 将生成的完整文件 ID 写入 file_id。
        char file_id[256];
        // nullptr 表示由 SDK 获取 Storage，不指定扩展名、元数据和 group。
        int ret = storage_upload_by_filebuff1(tracker_server, nullptr, 0, buff.c_str(), buff.size(), nullptr, nullptr, 0, nullptr, file_id);
        if (ret != 0)
        {
            ERR("文件上传失败: {}", STRERROR(ret));
            // 操作失败时也要归还或关闭已经获取的 Tracker 连接。
            tracker_close_connection(tracker_server);
            return std::optional<std::string>();
        }

        // 操作完成后关闭或归还连接池中的 Tracker 连接。
        tracker_close_connection(tracker_server);
        return std::string(file_id);
    }
    std::optional<std::string> FDFSClient::upload_from_file(const std::string &path)
    {
        // 从 SDK 获取 Tracker 连接；开启连接池时可以复用空闲连接。
        auto tracker_server = tracker_get_connection();
        if (tracker_server == nullptr)
        {
            ERR("获取tracker服务器连接句柄失败");
            return std::optional<std::string>();
        }
        // 上传本地路径指定的文件；SDK 将完整文件 ID 写入 file_id。
        char file_id[256];
        // nullptr 表示由 SDK 获取 Storage，并使用默认扩展名和 group。
        int ret = storage_upload_by_filename1(tracker_server, nullptr, 0, path.c_str(), nullptr, nullptr, 0, nullptr, file_id);
        if (ret != 0)
        {
            ERR("文件上传失败: {}", STRERROR(ret));
            // 操作失败时也要归还或关闭已经获取的 Tracker 连接。
            tracker_close_connection(tracker_server);
            return std::optional<std::string>();
        }
        // 操作完成后关闭或归还连接池中的 Tracker 连接。
        tracker_close_connection(tracker_server);
        return std::string(file_id);
    }
    bool FDFSClient::download_to_buff(const std::string &file_id, std::string &buff)
    {
        // 从 SDK 获取 Tracker 连接；开启连接池时可以复用空闲连接。
        auto tracker_server = tracker_get_connection();
        if (tracker_server == nullptr)
        {
            ERR("获取tracker服务器连接句柄失败");
            return false;
        }
        // SDK 分配下载缓冲区，并通过这两个输出参数返回地址和字节数。
        int64_t file_size;
        char *file_buff = nullptr;
        int ret = storage_download_file1(tracker_server, nullptr, file_id.c_str(), &file_buff, &file_size);
        if (ret != 0)
        {
            ERR("文件下载失败: {}", STRERROR(ret));
            // 失败路径同样需要释放 Tracker 连接
            tracker_close_connection(tracker_server);
            return false;
        }
        // 按明确长度复制，文件中包含二进制零字节也不会被截断。
        buff.assign(file_buff, file_size);
        // storage_download_file1 返回的缓冲区由调用方用 free 释放。
        free(file_buff);
        // 操作完成后关闭或归还连接池中的 Tracker 连接。
        tracker_close_connection(tracker_server);
        return true;
    }
    bool FDFSClient::download_to_file(const std::string &file_id, const std::string &path)
    {
        // 从 SDK 获取 Tracker 连接；开启连接池时可以复用空闲连接。
        auto tracker_server = tracker_get_connection();
        if (tracker_server == nullptr)
        {
            ERR("获取tracker服务器连接句柄失败");
            return false;
        }
        // 直接下载到指定本地路径；file_size 接收下载的字节数。
        int64_t file_size;
        int ret = storage_download_file_to_file1(tracker_server, nullptr, file_id.c_str(), path.c_str(), &file_size);
        if (ret != 0)
        {
            ERR("文件下载失败: {}", STRERROR(ret));
            // 失败路径同样需要释放 Tracker 连接。
            tracker_close_connection(tracker_server);
            return false;
        }
        // 操作完成后关闭或归还连接池中的 Tracker 连接。
        tracker_close_connection(tracker_server);
        return true;
    }
    bool FDFSClient::remove(const std::string &file_id)
    {
        // 从 SDK 获取 Tracker 连接；开启连接池时可以复用空闲连接。
        auto tracker_server = tracker_get_connection();
        if (tracker_server == nullptr)
        {
            ERR("获取tracker服务器连接句柄失败");
            return false;
        }
        // 按完整文件 ID 删除；nullptr 表示由 SDK 获取 Storage 连接。
        int ret = storage_delete_file1(tracker_server, nullptr, file_id.c_str());
        if (ret != 0)
        {
            ERR("文件删除失败: {}", STRERROR(ret));
            // 失败路径同样需要释放 Tracker 连接。
            tracker_close_connection(tracker_server);
            return false;
        }
        // 操作完成后关闭或归还连接池中的 Tracker 连接。
        tracker_close_connection(tracker_server);
        return true;
    }
}
