# BRpc

RPC（Remote Procedure Call）远程过程调用，主要目标就是让构建分布式计算（应用）更容易，在提供强大的远程调用能力时不损失本地调用的语义简洁性。

brpc是用c++语言编写的工业级RPC框架，常用于搜索、存储、机器学习、广告、推荐等高性能系统。

你可以使用它：

- 搭建能在一个端口支持多协议的服务, 或访问各种服务

  - restful http/https, h2/gRPC。使用brpc的http实现比libcurl方便多了。从其他语言通过HTTP/h2+json访问基于protobuf的协议.
  - redis和memcached, 线程安全，比官方client更方便。
  - rtmp/flv/hls, 可用于搭建流媒体服务.
  - 支持thrift , 线程安全，比官方client更方便
  - 各种百度内使用的协议: baidu_std, streaming_rpc, hulu_pbrpc, sofa_pbrpc, nova_pbrpc, public_pbrpc, ubrpc和使用nshead的各种协议.
  - 基于工业级的RAFT算法实现搭建高可用分布式系统，已在braft开源。
- Server能同步或异步处理请求。
- Client支持同步、异步、半同步，或使用组合channels简化复杂的分库或并发访问。
- 通过http界面调试服务, 使用cpu, heap, contention profilers.
- 获得更好的延时和吞吐.
- 把你组织中使用的协议快速地加入brpc，或定制各类组件, 包括命名服务 (dns, zk, etcd), 负载均衡 (rr, random, consistent hashing)

### 1. 安装

先安装依赖

```bash
dev@dev-host:~/workspace$ sudo apt-get install -y git g++ make libssl-dev libprotobuf-dev libprotoc-dev protobuf-compiler libleveldb-dev
```

安装brpc

```bash
dev@dev-host:~/workspace$ git clone https://github.com/apache/brpc.git
dev@dev-host:~/workspace$ cd brpc/
dev@dev-host:~/workspace/brpc$ mkdir build && cd build
dev@dev-host:~/workspace/brpc/build$ cmake -DCMAKE_INSTALL_PREFIX=/usr .. && cmake --build . -j6
dev@dev-host:~/workspace/brpc/build$ make && sudo make install
```

### 2. 类与接口介绍

#### 2.1 日志输出类与接口

包含头文件： #include <butil/logging.h>

brpc库内有日志输出模块，将不需要的日志输出给禁用掉，避免其运行时的大量日志。

```cpp
namespace logging {
enum LoggingDestination {
   LOG_TO_NONE             = 0
};
struct BUTIL_EXPORT LoggingSettings {
   LoggingSettings();
   LoggingDestination logging_dest;
};
bool InitLogging(const LoggingSettings& settings);
}
```

#### 2.2 protobuf类与接口

```cpp
namespace google {
namespace protobuf {
   class PROTOBUF_EXPORT Closure {
   public:
       Closure() {}
       virtual ~Closure();
       virtual void Run() = 0;
};
inline Closure* NewCallback(void (*function)());

class PROTOBUF_EXPORT RpcController {//做错误判断
   bool Failed();
   std::string ErrorText() ;
}
}
}
```

#### 2.3 服务端类与接口

```cpp
namespace brpc {

struct ServerOptions {
   //无数据传输，则指定时间后关闭连接
   int idle_timeout_sec; // Default: -1 (disabled)
   int num_threads; // Default: #cpu-cores
   //....
}
enum ServiceOwnership {
   //添加服务失败时，服务器将负责删除服务对象
   SERVER_OWNS_SERVICE,
   //添加服务失败时，服务器也不会删除服务对象
   SERVER_DOESNT_OWN_SERVICE
};
class Server {
   int AddService(google::protobuf::Service* service,//由什么函数来执行
                  ServiceOwnership ownership);//如何释放
   int Start(int port, const ServerOptions* opt);
   int Stop(int closewait_ms/*not used anymore*/);
   int Join();
   //休眠直到ctrl+c按下，或者stop和join服务器
   void RunUntilAskedToQuit();
}
class ClosureGuard {//RAII思想，自动调用done进行资源释放，常用异步Rpc
   explicit ClosureGuard(google::protobuf::Closure* done);
   ~ClosureGuard() { if (_done) _done->Run(); }
}

}
```

#### 2.2.4 HTTP相关类

