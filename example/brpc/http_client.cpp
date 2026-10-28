#include "cal.pb.h"
#include <brpc/channel.h>
#include <brpc/controller.h>

int main(int argc, char *argv[])
{
    brpc::ChannelOptions options;
    options.protocol = brpc::PROTOCOL_HTTP; // 设置协议为http
    options.timeout_ms = 1000;              // 设置超时时间为1000毫秒
    brpc::Channel channel;                  // 实例化brpc通道对象
    // 初始化通道，应用服务器地址、通信协议和超时配置。
    if (channel.Init("127.0.0.1:9000", &options) != 0)
    {
        LOG(ERROR) << "Fail to initialize channel";
        return -1;
    }
    brpc::Controller cntl;
    cntl.http_request().set_method(brpc::HTTP_METHOD_POST);                          // 设置请求方法为POST
    cntl.request_attachment().append("{\"rid\":\"123\",\"msg\":\"Hello, World!\"}"); // 设置请求的JSON数据
    cntl.http_request().uri() = "/CalService/PersonHello";                           // 设置请求的URI，注意这里要设置为服务名/方法名
    cntl.http_request().SetHeader("Content-Type", "application/json");               // 设置请求头，指定内容类型为JSON,会将json请求转换为protobuf请求
    channel.CallMethod(nullptr, &cntl, nullptr, nullptr, nullptr);                   // 发送HTTP请求
    if (cntl.Failed())
    {
        LOG(ERROR) << "RPC failed: " << cntl.ErrorText();
    }
    else
    {
        LOG(INFO) << cntl.response_attachment();
    }
    return 0;
}
