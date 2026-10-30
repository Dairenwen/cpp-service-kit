#include "BreezeRPC.hpp"
#include "cal.pb.h"
#include <brpc/closure_guard.h>

// 实现测试协议中的加法方法。
class CalculatorImpl : public breeze_test::Calculator
{
public:
    void Add(google::protobuf::RpcController *controller, const breeze_test::AddRequest *request,
             breeze_test::AddResponse *response, google::protobuf::Closure *done) override
    {
        brpc::ClosureGuard guard(done);
        response->set_result(request->a() + request->b());
    }
};

// 使用系统分配的端口启动测试服务。
std::shared_ptr<brpc::Server> start_test_server()
{
    return RpcServerFactory::create(0, new CalculatorImpl());
}
