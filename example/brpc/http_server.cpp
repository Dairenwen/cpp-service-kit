#include "cal.pb.h"
#include <brpc/closure_guard.h>
#include <brpc/server.h>
#include <iostream>

class CalServiceImpl : public cal::CalService
{
public:
    void PersonHello(google::protobuf::RpcController *controller,
                     const cal::PersonRsp *request, cal::PersonRsp *response,
                     google::protobuf::Closure *done) override
    {
        brpc::ClosureGuard done_guard(done);

        auto *cntl = static_cast<brpc::Controller *>(controller);
        const auto &header = cntl->http_request();
        std::cout << "method: " << brpc::HttpMethod2Str(header.method()) << std::endl;
        std::cout << "uri: " << header.uri() << std::endl;
        std::cout << "content-type: " << header.GetHeader("Content-Type") << std::endl;
        std::cout << "rid: " << request->rid() << std::endl; // 原本的request_attachment() 可能已经为空，应该使用request对象来获取请求参数
        std::cout << "msg: " << request->msg() << std::endl;
        cntl->http_response().set_status_code(brpc::HTTP_STATUS_OK);

        response->set_rid(request->rid());
        response->set_msg(request->msg());
    }
};

int main(int argc, char *argv[])
{
    brpc::Server server;             // 实例化brpc服务器对象
    CalServiceImpl cal_service_impl; // 实例化计算服务对象

    // 将CalServiceImpl服务注册到brpc服务器中
    if (server.AddService(&cal_service_impl, brpc::SERVER_DOESNT_OWN_SERVICE) != 0)
    {
        LOG(ERROR) << "Fail to add service";
        return -1;
    }

    // 启动brpc服务器，监听9000端口
    brpc::ServerOptions options;   // 服务器配置对象
    options.idle_timeout_sec = -1; // 设置服务器空闲超时时间为-1，表示不超时
    if (server.Start(9000, &options) != 0)
    {
        LOG(ERROR) << "Fail to start server";
        return -1;
    }

    // 等待服务器退出
    server.RunUntilAskedToQuit();
    return 0;
}
