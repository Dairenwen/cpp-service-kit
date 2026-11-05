# Redis++使用样例

Redis（Remote Dictionary Server）是一个开源的高性能键值对（key-value）数据库。它通常用作数据结构服务器，因为除了基本的键值存储功能外，Redis 还支持多种类型的数据结构，如字符串 （strings）、哈希（hashes）、列表（lists）、集合（sets）、有序集合（sorted sets）以及范围查询、位图、超日志和地理空间索引等。

以下是 Redis 的一些主要特性：

1. 内存中数据库：Redis 将所有数据存储在内存中，这使得读写速度非常快。
2. 持久化：尽管 Redis 是内存数据库，但它提供了持久化选项，可以将内存中的数据保存到磁盘上， 以防系统故障导致数据丢失。
3. 支持多种数据结构：Redis 不仅支持基本的键值对，还支持列表、集合、有序集合等复杂的数据结构。
4. 原子操作：Redis 支持原子操作，这意味着多个操作可以作为一个单独的原子步骤执行，这对于并发控制非常重要。
5. 发布/订阅功能：Redis 支持发布订阅模式，允许多个客户端订阅消息，当消息发布时，所有订阅者都会收到消息。
6. 高可用性：通过 Redis 哨兵（Sentinel）和 Redis 集群，Redis 可以提供高可用性和自动故障转移。
7. 复制：Redis 支持主从复制，可以提高数据的可用性和读写性能。
8. 事务：Redis 提供了事务功能，可以保证一系列操作的原子性执行。
9. Lua 脚本：Redis 支持使用 Lua 脚本进行复杂的数据处理，可以在服务器端执行复杂的逻辑。
10. 客户端库：Redis 拥有丰富的客户端库，支持多种编程语言，如 Python、Ruby、Java、C# 等。
11. 性能监控：Redis 提供了多种监控工具和命令，可以帮助开发者监控和优化性能。
12. 易于使用：Redis 有一个简单的配置文件和命令行界面，使得设置和使用变得容易。

Redis 广泛用于缓存、会话存储、消息队列、排行榜、实时分析等领域。由于其高性能和灵活性， Redis 成为了现代应用程序中非常流行的数据存储解决方案之一。

## 安装

### 命令安装

```bash
apt install redis -y
sudo apt install redis-tools
```

### 配置修改

修改 /etc/redis/redis.conf ，支持远程连接。

- 修改 bind 127.0.0.1  为 bind 0.0.0.0
- 修改 protected-mode yes 为 protected-mode no

```ini
# By default, if no "bind" configuration directive is specified, Redis listens
# for connections from all the network interfaces available on the server.
# It is possible to listen to just one or multiple selected interfaces using
# the "bind" configuration directive, followed by one or more IP addresses.
#
# Examples:
#
# bind 192.168.1.100 10.0.0.1
# bind 127.0.0.1 ::1
#
# ~~~ WARNING ~~~ If the computer running Redis is directly exposed to the
# internet, binding to all the interfaces is dangerous and will expose the
# instance to everybody on the internet. So by default we uncomment the
# following bind directive, that will force Redis to listen only into
# the IPv4 loopback interface address (this means Redis will be able to
# accept connections only from clients running into the same computer it
# is running).
#
# IF YOU ARE SURE YOU WANT YOUR INSTANCE TO LISTEN TO ALL THE INTERFACES
# JUST COMMENT THE FOLLOWING LINE.
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# bind 127.0.0.1   # 注释掉这行
bind 0.0.0.0       # 添加这行

protected-mode no  # 把 yes 改成 no

requirepass 123456  //默认用户密码
```

### 控制 Redis 启动

#### 启动 Redis 服务

```bash
service redis-server start
```

#### 停止 Redis 服务

```bash
service redis-server stop
```

#### 重启 Redis 服务

```bash
service redis-server restart
```

### redis++安装

Github 地址: https://github.com/sewenew/redis-plus-plus

