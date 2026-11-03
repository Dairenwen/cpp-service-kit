#include "client.hpp"

int main(int argc, char *argv[])
{
    // 参数依次为客户端配置文件和要删除的完整文件 ID。
    if (argc != 3)
    {
        std::cerr << "用法: " << argv[0] << " <配置文件> <文件ID>\n";
        return 1;
    }
    // 连接 Tracker，结束时由 FastDfsClient 释放连接和全局配置。
    FastDfsClient client(argv[1]);
    if (!client.tracker())
        return 1;
    // 依次传入 Tracker、Storage（nullptr 表示由 SDK 获取）和文件 ID；
    // 返回 0 表示删除成功，非 0 为错误码。
    int result = storage_delete_file1(client.tracker(), nullptr, argv[2]);
    if (result != 0)
        return report_error("删除", result);
    std::cout << "删除成功: " << argv[2] << '\n';
}
