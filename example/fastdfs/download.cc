#include "client.hpp"
#include <cstdlib>
#include <fstream>
#include <memory>
#include <string>

int main(int argc, char *argv[])
{
    // 参数依次为配置文件、下载方式、远端文件 ID 和本地输出路径。
    if (argc != 5 || (std::string(argv[2]) != "file" && std::string(argv[2]) != "buffer"))
    {
        std::cerr << "用法: " << argv[0] << " <配置文件> <file|buffer> <文件ID> <输出文件>\n";
        return 1;
    }
    // 初始化全局配置并取得 Tracker 连接；退出作用域时自动清理。
    FastDfsClient client(argv[1]);
    if (!client.tracker())
        return 1;
    int64_t size = 0;
    int result;
    if (std::string(argv[2]) == "file")
    {
        // 直接下载到文件：Tracker、Storage（nullptr 表示由 SDK 获取）、
        // 文件 ID、输出路径、接收下载字节数的变量。输出文件会被覆盖。
        result = storage_download_file_to_file1(client.tracker(), nullptr, argv[3], argv[4], &size);
    }
    else
    {
        // 下载到 SDK 分配的内存：传入 Tracker、自动选择的 Storage、文件 ID，以及接收缓冲区地址和文件字节数的指针。
        char *buffer = nullptr;
        result = storage_download_file1(client.tracker(), nullptr, argv[3], &buffer, &size);
        // SDK 返回的缓冲区需要用 free 释放；智能指针确保错误路径也能释放
        std::unique_ptr<char, decltype(&std::free)> data(buffer, &std::free);
        if (result != 0)
            return report_error("下载", result);
        // 把内存中的内容写到本地文件
        std::ofstream output(argv[4], std::ios::binary);
        if (output && size > 0)
            output.write(data.get(), size);
        output.close();
        if (!output)
        {
            std::cerr << "写入文件失败: " << argv[4] << '\n';
            return 1;
        }
    }
    // FastDFS 接口返回 0 表示成功，非 0 为错误码。
    if (result != 0)
        return report_error("下载", result);
    std::cout << "下载成功: " << size << " 字节\n";
}