#### 安装 hiredis

redis-plus-plus 是基于 hiredis 实现的.hiredis 是一个 C 语言实现的 redis 客户端.因此需要先安装 hiredis. 直接使用包管理器安装即可.

```bash
apt install libhiredis-dev
```

#### 下载 redis++ 源码

```bash
git clone https://github.com/sewenew/redis-plus-plus.git
```

#### 编译/安装 redis++

使用 cmake 构建

```bash
cd redis-plus-plus

mkdir build && cd build

cmake ..

make && sudo make install
```

构建成功后, 会在 /usr/local/include/  中多出 sw  目录, 并且内部包含 redis-plus-plus 的一系列头文件.会在 /usr/local/lib/  中多出一系列 libredis  库文件.

## 接口

#### 头文件

```cpp
#include <sw/redis++/redis.h>
```

#### 链接库

```bash
-lhiredis -lredis++ -lpthread
```

#### 异常

```cpp
namespace sw {
namespace redis {
enum ReplyErrorType {
   ERR,
   MOVED,
   ASK
};
class Error : public std::exception {
public:
   explicit Error(const std::string &msg) : _msg(msg) {}

   Error(const Error &) = default;
   Error& operator=(const Error &) = default;

   Error(Error &&) = default;
   Error& operator=(Error &&) = default;

   virtual ~Error() override = default;

   virtual const char* what() const noexcept override {
       return _msg.data();
   }

private:
   std::string _msg;
};
// .... 还有很多其他的异常类，不过这里没有解除到因此不做过多介绍
class WatchError : public Error {
public:
   explicit WatchError() : Error("Watched key has been modified") {}

   WatchError(const WatchError &) = default;
   WatchError& operator=(const WatchError &) = default;

   WatchError(WatchError &&) = default;
   WatchError& operator=(WatchError &&) = default;

   virtual ~WatchError() override = default;
};
}
}
```

#### 初始化操作

```cpp
namespace sw {
namespace redis {

struct ConnectionOptions {
   std::string host;
   int port = 6379;
   std::string path;
   std::string user = "default";
   std::string password;
   int db = 0; // 默认0号库
   bool keep_alive = false;
}
struct ConnectionPoolOptions {
   //最大连接数量，包含使用中和空闲连接
   std::size_t size = 1;
   // Max time to wait for a connection. 0ms means client waits forever.
   std::chrono::milliseconds wait_timeout{0};
   // Max lifetime of a connection. 0ms means we never expire the connection.
   std::chrono::milliseconds connection_lifetime{0};
   // Max idle time of a connection. 0ms means we never expire the connection.
   std::chrono::milliseconds connection_idle_time{0};
}
class Redis {
   // tcp://[[username:]password@]host[:port][/db]
   explicit Redis(const std::string &uri)
   explicit Redis(const ConnectionOptions &connection_opts,
       const ConnectionPoolOptions &pool_opts = {})
};
}
}
```

#### 常规操作

```cpp
class Redis {
   // Transaction commands.
   void watch(const StringView &key);
   template <typename Input>
   void watch(Input first, Input last);
   //关于transaction和pipeline，默认会创建新的connection，作者推荐尽量重用返回的对象
   //但是需要注意的是，这两个对象的操作都是非线程安全的。
   //创建一个事务对象用于事务中的批量操作的效率以及原子性
   // 若new_connection为false，则会从连接池中获取连接创建事务对象，
   //     连接在返回的事务/pipe对象析构时返回连接池
   Transaction transaction(bool piped = false, bool new_connection = true);
   //创建一个pipeline，用于提高批量操作性能
   Pipeline pipeline(bool new_connection = true);
   //删除所有库中的数据
   void flushall(bool async = false);
   //删除当前库中所有数据
   void flushdb(bool async = false);
   //删除指定键值对
   long long del(const StringView &key);
   template <typename Input>
   long long del(Input first, Input last);
   //判断指定键值对是否存在
   long long exists(const StringView &key);
   template <typename Input>
   long long exists(Input first, Input last);
   //为key设置一个过期时间
   bool expire(const StringView &key, const std::chrono::seconds &timeout);
   //移除key的超时过期
   bool persist(const StringView &key);
   //对指定数字字段进行数值增加
   long long incrby(const StringView &key, long long increment);
   //对指定数字字段进行数值减少
   long long decrby(const StringView &key, long long decrement);
}
```

