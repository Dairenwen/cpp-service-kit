#pragma once
extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavutil/error.h>
}
#include <memory>
#include <string>
#include <algorithm>
#include <sstream>
#include <utility>
#include <vector>

// M3U8 播放列表使用的标签。
const std::string HLS_EXTM3U = "#EXTM3U";
const std::string HLS_VERSION = "#EXT-X-VERSION:";
const std::string HLS_TARGETDURATION = "#EXT-X-TARGETDURATION:";
const std::string HLS_SEQUENCE = "#EXT-X-MEDIA-SEQUENCE:";
const std::string HLS_PLAYLIST_TYPE = "#EXT-X-PLAYLIST-TYPE:";
const std::string HLS_INDEPENDENT_SEGMENTS = "#EXT-X-INDEPENDENT-SEGMENTS";
const std::string HLS_ENDLIST = "#EXT-X-ENDLIST";
const std::string HLS_EXTINF = "#EXTINF:";

// 读取和写入播放列表中的头部及分片信息
class M3U8Info
{
public:
    using ptr = std::shared_ptr<M3U8Info>;

    M3U8Info(const std::string &filename);
    bool parse();                        // 读取M3U8文件，保存头部和分片信息
    bool write();                        // 将头部、分片和结束标签写回M3U8文件
    std::vector<std::string> &headers(); //
    std::vector<std::pair<std::string, std::string>> &pieces();

private:
    std::string _filename;                                    // 保存 M3U8 文件路径
    std::vector<std::string> _headers;                        // 保存播放列表头部信息
    std::vector<std::pair<std::string, std::string>> _pieces; // 保存媒体分片信息
};

// HLS 点播列表和分片的配置。
struct trans_settings
{
    int _hls_time = 10;                              // 每个分片的时长，单位秒
    int _hls_list_size = 0;                          // 播放列表中分片的最大数量，0 表示无限制
    std::string _hls_base_url;                       // 分片的 URL 前缀，通常为 CDN 地址
    std::string _hls_playlist_type = "vod";          // 播放列表类型，"vod" 表示点播，"event" 表示事件直播
    std::string _hls_flags = "independent_segments"; // 声明各分片可以独立解码

    // 创建 FFmpeg 参数字典，使用完毕后由调用方释放。
    AVDictionary *hls_dictionary() const;
};

// 将输入媒体流重新封装为 HLS 播放列表和分片。
class HLSTranscoder
{
public:
    // 执行转码操作，返回是否成功。
    static bool transcode(const std::string &input_file, const std::string &output_file, const trans_settings &ts);

private:
    // 将 FFmpeg 错误码转换为可读信息。
    static std::string avError(int error_code);
};
