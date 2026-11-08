# BreezeKit 使用说明

BreezeKit 提供 C++17 服务开发常用模块。发布包适用于 Ubuntu 22.04 x86_64、GCC 11 环境；使用方安装所选模块的第三方开发库与头文件，并使用兼容的编译环境。

## 基础接入

- **构建和打包**：在项目根目录执行以下命令。示例与测试通过 `BREEZE_BUILD_EXAMPLES` 和 `BREEZE_BUILD_TESTS` 开启。

  ```bash
  cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
  cmake --build build -j
  cpack --config build/CPackConfig.cmake -B build
  ```
- **安装位置**：将生成的 `BreezeKit-0.1.0-Linux.tar.gz` 解压到 `~/opt`。包中包含 `include/BreezeKit`、`lib`、CMake 配置、说明和许可证。通过 `cmake --install build --prefix "$HOME/.local"` 也可以安装到个人目录。

  ```bash
  mkdir -p "$HOME/opt"
  tar -xzf build/BreezeKit-0.1.0-Linux.tar.gz -C "$HOME/opt"
  ```
- **CMake 项目**：在使用方项目中选择所需组件，例如邮件和 Redis：

  ```cmake
  cmake_minimum_required(VERSION 3.20)
  project(my_app LANGUAGES CXX)

  find_package(BreezeKit CONFIG REQUIRED COMPONENTS code redis)
  add_executable(my_app main.cpp)
  target_link_libraries(my_app PRIVATE BreezeKit::code BreezeKit::redis)
  ```

  ```bash
  cmake -S . -B build -DCMAKE_PREFIX_PATH="$HOME/opt/BreezeKit-0.1.0-Linux"
  cmake --build build
  ```

  项目头文件使用 `#include <BreezeKit/BreezeCode.hpp>` 的形式。静态库中的第三方依赖由 CMake 目标传递给使用方，使用方准备对应的开发库和兼容的运行环境。

## 日志与工具

- **日志**：组件 `log`，头文件 `<BreezeKit/BreezeLog.hpp>`，依赖 spdlog、fmt 和线程库。

  ```cpp
  BreezeLog::Logger::setLevel(spdlog::level::info);
  INF("服务启动，端口 {}", 8080);
  ```
- **工具**：组件 `util`，头文件 `<BreezeKit/BreezeUtil.hpp>`，依赖 JsonCpp、Boost、spdlog 和线程库；提供 JSON、文件、字符串和随机标识工具。

  ```cpp
  auto value = BreezeUtil::JSON::unserialize(R"({"name":"Alice"})");
  auto id = BreezeUtil::Random::code();
  ```

## RPC 与 etcd

- **RPC**：组件 `rpc`，头文件 `<BreezeKit/BreezeRPC.hpp>`，依赖 brpc、Protobuf、gflags、LevelDB、OpenSSL、zlib 和线程库。示例地址根据实际服务替换：

  ```cpp
  Channels channels;
  channels.insert("127.0.0.1:8000");
  auto channel = channels.select();
  ```

  服务接口通过 Protobuf 编写并生成 C++ 代码；`RpcServerFactory::create()` 接收堆上创建的服务对象和监听端口。
- **etcd**：组件 `etcd`，头文件 `<BreezeKit/BreezeEtcd.hpp>`，依赖 etcd-cpp-api、Boost、JsonCpp 和 spdlog。下面注册示例服务；`provider` 的生命周期覆盖服务运行期间。

  ```cpp
  SvcProvider provider("http://127.0.0.1:2379", "user", "127.0.0.1:8000");
  bool registered = provider.registry();
  ```

  `SvcWatcher` 接收上线与下线回调，通过 `watch()` 读取并监听服务节点。

## 消息队列

- 组件 `mq`，头文件 `<BreezeKit/BreezeMQ.hpp>`，依赖 AMQP-CPP、libev 和 spdlog。RabbitMQ 地址、用户名与密码使用实际环境配置。

  ```cpp
  auto client = MQFactory::create<MQClient>("amqp://guest:guest@127.0.0.1:5672/");
  declare_settings settings;
  settings.exchange = "events";
  settings.exchange_type = DIRECT;
  settings.queue = "events.queue";
  settings.binding_key = "created";
  Publisher publisher(client, settings);
  publisher.publish("hello");
  ```

  `Subscriber` 使用同一份声明配置，通过 `consume()` 注册消息回调。

## FastDFS

