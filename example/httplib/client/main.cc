#include <httplib.h>
#include <iostream>

// 向本机的普通请求示例发送带头字段和查询参数的 GET 请求。
int main()
{
    httplib::Client client("127.0.0.1", 8080);
    httplib::Headers headers = {{"Connection", "close"}};
    httplib::Params params = {{"name", "lisi"}, {"age", "18"}};
    auto rsp = client.Get("/hi", params, headers);
    if (rsp && rsp->status == 200)
    {
        std::cout << rsp->body << std::endl;
    }
    return 0;
}
