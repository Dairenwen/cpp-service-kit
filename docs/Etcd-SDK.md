# Etcd-SDK

Etcd是一个golang编写的分布式、高可用的一致性键值存储系统，用于配置共享和服务发现等。它使用Raft一致性算法来保持集群数据的一致性，且客户端通过长连接watch功能，能够及时收到数据变化通知，相较于Zookeeper框架更加轻量化。

### 主要功能

1. 键值对存储：提供 Key-Value 数据的存储和管理。
2. 租约机制（Lease）：为键值对设置过期时间，支持通过长连接定期续约（KeepAlive），防止数据过期。
3. 数据变化通知（Watch）：监听键值对变化，并向客户端发送通知。
4. 目录管理与监控：支持按路径组织键值对，并对指定目录（键前缀）下的数据进行统一监控。

## 安装Etcd

安装Etcd：

```
sudo apt-get install etcd
```

启动Etcd服务：

```
sudo systemctl start etcd
```

设置Etcd开机自启：

```
sudo systemctl enable etcd
```

### 节点配置

如果是单节点集群其实就可以不用进行配置，默认etcd的集群节点通信端口为2380，客户端访问端口为2379，若需要修改，则可以配置：/etc/default/etcd

```bash
#节点名称，默认为 "default"
ETCD_NAME="etcd1"
#数据目录，默认为 "${name}.etcd"
ETCD_DATA_DIR="/var/lib/etcd/default.etcd"
#用于客户端连接的 URL。
ETCD_LISTEN_CLIENT_URLS="http://192.168.65.132:2379,http://127.0.0.1:2379"
#用于客户端访问的公开，也就是提供服务的 URL
ETCD_ADVERTISE_CLIENT_URLS="http://192.168.65.132:2379,http://127.0.0.1:2379"
#用于集群节点间通信的 URL。
ETCD_LISTEN_PEER_URLS="http://192.168.65.132:2380"
ETCD_INITIAL_ADVERTISE_PEER_URLS="http://192.168.65.132:2380"
#心跳间隔时间-毫秒
ETCD_HEARTBEAT_INTERVAL=100
#选举超时时间-毫秒
ETCD_ELECTION_TIMEOUT=1000
#以下为集群配置，若无集群则需要注销
#初始集群状态和配置--集群中所有节点
#ETCD_INITIAL_CLUSTER="etcd1=http://192.168.65.132:2380,etcd2=http://192.168.65.132:2381,etcd3=http://192.168.65.132:2382"
#初始集群令牌-集群的ID
#ETCD_INITIAL_CLUSTER_TOKEN="etcd-cluster"
#ETCD_INITIAL_CLUSTER_STATE="new"
#以下为安全配置，如果要求SSL连接etcd的话，把下面的配置启用，并修改文件路径
#ETCD_CERT_FILE="/etc/ssl/client.pem"
#ETCD_KEY_FILE="/etc/ssl/client-key.pem"
#ETCD_CLIENT_CERT_AUTH="true"
#ETCD_TRUSTED_CA_FILE="/etc/ssl/ca.pem"
#ETCD_AUTO_TLS="true"
#ETCD_PEER_CERT_FILE="/etc/ssl/member.pem"
#ETCD_PEER_KEY_FILE="/etc/ssl/member-key.pem"
#ETCD_PEER_CLIENT_CERT_AUTH="false"
#ETCD_PEER_TRUSTED_CA_FILE="/etc/ssl/ca.pem"
#ETCD_PEER_AUTO_TLS="true"
```

单节点运行示例：

```bash
etcd --name etcd1 --initial-advertise-peer-urls http://192.168.65.132:2380 \
   --listen-peer-urls http://192.168.65.132:2380 \
   --listen-client-urls http://192.168.65.132:2379 \
   --advertise-client-urls http://192.168.65.132:2379 \
   --initial-cluster-token etcd-cluster \
   --initial-cluster etcd1=http://192.168.65.132:2380,etcd2=http://192.168.65.132:2381,etcd3=http://192.168.65.132:2382 \
   --initial-cluster-state new &> nohup1.out &
```

## 客户端SDK

etcd 采用golang 编写，v3版本通信采用grpc API，即(HTTP2+protobuf)；官方只维护了go语言版本的client库, 因此需要找到C/C++ 非官方的client 开发库：**etcd-cpp-apiv3**

### 安装

etcd-cpp-apiv3是一个etcd的C++版本客户端API。它依赖于mipsasm, boost, protobuf, gRPC,cpprestsdk等库。

依赖安装：

```
sudo apt-get install libboost-all-dev libssl-dev
sudo apt-get install libprotobuf-dev protobuf-compiler-grpc
sudo apt-get install libgrpc-dev libgrpc++-dev
sudo apt-get install libcpprest-dev
```

api框架安装

```
git clone https://github.com/etcd-cpp-apiv3/etcd-cpp-apiv3.git
cd etcd-cpp-apiv3
mkdir build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/usr
make -j$(nproc) && sudo make install
```

### 头文件

```
#include <etcd/Client.hpp>
#include <etcd/KeepAlive.hpp>
#include <etcd/Watcher.hpp>
#include <etcd/Response.hpp>
#include <etcd/Value.hpp>
```

### 接口