- 组件 `fdfs`，头文件 `<BreezeKit/BreezeFDFS.hpp>`，依赖 FastDFS 客户端、libfastcommon 和 spdlog。程序启动时初始化一次，业务调用结束后销毁一次。

  ```cpp
  Breezefdfs::fdfs_settings settings;
  settings.trackers = {"127.0.0.1:22122"};
  Breezefdfs::FDFSClient::init(settings);
  auto id = Breezefdfs::FDFSClient::upload_from_file("example.txt");
  if (id) {
      Breezefdfs::FDFSClient::download_to_file(*id, "download.txt");
      Breezefdfs::FDFSClient::remove(*id);
  }
  Breezefdfs::FDFSClient::destroy();
  ```

  `upload_from_buff()` 和 `download_to_buff()` 提供内存数据操作。

## Redis

- 组件 `redis`，头文件 `<BreezeKit/BreezeRedis.hpp>`，依赖 redis-plus-plus、hiredis 和线程库。连接池默认容量为 3，密码通过环境变量提供。

  ```cpp
  #include <cstdlib>
  Breezeredis::redis_settings settings;
  settings.host = "127.0.0.1";
  settings.passwd = std::getenv("REDIS_PASSWORD");
  auto redis = Breezeredis::RedisFactory::create(settings);
  redis->set("example:name", "Alice");
  auto name = redis->get("example:name");
  redis->del("example:name");
  ```

## MySQL 与 ODB

- 组件 `odb`，头文件 `<BreezeKit/BreezeODB.hpp>`，依赖 ODB、ODB MySQL 运行库和 MySQL 客户端开发库。业务对象使用 ODB 编译器生成映射代码，并将生成文件加入使用方构建。

  ```cpp
  #include <cstdlib>
  BreezeODB::mysql_settings settings;
  settings.host = "127.0.0.1";
  settings.user = "app";
  settings.passwd = std::getenv("MYSQL_PASSWORD");
  settings.db = "app_db";
  auto database = BreezeODB::DBFactory::mysql(settings);
  ```

  通过 ODB 事务与生成的对象映射执行增删改查。

## Elasticsearch

- 组件 `elastic`，头文件 `<BreezeKit/BreezeElastic.hpp>`，依赖 elasticlient、CPR、JsonCpp 和 spdlog。服务地址和认证信息根据实际环境配置。

  ```cpp
  auto client = BreezeElastic::ESClientFactory::create({"http://127.0.0.1:9200/"});
  BreezeElastic::Inserter doc("students", "student_1");
  doc.add("name", "Alice");
  doc.add("age", 20);
  bool saved = client->insert(doc);
  BreezeElastic::Searcher query("students");
  query.query()->must()->term("name")->setValue("Alice");
  auto result = client->search(query);
  BreezeElastic::Deleter item("students", "student_1");
  client->remove(item);
  ```

  `Indexer` 配置索引映射；服务版本与索引创建接口的适配情况请结合 Elasticsearch 环境验证。

## 邮件

- 组件 `code`，头文件 `<BreezeKit/BreezeCode.hpp>`，依赖 libcurl 和 spdlog。SMTP 用户名和授权码通过环境变量提供，HTML 模板通过 `{{code}}` 接收验证码。

  ```cpp
  #include <cstdlib>
  Breezecode::mail_settings settings;
  settings.username = std::getenv("QQ_MAIL_USER");
  settings.password = std::getenv("QQ_MAIL_AUTH_CODE");
  settings.url = "smtps://smtp.qq.com:465";
  settings.from = settings.username;
  settings.login_options = "AUTH=LOGIN";
  settings.html_path = "mail.html";
  settings.attachment_path = "attachment.png"; // 需要附件时填写路径
  Breezecode::MailClient mail(settings);
  bool sent = mail.send("recipient@example.com", "5678");
  ```

  模板示例：`<html><body>验证码：<b>{{code}}</b></body></html>`。收件地址、模板路径和附件路径由调用方提供。

## 音视频转码

- 组件 `trans`，头文件 `<BreezeKit/BreezeTrans.hpp>`，依赖 FFmpeg 的 libavformat、libavcodec、libavutil，以及 JsonCpp 和 spdlog。HLS 输出目录由使用方提前创建。

  ```cpp
  trans_settings settings;
  settings._hls_time = 5;
  settings._hls_base_url = "/video/";
  bool success = HLSTranscoder::transcode("input.mp4", "output/index.m3u8", settings);
  ```
  `M3U8Info` 提供播放列表的读取和写入。
