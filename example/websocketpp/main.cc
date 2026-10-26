#include <websocketpp/config/asio_no_tls.hpp> //不启用https，使用普通 WebSocket 的 Asio 配置
#include <websocketpp/server.hpp>
#include <iostream>
#include <sstream>

// 提供完整的异步 TCP 网络通信，还可以使用其他iostream、stub等配置
typedef websocketpp::server<websocketpp::config::asio> websocketsvr; // 创建使用 Asio、Http 的 WebSocket 服务器类型
typedef websocketsvr::message_ptr message_ptr;

using websocketpp::lib::bind;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;

// WebSocket 握手成功后输出连接建立提示。
void OnOpen(websocketsvr *server, websocketpp::connection_hdl hdl)
{
    std::cout << "连接成功" << std::endl;
}

// WebSocket 连接关闭后输出关闭提示。
void OnClose(websocketsvr *server, websocketpp::connection_hdl hdl)
{
    std::cout << "连接关闭" << std::endl;
}

// 读取收到的消息，并向同一个连接发送内容相同的文本消息。
void OnMessage(websocketsvr *server, websocketpp::connection_hdl hdl, message_ptr msg)
{
    std::cout << "收到消息" << msg->get_payload() << std::endl;
    // hdl 标识当前连接，get_payload 获取正文，text 指定发送文本帧。
    server->send(hdl, msg->get_payload(), websocketpp::frame::opcode::text);
}

// 连接建立失败时输出异常提示。
void OnFail(websocketsvr *server, websocketpp::connection_hdl hdl)
{
    std::cout << "连接异常" << std::endl;
}

// 处理普通 HTTP 请求，返回状态码为 200 的 HTML 欢迎页面。
void OnHttp(websocketsvr *server, websocketpp::connection_hdl hdl)
{
    std::cout << "处理http请求" << std::endl;
    // 通过连接句柄取得连接对象，再设置该连接的 HTTP 响应。
    websocketsvr::connection_ptr con = server->get_con_from_hdl(hdl);
    std::stringstream ss;
    ss << "<!doctype html><html><head>"
       << "<title>hello websocket</title></head><body>"
       << "<h1>hello websocketpp</h1>"
       << "</body></html>";
    con->set_body(ss.str());
    con->set_status(websocketpp::http::status_code::ok);
}

// 注册 HTTP 和 WebSocket 事件回调，在 8888 端口运行服务器。
int main()
{
    websocketsvr server;
    // 关闭 WebSocket++ 的日志输出，但是保留错误日志，避免输出大量调试信息，websocket有自己的日志系统
    server.clear_access_channels(websocketpp::log::alevel::all);
    // 初始化 Asio 的事件调度器，为网络监听和消息处理准备运行环境。
    server.init_asio();

    // bind 固定服务器指针，_1 和 _2 分别接收事件传入的连接句柄与消息。
    server.set_http_handler(bind(&OnHttp, &server, _1));
    server.set_open_handler(bind(&OnOpen, &server, _1));
    server.set_close_handler(bind(&OnClose, &server, _1));
    server.set_message_handler(bind(&OnMessage, &server, _1, _2));
    server.set_fail_handler(bind(&OnFail, &server, _1));

    // 绑定监听端口，并启动接收 TCP 连接的异步操作。
    server.listen(9000);
    server.start_accept();
    // 进入事件循环，持续处理连接、HTTP 请求和 WebSocket 消息。
    server.run();
    return 0;
}