```cpp
class URI {
typedef butil::FlatMap<std::string, std::string> QueryMap;
typedef QueryMap::const_iterator QueryIterator;
int SetHttpURL(const std::string& url);// Returns 0 on success, -1 otherwise
void set_path(const std::string& path)
void set_host(const std::string& host)
void set_port(int port)
void SetHostAndPort(const std::string& host_and_optional_port);
size_t RemoveQuery(const char* key);// Returns 1 on removed, 0 otherwise

const std::string& host() const { return _host; }
int port()
const std::string& path()
const std::string& user_info()
const std::string& query() const;
const std::string* GetQuery(const std::string& key)
void SetQuery(const std::string& key, const std::string& value);
QueryIterator QueryBegin()
QueryIterator QueryEnd()
size_t QueryCount()
};

enum HttpMethod {
    HTTP_METHOD_DELETE      =   0,
    HTTP_METHOD_GET         =   1,
    HTTP_METHOD_HEAD        =   2,
    HTTP_METHOD_POST        =   3,
    HTTP_METHOD_PUT         =   4,
};
const char *HttpMethod2Str(HttpMethod http_method);
bool Str2HttpMethod(const char* method_str, HttpMethod* method);

static const int HTTP_STATUS_OK = 200;
static const int HTTP_STATUS_BAD_REQUEST = 400;
static const int HTTP_STATUS_UNAUTHORIZED = 401;
static const int HTTP_STATUS_FORBIDDEN = 403;
static const int HTTP_STATUS_NOT_FOUND = 404;
static const int HTTP_STATUS_METHOD_NOT_ALLOWED = 405;
static const int HTTP_STATUS_INTERNAL_SERVER_ERROR = 500;

class HttpHeader {
    const std::string& content_type()
    void set_content_type(const std::string& type)
    const std::string* GetHeader(const std::string& key)
    void SetHeader(const std::string& key,
        const std::string& value);
    const URI& uri() const { return _uri; }
    HttpMethod method() const { return _method; }
    void set_method(const HttpMethod method)
    int status_code()
    void set_status_code(int status_code);

}
//继承了原来的cotroller，用来错误判断
class Controller : public google::protobuf::RpcController {
    void set_timeout_ms(int64_t timeout_ms);
    void set_max_retry(int max_retry);
    void Reset();
    google::protobuf::Message* response();//获取/设置正文
    HttpHeader& http_response();//可以获取/设置http响应头部
    butil::IOBuf& response_attachment();
    HttpHeader& http_request();//可以获取/设置http请求头部
    butil::IOBuf& request_attachment();//获取/设置正文


    bool Failed();
    std::string ErrorText();

    using AfterRpcRespFnType = std::function<
        void(Controller* cntl,
        const google::protobuf::Message* req,
        const google::protobuf::Message* res)>;
    void set_after_rpc_resp_fn(AfterRpcRespFnType&& fn)
}

namespace butil {
class IOBuf {
    //Returns 0 on success, -1 otherwise.
    int append(const std::string& s);
    // Convert all data in this buffer to a std::string.
    std::string to_string() const;
    void clear();
    bool empty() const;
    size_t length() const;
};
```

#### 2.2.5 客户端类与接口

```cpp
namespace brpc {
enum ProtocolType : int {
 PROTOCOL_UNKNOWN = 0,
 PROTOCOL_BAIDU_STD = 1,
 PROTOCOL_STREAMING_RPC = 2,
 PROTOCOL_HULU_PBRPC = 3,
 PROTOCOL_SOFA_PBRPC = 4,
 PROTOCOL_RTMP = 5,
 PROTOCOL_THRIFT = 6,
 PROTOCOL_HTTP = 7,
 PROTOCOL_PUBLIC_PBRPC = 8,
 PROTOCOL_NOVA_PBRPC = 9,
 PROTOCOL_REDIS = 10,
 PROTOCOL_NSHEAD_CLIENT = 11,
 PROTOCOL_NSHEAD = 12,
 PROTOCOL_HADOOP_RPC = 13,
 PROTOCOL_HADOOP_SERVER_RPC = 14,
 PROTOCOL_MONGO = 15,
 PROTOCOL_UBRPC_COMPACK = 16,
 PROTOCOL_DIDX_CLIENT = 17,
 PROTOCOL_MEMCACHE = 18,
 PROTOCOL_ITP = 19,
 PROTOCOL_NSHEAD_MCPACK = 20,
 PROTOCOL_DISP_IDL = 21,
 PROTOCOL_ERSDA_CLIENT = 22,
 PROTOCOL_UBRPC_MCPACK2 = 23,
 PROTOCOL_CDS_AGENT = 24,
 PROTOCOL_ESP = 25,
 PROTOCOL_H2 = 26
};
struct ChannelOptions {
   //请求连接超时时间
   int32_t connect_timeout_ms;// Default: 200 (milliseconds)
   //rpc请求超时时间
   int32_t timeout_ms;// Default: 500 (milliseconds)
   //最大重试次数
   int max_retry;// Default: 3
   //序列化协议类型  options.protocol = "baidu_std";
   AdaptiveProtocolType protocol;
   //....
}
class Channel : public ChannelBase {
   //初始化接口，成功返回0；
   int Init(
       const char* server_addr_and_port,  //192.168.xx.xx:9000
       const ChannelOptions* options);
   void CallMethod(const google::protobuf::MethodDescriptor* method,
       google::protobuf::RpcController* controller,//用于设置http请求信息或者获取rpc请求是否成功的判断
       const google::protobuf::Message* request,
       google::protobuf::Message* response,
       google::protobuf::Closure* done);//为NULL表示同步阻塞请求，否能可以实例化done进行异步调用，拿到结果后调用
};

inline ::google::protobuf::Closure* NewCallback(void (*function)());
```

