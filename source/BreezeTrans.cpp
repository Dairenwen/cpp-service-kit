#include "BreezeTrans.hpp"
#include "BreezeLog.hpp"
#include "BreezeUtil.hpp"

M3U8Info::M3U8Info(const std::string &filename) : _filename(filename) {}

// 按行读取播放列表，保存头部和每个分片的时长、地址。
bool M3U8Info::parse()
{
    std::string body;
    if (!BreezeUtil::FUTIL::read(_filename, body))
    {
        ERR("M3U8文件读取数据失败: {}", _filename);
        return false;
    }

    std::vector<std::string> lines;
    BreezeUtil::STR::split(body, "\n", lines);
    // 移除 Windows 换行中的回车和空行。
    for (auto &line : lines)
    {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
    }
    lines.erase(std::remove(lines.begin(), lines.end(), ""), lines.end());

    _headers.clear();
    _pieces.clear();
    for (size_t i = 0; i < lines.size(); ++i)
    {
        if (lines[i].find(HLS_ENDLIST) != std::string::npos) // 播放列表结束标签，后续内容忽略
            break;
        if (lines[i].find(HLS_EXTINF) != std::string::npos)
        {
            if (i + 1 >= lines.size())
            {
                ERR("M3U8分片地址缺失: {}", _filename);
                return false;
            }
            _pieces.push_back({lines[i], lines[i + 1]});
            ++i;
            continue;
        }
        _headers.push_back(lines[i]);
    }
    return true;
}

// 将头部、分片和结束标签写回播放列表。
bool M3U8Info::write()
{
    std::stringstream output;
    for (const auto &header : _headers)
        output << header << "\n";
    for (const auto &piece : _pieces)
        output << piece.first << "\n"
               << piece.second << "\n";
    output << HLS_ENDLIST << "\n";
    if (!BreezeUtil::FUTIL::write(_filename, output.str()))
    {
        ERR("M3U8文件写入数据失败: {}", _filename);
        return false;
    }
    return true;
}

// 返回可修改的播放列表头部。
std::vector<std::string> &M3U8Info::headers()
{
    return _headers;
}

// 返回可修改的分片信息。
std::vector<std::pair<std::string, std::string>> &M3U8Info::pieces()
{
    return _pieces;
}

// 将 HLS 配置转换为 FFmpeg 参数字典。
AVDictionary *trans_settings::hls_dictionary() const
{
    AVDictionary *dict = nullptr;
    av_dict_set_int(&dict, "hls_time", _hls_time, 0);
    av_dict_set_int(&dict, "hls_list_size", _hls_list_size, 0);
    av_dict_set(&dict, "hls_base_url", _hls_base_url.c_str(), 0);
    av_dict_set(&dict, "hls_playlist_type", _hls_playlist_type.c_str(), 0);
    av_dict_set(&dict, "hls_flags", _hls_flags.c_str(), 0);
    return dict;
}

// 将 FFmpeg 错误码转换为可读信息。
std::string HLSTranscoder::avError(int error_code)
{
    char message[AV_ERROR_MAX_STRING_SIZE];
    av_strerror(error_code, message, sizeof(message));
    return message;
}

// 复制输入媒体流，生成 HLS 播放列表和分片。
bool HLSTranscoder::transcode(const std::string &input_file, const std::string &output_file, const trans_settings &ts)
{
    AVFormatContext *input = nullptr;
    AVFormatContext *output = nullptr;
    AVDictionary *dict = nullptr;

    // 统一释放 FFmpeg 参数和格式上下文。
    auto cleanup = [&]
    {
        av_dict_free(&dict);
        avformat_free_context(output);
        avformat_close_input(&input);
    };

    // 1. 打开输入文件并读取媒体流信息。
    int ret = avformat_open_input(&input, input_file.c_str(), nullptr, nullptr);
    if (ret < 0)
    {
        ERR("打开输入文件失败: {}", avError(ret));
        return false;
    }
    ret = avformat_find_stream_info(input, nullptr);
    if (ret < 0)
    {
        ERR("解析媒体元信息失败: {}", avError(ret));
        cleanup();
        return false;
    }

    // 2. 创建 HLS 输出上下文和对应的媒体流。
    ret = avformat_alloc_output_context2(&output, nullptr, "hls", output_file.c_str());
    if (ret < 0 || !output)
    {
        ERR("创建输出上下文失败: {}", avError(ret));
        cleanup();
        return false;
    }
    for (unsigned int i = 0; i < input->nb_streams; ++i)
    {
        AVStream *input_stream = input->streams[i];
        AVStream *output_stream = avformat_new_stream(output, nullptr);
        if (!output_stream)
        {
            ERR("创建输出媒体流失败");
            cleanup();
            return false;
        }
        ret = avcodec_parameters_copy(output_stream->codecpar, input_stream->codecpar);
        if (ret < 0)
        {
            ERR("复制编码参数失败: {}", avError(ret));
            cleanup();
            return false;
        }
        // 复制时间基和帧率信息，避免输出分片时间戳错误。
        output_stream->codecpar->codec_tag = 0;
        output_stream->time_base = input_stream->time_base;
        output_stream->avg_frame_rate = input_stream->avg_frame_rate;
        output_stream->r_frame_rate = input_stream->r_frame_rate;
    }

    // 3. 设置 HLS 参数并写入输出头部。
    dict = ts.hls_dictionary();
    ret = avformat_write_header(output, &dict);
    if (ret < 0)
    {
        ERR("输出头部信息失败: {}", avError(ret));
        cleanup();
        return false;
    }

    // 4. 读取数据包，转换时间基后写入输出文件。
    AVPacket packet;
    while ((ret = av_read_frame(input, &packet)) >= 0)
    {
        AVStream *input_stream = input->streams[packet.stream_index];
        AVStream *output_stream = output->streams[packet.stream_index];
        av_packet_rescale_ts(&packet, input_stream->time_base, output_stream->time_base);
        packet.pos = -1;
        ret = av_interleaved_write_frame(output, &packet);
        av_packet_unref(&packet);
        if (ret < 0)
        {
            ERR("输出数据包失败: {}", avError(ret));
            cleanup();
            return false;
        }
    }
    if (ret != AVERROR_EOF)
    {
        ERR("读取数据包失败: {}", avError(ret));
        cleanup();
        return false;
    }

    // 5. 写入输出结尾并释放资源。
    ret = av_write_trailer(output);
    if (ret < 0)
    {
        ERR("输出尾部信息失败: {}", avError(ret));
        cleanup();
        return false;
    }
    cleanup();
    return true;
}
