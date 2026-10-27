# WebSocketpp 快速上手

## 安装

Ubuntu 开发包为 `libboost-dev`、`libboost-system-dev`、`libwebsocketpp-dev`。PDF 使用 `websocketpp/config/asio_no_tls.hpp` 和 `websocketpp/server.hpp`，链接 Boost.System 与线程库。

## WebSocket 协议

WebSocket 在 TCP 连接上进行双向消息传输，适合即时消息和服务器推送。连接建立后，双方都可以主动发送消息；HTTP 轮询则由客户端周期性发起请求。

握手从 HTTP 请求开始，使用 `Upgrade: websocket`、`Connection: Upgrade`、`Sec-WebSocket-Key` 和版本号等头字段。服务端以 `101 Switching Protocols` 和 `Sec-WebSocket-Accept` 确认升级，后续数据采用 WebSocket 帧格式。

### 帧格式

| 字段               | 含义                                                       |
| ------------------ | ---------------------------------------------------------- |
| `FIN`            | `1` 表示消息的末尾帧，`0` 表示后续还有分片             |
| `RSV1`～`RSV3` | 普通帧取 `0`，扩展协议协商后按扩展定义使用               |
| `opcode`         | 标识文本、二进制、关闭等帧类型                             |
| `mask`           | 客户端发往服务端的帧取 `1`，服务端发往客户端的帧取 `0` |
| `Payload length` | 指示正文长度及扩展长度字段                                 |
| `Mask-Key`       | 4 字节掩码密钥，用于还原载荷                               |
| `Payload data`   | 帧携带的内容                                               |

长度标记为 `0`～`125` 时直接表示字节数；标记为 `126` 时，后续 2 字节表示长度；标记为 `127` 时，后续 8 字节表示长度，其最高位取 `0`。从 `0` 开始计数，载荷第 `i` 个字节与密钥第 `i % 4` 个字节异或即可还原数据。

