#include "client.hpp"
#include <fstream>
#include <iterator>
#include <string>

int main(int argc, char *argv[])
{
    // 参数依次为配置文件、上传方式和本地文件路径。
    if (argc != 4 || (std::string(argv[2]) != "file" && std::string(argv[2]) != "buffer"))
    {
        std::cerr << "用法: " << argv[0] << " <配置文件> <file|buffer> <本地文件>\n";
        return 1;
    }
    // 初始化客户端并连接 Tracker；对象析构时自动清理资源。
    FastDfsClient client(argv[1]);
    if (!client.tracker())
        return 1;
    // SDK 将包含 group 的文件 ID 写入此缓冲区，后续下载和删除需要它。
    char file_id[256] = {};
    int result;
    if (std::string(argv[2]) == "file") // 文件直接上传，适合较大文件，SDK 内部会按块上传。
    {
        // 按文件名上传：Tracker、Storage 连接（nullptr 表示由 SDK 获取）、
        // 存储路径索引、本地文件名、扩展名（nullptr 从文件名提取）、
        // 元数据及数量（此处不设置）、group（nullptr 不指定）、输出文件 ID。
        result = storage_upload_by_filename1(client.tracker(), nullptr, 0, argv[3], nullptr, nullptr, 0, nullptr, file_id);
    }
    else
    {
        // buffer 模式先以二进制方式将整个文件读入内存，适合较小的文件。
        std::ifstream input(argv[3], std::ios::binary);
        if (!input)
        {
            std::cerr << "无法打开文件: " << argv[3] << '\n';
            return 1;
        }
        std::string data((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
        if (input.bad())
        {
            std::cerr << "读取文件失败: " << argv[3] << '\n';
            return 1;
        }
        // 按缓冲区上传：依次传入 Tracker、自动选择的 Storage、路径索引、
        // 数据地址与字节数、扩展名、元数据及数量、group 和输出文件 ID。
        // 扩展名和 group 为 nullptr 表示不指定，元数据数量为 0。
        result = storage_upload_by_filebuff1(client.tracker(), nullptr, 0, data.data(), data.size(), nullptr, nullptr, 0, nullptr, file_id);
    }
    // SDK 返回 0 表示成功；标准输出只打印文件 ID，便于脚本接收。
    if (result != 0)
        return report_error("上传", result);
    std::cout << file_id << '\n';
}