#### 2.2.6 定时任务相关

头文件：bthread/unstable.h , butil/time.h

```cpp

// Return 0 on success, errno otherwise.
int bthread_timer_add(bthread_timer_t* id,
    struct timespec abstime,
    void (*on_timer)(void*),
    void* arg);
// Returns: 0 - exist & not-run;
//          1 - still running;
//          EINVAL - not exist.
int bthread_timer_del(bthread_timer_t id);

namespace butil {
    timespec microseconds_from_now(int64_t milliseconds);
    timespec seconds_from_now(int64_t seconds);

}
```

### 3. 使用

#### 3.1 同步调用

同步调用是指客户端会阻塞收到server端的响应或发生错误。

下面我们以Echo（输出hello world）方法为例, 来讲解基础的同步RPC请求是如何实现的。

- 创建proto文件 - main.proto

```cpp
syntax="proto3";
package example;

option cc_generic_services = true;

// 定义Echo方法请求参数结构
message EchoRequest {
     string message = 1;
};

// 定义Echo方法响应参数结构
message EchoResponse {
     string message = 1;
};

// 定义 RPC 远端方法
service EchoService {
     rpc Echo(EchoRequest) returns (EchoResponse);
};
```

- 创建服务端源码 - brpc_server.cpp

```cpp
#include <gflags/gflags.h>
#include <butil/logging.h>
#include <brpc/server.h>
#include <json2pb/pb_to_json.h>
#include "main.pb.h"

// 使用gflags定义一些命令行参数
DEFINE_int32(listen_port, 9000, "服务器地址信息");
DEFINE_int32(idle_timeout_s, -1, "空闲连接超时关闭时间：默认-1表示不关闭");
DEFINE_int32(thread_count, 3, "服务器启动线程数量");

namespace example {
class EchoServiceImpl : public EchoService {
public:
   EchoServiceImpl() {}
   virtual ~EchoServiceImpl() {}
   // cntl_base：包含除了request和 response之外的参数集合
   // request: 请求，只读的，来自client端的数据包
   // response: 回复。需要用户填充，如果存在required字段没有被设置，该次调用会失败。
   // done: done由框架创建，递给服务回调，包含了调用服务回调后的后续动作，
   //     包括检查response正确性，序列化，打包，发送等逻辑。
   //    不管成功失败，done->Run()必须在请求处理完成后被用户调用一次。
   virtual void Echo(
       google::protobuf::RpcController* cntl_base,
       const EchoRequest* request,
       EchoResponse* response,
       google::protobuf::Closure* done) {
       // 类型于守卫锁，以RAII方式自动释放done对象
       brpc::ClosureGuard done_guard(done);
       auto cntl = static_cast<brpc::Controller*>(cntl_base);
       // 打印一些相关的参数日志信息
       std::cout << "请求内容：" << request->message() << std::endl;
       // 填充响应，客户端发送什么数据，服务器就回复什么数据
       response->set_message(request->message() + " Hello");
   }
};
}  // namespace example

int main(int argc, char* argv[]) {

   logging::LoggingSettings log_setting;
   log_setting.logging_dest = logging::LoggingDestination::LOG_TO_NONE;
   logging::InitLogging(log_setting);
   // 解析命令行参数
   google::ParseCommandLineFlags(&argc, &argv, true);

   // 定义服务器
   brpc::Server server;

   // 创建服务对象.
   example::EchoServiceImpl echo_service_impl;

   // 将服务添加到服务器中
   if (server.AddService(&echo_service_impl,
       brpc::SERVER_DOESNT_OWN_SERVICE) != 0) {
       std::cout << "add service failed!\n";
       return -1;
   }
   // 开始运行服务器
   brpc::ServerOptions options;
   options.idle_timeout_sec = FLAGS_idle_timeout_s;
   options.num_threads = FLAGS_thread_count;
   if (server.Start(FLAGS_listen_port, &options) != 0) {
       std::cout << "Fail to start EchoServer";
       return -1;
   }

   // 阻塞等待服务端运行
   server.RunUntilAskedToQuit();
   return 0;
}
```

