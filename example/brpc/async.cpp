#include "cal.pb.h"
#include <brpc/channel.h>
#include <brpc/controller.h>

void OnAddDone(brpc::Controller *cntl, cal::AddReq *request, cal::AddRsp *response)
{
    if (cntl->Failed()) // 可以传入request和response指针，方便在回调函数中使用
    {
        LOG(ERROR) << "RPC failed: " << cntl->ErrorText();
    }
    else
    {
        LOG(INFO) << "10 + 20 = " << response->result(); // 输出加法结果
    }
}
int main(int argc, char *argv[])
{
    brpc::ChannelOptions options;   // 实例化brpc通道配置对象
    options.protocol = "baidu_std"; // 设置协议为baidu_std
    options.timeout_ms = 1000;      // 设置超时时间为1000毫秒
    brpc::Channel channel;          // 实例化brpc通道对象
    // 初始化通道，应用服务器地址、通信协议和超时配置。
    if (channel.Init("127.0.0.1:9000", &options) != 0)
    {
        LOG(ERROR) << "Fail to initialize channel";
        return -1;
    }
    cal::CalService_Stub stub(&channel); // 实例化计算服务的客户端
    brpc::Controller cntl;               // 实例化brpc控制器对象
    cal::AddReq request;                 // 实例化加法请求对象
    request.set_num1(10);                // 设置加数1为10
    request.set_num2(20);                // 设置加数2为20
    cal::AddRsp response;                // 实例化加法响应对象
    // 调用加法服务的Add方法，发送请求并等待响应,采用异步阻塞方式,服务端处理完请求后会调用OnAddDone回调函数
    google::protobuf::Closure *done = brpc::NewCallback(OnAddDone, &cntl, &request, &response);
    stub.Add(&cntl, &request, &response, done);
    std::cout << "continue doing other things while waiting for the response..." << std::endl;
    return 0;
}
