#include "cal.pb.h"
#include <brpc/server.h>
#include <brpc/closure_guard.h>

class CalServiceImpl : public cal::CalService
{
public:
    CalServiceImpl() {}
    ~CalServiceImpl() {}
    // 通过继承cal::CalService类，重写Add方法，实现加法功能
    void Add(google::protobuf::RpcController *controller,
             const cal::AddReq *request, cal::AddRsp *response,
             google::protobuf::Closure *done) override
    {
        brpc::ClosureGuard done_guard(done); // 使用RAII思想，自动调用done进行资源释放
        int32_t a = request->num1();
        int32_t b = request->num2();
        response->set_result(a + b);
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