#### string操作

```cpp
class Redis {
   //获取一个string键值对
   OptionalString get(const StringView &key);
   //存放一个string键值对，且设置过期时间-毫秒，0表示不设置超时
   // 标志位：EXIST/NOT_EXIST/ALWAYS/，表示什么情况新增
   bool set(const StringView &key,
       const StringView &val,
       const std::chrono::milliseconds &ttl = std::chrono::milliseconds(0),
       UpdateType type = UpdateType::ALWAYS);
   //返回旧值，并设置新值
   OptionalString getset(const StringView &key, const StringView &val);
   //批量获取数据-
   //redis.mget(keys.begin(), keys.end(), std::back_inserter(vals));
   template <typename Input, typename Output>
   void mget(Input first, Input last, Output output);
   //批量新增数据
   template <typename Input>
   void mset(Input first, Input last);
}
```

#### list操作

```cpp
class Redis {
   long long rpush(const StringView &key, const StringView &val);
   long long lpush(const StringView &key, const StringView &val);
   long long llen(const StringView &key); // 获取元素数量
   template <typename Input>
   long long rpush(const StringView &key, Input first, Input last);
   template <typename Input>
   long long lpush(const StringView &key, Input first, Input last);
   //索引从0开始，-1表示末尾元素
   OptionalString lindex(const StringView &key, long long index);
   OptionalString lpop(const StringView &key);
   OptionalString rpop(const StringView &key);

   //0，-1表示全部
   template <typename Output>
   void lrange(const StringView &key, long long start, long long stop, Output output);
}
```

#### hash操作

```cpp
class Redis {
   bool hexists(const StringView &key, const StringView &field);
   //从key哈希中删除filed字段
   long long hdel(const StringView &key, const StringView &field);
   //批量字段删除
   template <typename Input>
   long long hdel(const StringView &key, Input first, Input last);
   //获取key的所有字段到std::unordered_map
   template <typename Output>
   void hgetall(const StringView &key, Output output);
   //
   OptionalString hget(const StringView &key, const StringView &field);
   template <typename Input, typename Output>
   void hmget(const StringView &key, Input first, Input last, Output output);
   //std::unordered_map<std::string, std::string> m
   //redis.hmset("hash", m.begin(), m.end());
   template <typename Input>
   void hmset(const StringView &key, Input first, Input last);
   long long hset(const StringView &key, const StringView &field, const StringView &val);
   //对指定字段进行数值增加操作，支持负数
   long long hincrby(const StringView &key, const StringView &field, long long increment);
}
```

#### set操作

```cpp
long long sadd(const StringView &key, const StringView &member);
template <typename Input> long long sadd(const StringView &key, Input first, Input last);
long long scard(const StringView &key); //获取无序集合元素数量
bool sismember(const StringView &key, const StringView &member);
//std::unordered_set<std::string> members1;
/// redis.smembers("set", std::inserter(members1, members1.begin()));
//std::vector<std::string> members2
/// redis.smembers("set", std::back_inserter(members2));
template <typename Output> void smembers(const StringView &key, Output output);
```

#### zset操作