补充：RSV 与长度边界采用 [RFC 6455 的帧格式定义](https://datatracker.ietf.org/doc/html/rfc6455#section-5.2)。

| opcode                             | 类型              |
| ---------------------------------- | ----------------- |
| `0x0`                            | 消息后续分片      |
| `0x1`、`0x2`                   | 文本帧、二进制帧  |
| `0x8`                            | 关闭帧            |
| `0x9`、`0xA`                   | Ping、Pong 心跳帧 |
| `0x3`～`0x7`、`0xB`～`0xF` | 为扩展保留的类型  |

## WebSocketpp 与配置

WebSocketpp 是跨平台的头文件库，采用事件回调接口，支持普通 HTTP 请求和 WebSocket 连接。PDF 通过 Boost.Asio 提供异步网络通信。

| 服务器配置                                 | 补充：适用场景                                            |
| ------------------------------------------ | --------------------------------------------------------- |
| `config::asio`，配合 `asio_no_tls.hpp` | 普通 HTTP 与 `ws` 联调                                  |
| `config::asio_tls`，配合 `asio.hpp`    | HTTPS 与 `wss`，同时配置 OpenSSL、证书及 TLS 初始化回调 |

配置模板决定网络传输和日志器类型。[配置说明](https://docs.websocketpp.org/reference_8config.html)

## 常用服务器接口

`websocketpp::server` 接收配置类型。PDF 中的 `websocketsvr` 是服务器类型别名，`message_ptr` 是消息指针类型。

| 接口                                  | 用途                                 |
| ------------------------------------- | ------------------------------------ |
| `init_asio`                         | 初始化 Asio 事件调度器               |
| `set_http_handler`                  | 注册普通 HTTP 请求回调               |
| `set_open_handler`                  | 注册 WebSocket 握手成功回调          |
| `set_close_handler`                 | 注册连接关闭回调                     |
| `set_message_handler`               | 注册消息接收回调                     |
| `set_fail_handler`                  | 注册连接建立失败回调                 |
| `listen`、`start_accept`、`run` | 绑定端口、开始接收连接、进入事件循环 |
| `send`                              | 通过连接句柄发送指定类型的消息       |
| `close`                             | 发送关闭状态码与原因，结束连接       |
| `set_timer`                         | 以毫秒为单位安排定时回调             |

补充：`set_reuse_addr(true)` 启用地址复用，便于频繁重启服务；`false` 保留默认绑定行为，适合沿用系统默认地址回收规则的部署。

`bind` 可固定服务器指针，`_1` 接收事件传入的连接句柄，消息回调中的 `_2` 接收消息对象。

## 日志通道

访问日志使用 `alevel` 位掩码，可以组合多个通道。`set_access_channels` 开启通道，`clear_access_channels` 清除通道。

| 通道                                    | 内容                                  |
| --------------------------------------- | ------------------------------------- |
| `connect`、`disconnect`             | 建立与关闭连接                        |
| `control`                             | 控制帧                                |
| `frame_header`、`frame_payload`     | 帧头与载荷                            |
| `message_header`、`message_payload` | 消息头与正文                          |
| `debug_handshake`、`debug_close`    | 握手与关闭过程                        |
| `endpoint`、`devel`、`app`        | 端点、库调试和应用日志                |
| `http`、`fail`                      | HTTP 请求和连接失败                   |
| `access_core`                         | 连接建立、关闭、HTTP 和失败通道的组合 |
| `all`、`none`                       | 全部访问日志、关闭访问日志            |

示例选择 `none`，适合终端主要显示业务回调输出的场景；协议排查时可开启 `all` 或相关通道。补充：访问日志与错误日志分别配置，清除访问日志后，错误日志仍按其通道设置输出。

## 连接对象与 HTTP 接口

`connection_hdl` 是弱引用句柄，适合在回调之间标识连接；通过 `get_con_from_hdl` 取得 `connection_ptr`，可读取请求并设置响应。

| 接口                                            | 用途                                       |
| ----------------------------------------------- | ------------------------------------------ |
| `get_opcode`、`get_payload`                 | 从消息对象取得帧类型及消息正文             |
| `get_request_header`、`get_request_body`    | 读取请求头字段和正文                       |
| `get_request`                                 | 取得完整 HTTP 请求对象                     |
| `get_method`、`get_uri`                     | 从请求对象取得方法和地址                   |
| `get_header`、`get_headers`、`get_body`   | 从请求对象读取指定头字段、全部头字段和正文 |
| `set_status`、`set_body`、`append_header` | 设置 HTTP 状态码、正文和头字段             |
| `get_handle`                                  | 从连接对象取得连接句柄                     |

`send` 可接收字符串，也可接收缓冲区地址与长度。PDF 将收到的正文以 `text` 类型发回；补充：UTF-8 文本使用 `text`，二进制内容使用 `binary`，按原类型回显时读取 `get_opcode` 选择发送类型。

HTTP 状态码按 `1xx` 信息、`2xx` 成功、`3xx` 重定向、`4xx` 请求问题、`5xx` 服务端问题分类。普通欢迎页面使用 `200`，WebSocket 升级使用 `101`；连接关闭状态码属于 WebSocket 协议，例如 `1000` 表示正常关闭。

## 服务器与浏览器客户端

示例服务端在同一端口处理 HTTP 欢迎页面和 WebSocket 回显。HTTP 请求触发 `OnHttp`；WebSocket 握手成功后触发 `OnOpen`，消息与关闭分别触发 `OnMessage`、`OnClose`。连接建立失败交给 `OnFail`。

浏览器通过普通地址访问欢迎页面，通过 `WebSocket` 对象建立消息连接。

| 浏览器事件    | 用途                               |
| ------------- | ---------------------------------- |
| `onopen`    | 连接建立后开始交互                 |
| `onmessage` | 从事件的 `data` 读取消息         |
| `onerror`   | 观察连接错误                       |
| `onclose`   | 观察连接关闭                       |
| 按钮点击回调  | 读取输入框，通过 `send` 发送文本 |

在 `onopen` 触发后提交消息，通过浏览器开发者工具控制台观察发送与回显。普通连接使用 `ws`，需要 TLS 的连接使用 `wss`。

## 本仓库示例

在项目根目录执行 `cmake -S . -B build`，再执行 `cmake --build build --target websocketpp_server`。程序为 `build/bin/websocketpp_server`，浏览器测试页复制为 `build/bin/websocketpp_client.html`。

服务端监听 `9000`，客户端页面中的连接地址使用 `8888`；联调时将客户端端口与服务端端口统一。跨机器访问填写服务端实际地址，Docker 端口映射对应服务器的监听端口；浏览器中的 `127.0.0.1` 指向浏览器所在机器。
