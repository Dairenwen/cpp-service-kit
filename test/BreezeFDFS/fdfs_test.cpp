#include <gtest/gtest.h>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "BreezeFDFS.hpp"

namespace
{

    // 每个测试进程只初始化和销毁一次 FastDFS 的全局配置。
    class FdfsEnvironment : public ::testing::Environment
    {
    public:
        void SetUp() override
        {
            Breezefdfs::fdfs_settings settings;
            settings.trackers = {"dev-tracker:22122"};
            Breezefdfs::FDFSClient::init(settings);
        }

        void TearDown() override
        {
            Breezefdfs::FDFSClient::destroy();
        }
    };

    class FdfsTest : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            // 为每个测试创建独立的临时目录，避免覆盖已有文件。
            char path[] = "/tmp/breeze_fdfs_test_XXXXXX";
            char *directory = mkdtemp(path);
            ASSERT_NE(directory, nullptr);
            directory_ = directory;
        }

        void TearDown() override
        {
            // 即使中途断言失败，也尝试删除本次测试创建的远端文件和本地文件。
            for (const auto &id : uploaded_ids_)
            {
                EXPECT_TRUE(Breezefdfs::FDFSClient::remove(id)) << id;
            }
            if (!directory_.empty())
                std::filesystem::remove_all(directory_);
        }

        std::string path(const char *name) const
        {
            return (directory_ / name).string();
        }

        std::filesystem::path directory_;
        std::vector<std::string> uploaded_ids_;
    };

    // 从文件上传，下载到文件及内存，验证两种结果均与原始字节相同。
    TEST_F(FdfsTest, UploadFileAndDownload)
    {
        const std::string original("FastDFS 文件测试\n二进制:\0END", sizeof("FastDFS 文件测试\n二进制:\0END") - 1);
        const std::string input_path = path("input.dat");
        const std::string output_path = path("output.dat");
        {
            std::ofstream output(input_path, std::ios::binary);
            ASSERT_TRUE(output);
            output.write(original.data(), original.size());
            output.close();
            ASSERT_TRUE(output);
        }
        auto id = Breezefdfs::FDFSClient::upload_from_file(input_path);
        ASSERT_TRUE(id.has_value());
        uploaded_ids_.push_back(*id);

        ASSERT_TRUE(Breezefdfs::FDFSClient::download_to_file(*id, output_path));
        std::ifstream input(output_path, std::ios::binary);
        ASSERT_TRUE(input);
        const std::string downloaded((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
        EXPECT_EQ(downloaded, original);

        std::string buffer;
        ASSERT_TRUE(Breezefdfs::FDFSClient::download_to_buff(*id, buffer));
        EXPECT_EQ(buffer, original);
    }

    // 从内存上传二进制数据，下载回内存和文件，最后验证删除接口。
    TEST_F(FdfsTest, UploadBufferDownloadAndRemove)
    {
        const std::string original("buffer data\0with binary bytes\n", sizeof("buffer data\0with binary bytes\n") - 1);
        auto id = Breezefdfs::FDFSClient::upload_from_buff(original);
        ASSERT_TRUE(id.has_value());
        uploaded_ids_.push_back(*id);

        std::string downloaded;
        ASSERT_TRUE(Breezefdfs::FDFSClient::download_to_buff(*id, downloaded));
        EXPECT_EQ(downloaded, original);

        const std::string output_path = path("buffer.dat");
        ASSERT_TRUE(Breezefdfs::FDFSClient::download_to_file(*id, output_path));
        std::ifstream input(output_path, std::ios::binary);
        ASSERT_TRUE(input);
        const std::string from_file((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
        EXPECT_EQ(from_file, original);

        ASSERT_TRUE(Breezefdfs::FDFSClient::remove(*id));
        uploaded_ids_.clear(); // 已删除，避免测试结束时重复删除。
        EXPECT_FALSE(Breezefdfs::FDFSClient::download_to_buff(*id, downloaded));
    }

} // namespace

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new FdfsEnvironment);
    return RUN_ALL_TESTS();
}