- 创建客户端源码 - client.cpp

```cpp
#include <gflags/gflags.h>
#include <butil/logging.h>
#include <butil/time.h>
#include <brpc/channel.h>
#include "main.pb.h"

DEFINE_string(protocol, "baidu_std", "通信协议类型，默认使用brpc自定制协议");
DEFINE_string(server_host, "127.0.0.1:9000", "服务器地址信息");
DEFINE_int32(timeout_ms, 500, "Rpc请求超时时间-毫秒");
DEFINE_int32(max_retry, 3, "请求重试次数");

int main(int argc, char* argv[]) {
   // 解析命令行参数
   google::ParseCommandLineFlags(&argc, &argv, true);

   // 创建通道， 可以理解为客户端到服务器的一条通信线路
   brpc::Channel channel;

   // 初始化通道，NULL表示使用默认选项
   brpc::ChannelOptions options;
   options.protocol = FLAGS_protocol;
   options.timeout_ms = FLAGS_timeout_ms;
   options.max_retry = FLAGS_max_retry;
   if (channel.Init(FLAGS_server_host.c_str(), &options) != 0) {
       LOG(ERROR) << "Fail to initialize channel";
       return -1;
   }
   //通常，我们不应直接调用通道，而是包装它的stub服务,通过stub进行rpc调用
   example::EchoService_Stub stub(&channel);

   // 创建请求、响应、控制对象
   example::EchoRequest request;
   example::EchoResponse response;
   brpc::Controller cntl;
   // 构造请求响应
   request.set_message("hello world");

   //由于“done”（最后一个参数）为NULL，表示阻塞等待响应
   stub.Echo(&cntl, &request, &response, NULL);
   if (cntl.Failed()) {
       std::cout << "请求失败: " << cntl.ErrorText() << std::endl;
       return -1;
   }
   std::cout << "响应：" << response.message() << std::endl;
   return 0;
}
```

#### 3.2 异步调用

异步调用是指客户端注册一个响应处理回调函数， 当调用一个RPC接口时立即返回，不会阻塞等待响应， 当server端返回响应时会调用传入的回调函数处理响应。

下面是异步调用的伪代码：

```cpp
static void OnRPCDone(MyResponse* response, brpc::Controller* cntl) {
   // unique_ptr会帮助我们在return时自动删掉response/cntl，防止忘记。gcc 3.4下的unique_ptr是模拟版本。
   std::unique_ptr<MyResponse> response_guard(response);
   std::unique_ptr<brpc::Controller> cntl_guard(cntl);
   if (cntl->Failed()) {
       // RPC失败了. response里的值是未定义的，勿用。
   } else {
       // RPC成功了，response里有我们想要的数据。开始RPC的后续处理。
   }
   // NewCallback产生的Closure会在Run结束后删除自己，不用我们做。
}

MyResponse* response = new MyResponse;
brpc::Controller* cntl = new brpc::Controller;
MyService_Stub stub(&channel);

MyRequest request;  // 不用new request
request.set_foo(...);cntl->set_timeout_ms(...);
stub.some_method(cntl, &request, response, brpc::NewCallback(OnRPCDone, response, cntl));
```

#### 3.3 定时任务

