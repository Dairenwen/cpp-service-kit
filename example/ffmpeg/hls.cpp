// FFmpeg 提供 C 接口，使用 C 链接方式调用。
extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/dict.h>
}
#include <iostream>

// 这里将输入mp4视频文件转换为 HLS（HTTP Live Streaming）格式，生成 m3u8 播放列表和分片文件。
int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        std::cout << "用法: hls 输入视频 输出播放列表.m3u8" << std::endl;
        return 1;
    }

    // 1. 打开输入视频，创建输入格式上下文。
    AVFormatContext *input = nullptr;
    avformat_open_input(&input, argv[1], nullptr, nullptr);

    // 2. 读取视频中的流信息，例如视频流和音频流。
    avformat_find_stream_info(input, nullptr);

    // 3. 创建 HLS 输出上下文，输出文件是 m3u8 播放列表。
    AVFormatContext *output = nullptr;
    avformat_alloc_output_context2(&output, nullptr, "hls", argv[2]);

    // 4. 遍历输入媒体流，为每条输入流创建输出流，复制编码参数和时间基。
    for (unsigned int i = 0; i < input->nb_streams; ++i)
    {
        AVStream *input_stream = input->streams[i];
        AVStream *output_stream = avformat_new_stream(output, nullptr);
        avcodec_parameters_copy(output_stream->codecpar, input_stream->codecpar); // 复制编码参数
        output_stream->codecpar->codec_tag = 0;                                   // 清除编码标签，避免格式不兼容
        output_stream->time_base = input_stream->time_base;                       // 复制时间基，之后输出封装器可能根据输出格式调整输出流的时间基
    }

    // 5. 设置点播列表、五秒分片和播放列表中的分片地址前缀。
    AVDictionary *options = nullptr;
    av_dict_set(&options, "hls_playlist_type", "vod", 0);                          // 设置点播列表类型为 VOD（Video on Demand）
    av_dict_set(&options, "hls_time", "5", 0);                                     // 设置每个分片的时长为 5 秒
    av_dict_set(&options, "hls_base_url", "http://192.168.65.130:9000/video/", 0); // 设置播放列表中分片的地址前缀，客户端在播放时会根据这个前缀请求分片文件

    // 6. 写入输出头部，准备生成播放列表和分片。
    avformat_write_header(output, &options);

    // 7. 逐个读取编码数据包，将时间戳转换为输出流的时间基。
    AVPacket *packet = av_packet_alloc();
    while (av_read_frame(input, packet) >= 0)
    {
        AVStream *input_stream = input->streams[packet->stream_index];
        AVStream *output_stream = output->streams[packet->stream_index];
        // 将时间戳从输入流时间基转换为输出流时间基。
        av_packet_rescale_ts(packet, input_stream->time_base, output_stream->time_base);
        // 设置数据包位置为 -1，表示数据包没有特定位置。
        packet->pos = -1;

        // 8. 将数据包写入 HLS 输出，由封装器生成分片。
        av_interleaved_write_frame(output, packet);

        // 9. 释放当前数据包引用，准备读取下一个数据包。
        av_packet_unref(packet);
    }

    // 10. 写入结尾并释放数据包、参数字典和格式上下文。
    av_write_trailer(output);
    av_packet_free(&packet);
    av_dict_free(&options);
    avformat_close_input(&input);
    avformat_free_context(output);
    return 0;
}