```cpp
namespace etcd {
class Value {
    bool is_dir()；//判断是否是一个目录
    std::string const& key() //键值对的key值
    std::string const& as_string()//键值对的val值

    int64_t lease() //用于创建租约的响应中，返回租约ID
}
//etcd会监控所管理的数据的变化，一旦数据产生变化会通知客户端
//在通知客户端的时候，会返回改变前的数据和改变后的数据
class Event {
    enum class EventType {
        PUT, //键值对新增或数据发生改变
        DELETE_,//键值对被删除
        INVALID,
    };
    enum EventType event_type()
    const Value& kv() //当前键值对的数据
    const Value& prev_kv() // 改变前键值对的数据
}
class Response {
    bool is_ok()
    std::string const& error_message()
    Value const& value()//当前的数值 或者 一个请求的处理结果
    Value const& prev_value()//之前的数值
    Value const& value(int index)//
    std::vector<Event> const& events();//触发的事件

    using Values = std::vector<Value>;
    Values const& values() const; //多组数据的响应结果--针对目录
}
}
//pplx::task 并行库异步结果对象
//阻塞方式   get(): 阻塞直到任务执行完成，并获取任务结果
//非阻塞方式 wait(): 等待任务到达终止状态，然后返回任务状态
namespace etcd {
class Client {
   // etcd_url： "http://127.0.0.1:2379"
   Client(std::string const& etcd_url,
       std::string const& load_balancer = "round_robin");
   //Put a new key-value pair 新增一个键值对
   pplx::task<Response> put(std::string const& key,
       std::string const& value);
   //新增带有租约的键值对 （一定时间后，如果没有续租，数据自动删除）
   pplx::task<Response> put(std::string const& key,
       std::string const& value,
       const int64_t leaseId);
   pplx::task<Response> get(std::string const& key);
   //获取一个指定key目录下的数据列表
   pplx::task<Response> ls(std::string const& key);
   //创建并获取一个存活ttl时间的租约
   pplx::task<Response> leasegrant(int ttl);
   //获取一个租约保活对象，其参数ttl表示租约有效时间
   pplx::task<std::shared_ptr<KeepAlive>> leasekeepalive(int ttl);
   //Returns etcdv3::ERROR_KEY_NOT_FOUND if the key does not exist
   pplx::task<Response> rm(std::string const& key);
   //Returns etcdv3::ERROR_KEY_NOT_FOUND if the no key been deleted.
   pplx::task<Response> rmdir(std::string const& key, bool recursive = false);
   //Watches for changes of a key or a subtree
   pplx::task<Response> watch(std::string const& key, bool recursive = false);
   //撤销一个指定的租约
   pplx::task<Response> leaserevoke(int64_t lease_id);
   //数据锁
   pplx::task<Response> lock(std::string const& key);
   pplx::task<Response> unlock(std::string const& lock_key);
   //Execute a etcd transaction
   pplx::task<Response> txn(etcdv3::Transaction const& txn);
}
class KeepAlive {
   KeepAlive(Client const& client, int ttl, int64_t lease_id = 0);
   KeepAlive(std::string const& address,
           std::function<void(std::exception_ptr)> const& handler,
           int ttl,
           int64_t lease_id = 0);
   //返回租约ID
   int64_t Lease();
   //停止保活动作
   void Cancel();
}
class Watcher {
   Watcher(Client const& client,
       std::string const& key, //要监控的键值对key
       std::function<void(Response)> callback, //发生改变后的回调
       bool recursive = false); //是否递归监控目录下的所有数据改变
   Watcher(std::string const& address,
       std::string const& key,
       std::function<void(Response)> callback,
       bool recursive = false);
   //阻塞等待，直到监控任务被停止
   bool Wait();
   //异步非阻塞，设置任务停止时的回调函数，当任务被主动取消则传入true
   bool Wait(std::function<void(bool)> callback);
   bool Cancel();
}
```

## 服务管理封装

因为etcd是一个内存键值存储中心，且提供了数据改变通知功能，因此作为注册中心也是一个很好的应用。
使用Etcd作为服务注册发现中心，你需要定义服务的注册和发现逻辑。这通常涉及到以下几个操作：

1. 服务注册：服务启动时，向Etcd注册自己的服务名称和访问地址信息的键值对。
2. 服务发现：客户端通过Etcd获取服务的访问地址信息，用于连接服务器进行远程调用。
3. 健康检查：服务定期向Etcd发送心跳，以维持其注册信息的有效性。

### 设计

服务管理的封装中，主要需要注意两个点：

1. 服务的注册: 向etcd服务器中，添加服务数据；例如: put /user/myid 192.168.30.86:9090
2. 服务的发现: 从etcd服务器中，获取服务数据；例如：ls /user 通过从etcd服务器上获取的数据，可以解析获知，当前有哪个主机可以提供/user 服务
3. 封装思想：

- 将服务注册和服务发现分别封装类，方便外部能够通过实例化的对象简便实现服务发现/注册
- 服务注册类，添加 {/服务名称, 访问地址} 的键值对。
- 服务发现类，通过ls以及监视 / 目录来获取 / 下的所有键值对变动。

#### 服务的注册

封装服务注册类，将客户端请求与租约保活部分封装起来，向外提供一个接口能够实现数据的新增即可，通过实例化的对象可以方便快捷的实现服务注册功能。

##### 成员：

- 注册中心地址
- 当前节点标识
- 要注册的服务信息（服务名称&节点地址）
- 租约保活器

##### 接口：

- 服务注册接口

#### 服务的发现

封装服务发现类，将客户端请求与路径监视部分封装起来，通过实例化的对象可以方便快捷的实现服务发现功能，并针对发现的服务进行对应处理。为了能够与其他功能进行解耦，因此这里封装的时候由外部传入针对服务上线和下线所进行处理的接口进行回调处理，当前模块部分本身并不关注具体事件该如何处理。

##### 成员：

- 注册中心地址
- 针对服务上线&下线的处理回调对象
- 监视器

##### 接口：

- 服务监视接口