```cpp
long long zadd(const StringView &key,
            const StringView &member,
            double score,
            UpdateType type = UpdateType::ALWAYS,
            bool changed = false);
// return：已添加成员数量
// 当前结构不支持增加分数，若想要增加可以使用通用接口进行
//样例：auto score = redis.command<OptionalDouble>("ZADD", "key", "XX", "INCR", 10, "mem");
template <typename Input>
long long zadd(const StringView &key,
            Input first,
            Input last,
            UpdateType type = UpdateType::ALWAYS,
            bool changed = false);
// std::unordered_map<std::string, double> m = {{"m1", 1.2}, {"m2", 2.3}};
// redis.zadd("zset", m.begin(), m.end());
long long zcard(const StringView &key);
// 获取有序集合成员数量
double zincrby(const StringView &key, double increment, const StringView &member);
// 增加/减少给定成员的分数
template <typename Output>
void zrange(const StringView &key, long long start, long long stop, Output output);
// 获取以得分排名的区间成员，从低到高排序
template <typename Interval, typename Output>
void zrangebyscore(const StringView &key, const Interval &interval, Output output);
template <typename Interval, typename Output>
void zrangebyscore(const StringView &key, const Interval &interval,
                    const LimitOptions &opts, Output output);
// 获取得分的区间成员，，从低到高排序
OptionalLongLong zrank(const StringView &key, const StringView &member);
// 获取成员排名 -- 从低到高， 从0开始
template <typename Interval, typename Output>
void zrangebylex(const StringView &key, const Interval &interval, Output output);
template <typename Interval, typename Output>
void zrangebylex(const StringView &key, const Interval &interval,
                const LimitOptions &opts, Output output);
//获取字典序区间元素
template <typename Output>
void zrevrange(const StringView &key, long long start, long long stop, Output output);
// 获取排名区间内的元素--- 从高到低排序
template <typename Interval, typename Output>
void zrevrangebyscore(const StringView &key, const Interval &interval, Output output);
template <typename Interval, typename Output>
void zrevrangebyscore(const StringView &key, const Interval &interval,
                const LimitOptions &opts, Output output);
// 获取分数区间内的元素--- 从高到底排序
OptionalLongLong zrevrank(const StringView &key, const StringView &member);
// 获取指定元素排名 -- 从高到低
template <typename Interval, typename Output>
void zrevrangebylex(const StringView &key, const Interval &interval, Output output);
template <typename Interval, typename Output>
void zrevrangebylex(const StringView &key, const Interval &interval,
                    const LimitOptions &opts, Output output);
// 获取字典序区间元素
OptionalDouble zscore(const StringView &key, const StringView &member);
// 获取指定元素的权重得分
template <typename Output>
Cursor zscan(const StringView &key,
             Cursor cursor,
             const StringView &pattern,
             long long count,
             Output output);
// 按照指定匹配模式迭代浏览有序集合元素

long long zrem(const StringView &key, const StringView &member);
// 移除给定成员
Optional<std::pair<std::string, double>> zpopmax(const StringView &key);
// 弹出集合中得分最高的成员
Optional<std::pair<std::string, double>> zpopmin(const StringView &key);
// 弹出集合中得分最低的成员
template <typename Interval>
long long zremrangebyscore(const StringView &key, const Interval &interval);
// 删除分数区间元素
long long zremrangebyrank(const StringView &key, long long start, long long stop);
// 删除排名区间元素
template <typename Interval>
long long zremrangebylex(const StringView &key, const Interval &interval);
// 删除集合中字典序区间元素
```

#### Transaction&Pipeline操作