```cpp
#include <iostream>
#include <string>
#include <thread>
#include <butil/logging.h>
#include <butil/time.h>
#include <bthread/unstable.h>

static void timer_func(void *arg)
{
    std::cout << "timer callback" << std::endl;
}
int main()
{
    bthread_timer_t timer_id;
    auto abstime = butil::seconds_from_now(3);
    int ret = bthread_timer_add(&timer_id, abstime, timer_func, nullptr);
    if (ret != 0) {
        std::cout << "bthread_timer_add failed" << std::endl;
        return -1;
    }
    //int bthread_timer_del(bthread_timer_t id);
    std::this_thread::sleep_for(std::chrono::seconds(10));
    return 0;
}
```

## 4. 封装

### 4.1 设计

因为Rpc调用的特殊性，我们无法封装一个客户端能够实现所有功能，因此这里只是封装一些能够简化rpc调用的操作。

封装思想

- 封装信道管理类：其中可以管理多个信道，为每个服务都实例化一个对象管理服务对应的主机信道
- 封装服务管理类：相当于其内部管理多个服务，每个服务内管理各个服务的信道。
- 封装异步Closure工厂类：便于rpc客户能够快速的设置异步回调（封装这个的目的是brpc创建Closure的接口不好用）
- 封装服务器工厂类：便于简单的实现服务器对象的创建。

#### 4.1.1 服务信道管理类

用于管理主机地址与通信信道的对应关系的类，最主要目的是以轮询或其他负载均衡策略来快速获取能够提供某个服务的主机信道。

成员变量

- 信道对象vector<主机地址, 信道对象>
- 轮询记录索引
- 互斥锁

成员方法

- 新增服务信道
- 移除服务信道
- 获取服务信道
- 获取服务节点地址

#### 4.1.2 服务管理类

能够快速获取某个服务的主机信道管理对象，以便于快速对该服务的主机信道做出处理。

成员变量

- {服务名称，信道管理对象} 映射map
- 互斥锁

成员方法

- 新增服务地址接口
- 移除服务地址接口
- 获取服务地址接口
- 服务预声明接口（关注了哪些服务，哪些服务才会添加管理）

#### 4.1.3 异步回调工厂类

成员方法

- 静态方法创建Closure

#### 4.1.4 服务器工厂类

成员方法

- 静态方法创建Server

### 4.2 实现

#### 4.2.1 服务信道管理类

```cpp
//指定服务的信道管理类
class Channels {
    public:
        using ChannelPtr = std::shared_ptr<Channels>;
        Channels();
        //1. 新增节点
        void insert(const std::string &addr);
        //2. 删除节点
        void remove(const std::string &addr);
        //3. 获取节点信道
        ChannelPtr select();
        //4. 获取节点地址
        std::optional<std::string> selectAddr();
    private:
        std::mutex _mtx;
        std::string _svc_name; // 服务名称
        uint32_t _idx; //轮询下标
        std::vector<std::pair<std::string, ChannelPtr>> _channels;
};

Channels::Channels() : _idx(0) {};
void Channels::insert(const std::string &addr) {
    std::unique_lock<std::mutex> lock(_mtx);
    //1. 实例化channel对象
    auto channel = std::make_shared<brpc::Channel>();
    brpc::ChannelOptions options;
    options.protocol = "baidu_std";
    options.timeout_ms = 30000;
    channel->Init(addr.c_str(), &options);
    //3. 将对象添加到vector中
    _channels.push_back(std::make_pair(addr, channel));
}
void Channels::remove(const std::string &addr) {
    //1. 加锁
    std::unique_lock<std::mutex> lock(_mtx);
    //3. 从vector中移除channel
    for (auto vit = _channels.begin(); vit != _channels.end(); ++vit) {
        if (addr == vit->first) {
            _channels.erase(vit);
            break;
        }
    }
}
ChannelPtr Channels::select() {
    //1. 加锁
    std::unique_lock<std::mutex> lock(_mtx);
    if (_channels.size() == 0) {
        return ChannelPtr();
    }
    //2. 获取下标索引
    size_t idx = (_idx++) % _channels.size();
    //3. 返回下标对应的节点信道
    return _channels[idx].second;
}
std::optional<std::string> Channels::selectAddr() {
    std::unique_lock<std::mutex> lock(_mtx);
    if (_channels.size() == 0) {
        return std::optional<std::string>();
    }
    //2. 获取下标索引
    size_t idx = (_idx++) % _channels.size();
    //3. 返回下标对应的节点地址
    return _channels[idx].first;
}
```

#### 4.2.2 服务管理类

