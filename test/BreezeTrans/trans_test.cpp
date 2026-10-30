#include "BreezeTrans.hpp"

#include <cstdlib>
#include <filesystem>
#include <string>
#include <gtest/gtest.h>

class BreezeTransTest : public testing::Test
{
protected:
    std::filesystem::path directory;

    // 为每个测试创建独立的临时目录。
    void SetUp() override
    {
        char path[] = "/tmp/breeze_trans_XXXXXX";
        char *created = mkdtemp(path);
        ASSERT_NE(created, nullptr);
        directory = created;
    }

    // 测试结束后清理生成的播放列表和分片。
    void TearDown() override
    {
        if (!directory.empty())
            std::filesystem::remove_all(directory);
    }
};

// 写入播放列表，再读取并检查头部和分片信息。
TEST_F(BreezeTransTest, WriteAndReadPlaylist)
{
    const auto filename = (directory / "index.m3u8").string();
    M3U8Info playlist(filename);
    playlist.headers().push_back(HLS_EXTM3U);
    playlist.pieces().push_back({"#EXTINF:5.0,", "first.ts"});
    playlist.pieces().push_back({"#EXTINF:4.0,", "second.ts"});
    ASSERT_TRUE(playlist.write());

    M3U8Info result(filename);
    ASSERT_TRUE(result.parse());
    ASSERT_EQ(result.headers().size(), 1);
    EXPECT_EQ(result.headers()[0], HLS_EXTM3U);
    ASSERT_EQ(result.pieces().size(), 2);
    EXPECT_EQ(result.pieces()[0].first, "#EXTINF:5.0,");
    EXPECT_EQ(result.pieces()[0].second, "first.ts");
    EXPECT_EQ(result.pieces()[1].second, "second.ts");
}

// 使用测试视频生成 HLS，检查播放列表和媒体分片。
TEST_F(BreezeTransTest, CreateHls)
{
    const auto filename = (directory / "index.m3u8").string();
    trans_settings settings;
    settings._hls_time = 5;
    settings._hls_base_url = "/video/";
    ASSERT_TRUE(HLSTranscoder::transcode(BREEZE_TEST_VIDEO, filename, settings));

    M3U8Info playlist(filename);
    ASSERT_TRUE(playlist.parse());
    ASSERT_FALSE(playlist.pieces().empty());
    EXPECT_EQ(playlist.pieces()[0].second.find("/video/"), 0);

    // 播放列表中的地址包含前缀，文件保存在临时目录。
    const auto segment = std::filesystem::path(playlist.pieces()[0].second).filename();
    EXPECT_TRUE(std::filesystem::exists(directory / segment));
}
