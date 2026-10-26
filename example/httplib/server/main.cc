#include <httplib.h>
#include <iostream>
#include <string>

// 输出请求信息、头字段及查询参数，返回 Hello World 页面。
void HelloWorld(const httplib::Request &req, httplib::Response &rsp)
{
    std::cout << req.method << std::endl;
    std::cout << req.path << std::endl;
    std::cout << req.body << std::endl;
    for (const auto &header : req.headers)
    {
        std::cout << header.first << " = " << header.second << std::endl;
    }
    for (const auto &param : req.params)
    {
        std::cout << param.first << " = " << param.second << std::endl;
    }
    std::string html_body = "<html><body><h1>Hello World</h1></body></html>";
    rsp.set_content(html_body, "text/html");
    rsp.status = 200;
}

// 注册普通路由及正则路由，在 9000 端口提供 HTTP 服务。
int main()
{
    httplib::Server svr;
    svr.Get("/hi", HelloWorld);
    // 正则捕获组从路径中提取数字，matches 同时包含完整匹配。
    svr.Get(R"(/numbers/(\d+))", [](const httplib::Request &req, httplib::Response &)
            {
                std::cout << req.method << std::endl;
                std::cout << req.path << std::endl;
                for (const auto& match : req.matches)
                {
                    std::cout << match << std::endl;
                } });
    svr.listen("0.0.0.0", 8080);
    return 0;
}