```cpp
namespace redis {
class Redis {
   //关于transaction和pipeline，默认会创建新的connection，作者建议尽量重用返回的对象
   //    但是需要注意的是，这两个对象的操作都是非线程安全的。
   // 若new_connection为false，则会从连接池中获取连接创建事务对象，这样成本会低很多
   //     连接在返回的transaction和pipeline对象析构时返回连接池
   Transaction transaction(bool piped = false, bool new_connection = true);
   //创建一个pipeline，用于提高批量操作性能
   Pipeline pipeline(bool new_connection = true);
}

using Transaction = QueuedRedis<TransactionImpl>;
using Pipeline = QueuedRedis<PipelineImpl>;

template <typename Impl>
class QueuedRedis {
   // 创建一个redis对象，与该对象共享连接
   //     作者建议：尽量限定返回的redis作用范围，并尽快销毁它（否则很容易死锁）
   Redis redis();
   QueuedReplies exec(); //用于执行并获取返回结果
   void discard();//过程中取消操作
   QueuedRedis& flushall(bool async = false)
   QueuedRedis& del(const StringView &key)
   QueuedRedis& exists(const StringView &key)
   QueuedRedis& expire(const StringView &key, const std::chrono::seconds &timeout)
   QueuedRedis& get(const StringView &key)
   QueuedRedis& getrange(const StringView &key, long long start, long long end)
   QueuedRedis& getset(const StringView &key, const StringView &val)
   QueuedRedis& mget(Input first, Input last)
   //......
   QueuedRedis& hexists(const StringView &key, const StringView &field)
   QueuedRedis& hget(const StringView &key, const StringView &field)
   QueuedRedis& hgetall(const StringView &key)
   //。。。。。
   QueuedRedis& lrange(const StringView &key, long long start, long long stop)
};
// QueuedRedis 中的所有常规数据操作都无法直接获取结果，而是在执行exec()，获取保存了结果的对象
QueuedRedis.exists().get().mget().sget()....exec()
class QueuedReplies {
   std::size_t size();
   template <typename Result>
   auto get(std::size_t idx)
       -> typename std::enable_if<!std::is_same<Result, bool>::value, Result>::type
   template <typename Output>
   void get(std::size_t idx, Output output);
   redisReply& get(std::size_t idx);
};
}
```

```cpp
#include <iostream>
#include <string>
#include <unordered_map>
#include <cassert>
#include <thread>
#include <chrono>
#include "init.h"

void show(std::shared_ptr<sw::redis::Redis> &redis) {
   auto username = redis->get("username");
   if (username) std::cout << *username << std::endl;
   auto nickname = redis->get("nickname");
   if (nickname) std::cout << *nickname << std::endl;

   std::vector<std::string> skills;
   redis->lrange("skills", 0, -1, std::back_inserter(skills));
   for (auto &v : skills) {
       std::cout << v << std::endl;
   }

   std::unordered_map<std::string, std::string> properties;
   redis->hgetall("properties", std::inserter(properties, properties.begin()));
   for (auto &r : properties) {
       std::cout << r.first << "=" << r.second << std::endl;
   }
}
int main()
{
   auto redis = init();
   redis->flushall();
   assert(redis->set("username", "zhangsan"));
   assert(redis->set("nickname", "xiaozhang", std::chrono::milliseconds(3000)));

   std::vector<std::string> skills = {"C++", "Java", "Go", "Python"};
   redis->rpush("skills", skills.begin(), skills.end());

   std::unordered_map<std::string, std::string> fields = {
       {"age", "18"},
       {"sex", "male"},
       {"height", "175"},
       {"weight", "85"}
   };
   redis->hmset("properties", fields.begin(), fields.end());

   assert(redis->expire("username", std::chrono::seconds(3)));
   assert(redis->expire("skills", std::chrono::seconds(3)));
   assert(redis->expire("properties", std::chrono::seconds(3)));
   assert(redis->persist("username"));
   show(redis);

   std::this_thread::sleep_for(std::chrono::seconds(4));
   std::cout << "-------------4s later---------------";
   show(redis);
   return 0;
}
```

```makefile
main : main.cc
   g++ -std=c++17 $^ -o $@ -lredis++ -lhiredis -lgflags -pthread
```

## 封装

### 设计

#### redis服务器配置结构

##### 成员：

- 服务器IP地址
- 服务器访问端口
- 访问用户名
- 访问密码
- 库号
- 连接池最大数量

#### redis工厂类

##### 方法：

- 创建Redis对象