```cpp
//服务管理类
class SvcChannels {
    public:
        using ptr = std::shared_ptr<SvcChannels>;
        SvcChannels() = default;
        void setWatch(const std::string &svc_name);
        void addNode(const std::string &svc_name, const std::string &addr);
        void delNode(const std::string &svc_name, const std::string &addr);
        ChannelPtr getNode(const std::string &svc_name);
        std::optional<std::string> getNodeAddr(const std::string &svc_name);
    private:
        Channels::ptr _Channels(const std::string &svc_name);
    private:
        std::mutex _mtx;
        std::unordered_map<std::string, Channels::ptr> _maps;
};
Channels::ptr SvcChannels::_Channels(const std::string &svc_name) {
    //限定作用域是为了只保护获取集合的过程，而不需要保护添加结果的过程
    std::unique_lock<std::mutex> lock(_mtx);
    auto it = _maps.find(svc_name);
    if (it == _maps.end()) {
        return Channels::ptr();
    }
    return it->second;
}
void SvcChannels::setWatch(const std::string &svc_name) {
    // 针对指定服务，初始化构造集合，并添加管理
    std::unique_lock<std::mutex> lock(_mtx);
    auto channels = std::make_shared<Channels>();
    _maps.insert(std::make_pair(svc_name, channels));
}
void SvcChannels::addNode(const std::string &svc_name,
    const std::string &addr) {
    // 加锁，查找集合，没找到则直接返回
    // 找到了：获取集合，通过集合，添加节点
    Channels::ptr channels = _Channels(svc_name);
    if (!channels) {
        DBG("{} 有节点上线：{}, 未找到管理对象", svc_name, addr);
        return;
    }
    DBG("添加 {} 服务节点 {} 管理", svc_name, addr);
    return channels->insert(addr);
}
void SvcChannels::delNode(const std::string &svc_name,
    const std::string &addr) {
    // 加锁，查找集合，没找到则直接返回
    // 找到了：获取集合，通过集合，删除节点
    Channels::ptr channels = _Channels(svc_name);
    if (!channels) {
        DBG("{} 有节点下线：{}, 未找到管理对象", svc_name, addr);
        return;
    }
    DBG("移除 {} 服务节点 {} 管理", svc_name, addr);
    return channels->remove(addr);
}
ChannelPtr SvcChannels::getNode(const std::string &svc_name) {
    // 加锁，查找集合，没找到则直接返回
    // 找到了：获取集合，通过集合，获取节点
    Channels::ptr channels = _Channels(svc_name);
    if (!channels) { return ChannelPtr(); }
    return channels->select();
}
std::optional<std::string> SvcChannels::getNodeAddr(
    const std::string &svc_name) {
    Channels::ptr channels = _Channels(svc_name);
    if (!channels) { return std::optional<std::string>(); }
    return channels->selectAddr();
}
```

#### 4.2.3 工厂类

```cpp
#pragma once
#include <butil/logging.h>
#include <brpc/server.h>
#include <brpc/channel.h>
#include <mutex>
#include <optional>
class ClosureFactory {
    public:
        using callback_t = std::function<void()>;
        static google::protobuf::Closure* create(callback_t &&cb);
    private:
        struct Object{
            using ptr = std::shared_ptr<Object>;
            callback_t callback;
        };
        static void asyncCallback(const Object::ptr obj);
};

google::protobuf::Closure* ClosureFactory::create(callback_t &&cb) {
    auto obj = std::make_shared<Object>();
    obj->callback = std::move(cb);
    return brpc::NewCallback(&ClosureFactory::asyncCallback, obj);
}
void ClosureFactory::asyncCallback(const Object::ptr obj) {
    obj->callback();
}
```


```cpp
class RpcServerFactory {
    public:
        // 默认svc是堆上new出来的对象，将管理权移交给rpc服务器进行管理
        static std::shared_ptr<brpc::Server> create(int port,
            google::protobuf::Service *svc);
};

std::shared_ptr<brpc::Server> RpcServerFactory::create(int port,
    google::protobuf::Service *svc) {
    brpc::ServerOptions options;
    options.idle_timeout_sec = -1;
    auto server = std::make_shared<brpc::Server>();
    int ret = server->AddService(svc, brpc::SERVER_OWNS_SERVICE);
    if (ret == -1) {
        ERR("添加RPC服务失败!");
        abort();
    }
    ret = server->Start(port, &options);
    if (ret == -1) {
        ERR("启动服务器失败!");
        abort();
    }
    return server;
}
```
