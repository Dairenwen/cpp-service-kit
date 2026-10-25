<div align="center">


<div align="center">

---

## Overview

**CppServiceKit** 是一个面向现代 C++ 服务端开发的模块化脚手架项目。

项目旨在整合常用第三方库与中间件 SDK，构建统一、可复用、易扩展的服务端开发基础设施，减少重复性的环境配置与基础组件开发工作。

项目覆盖配置管理、日志系统、网络通信、数据序列化、单元测试、音视频处理及数据存储等典型开发场景，帮助开发者更加高效地构建 C++ 服务端应用。

## Features

- **Modular Architecture** — 模块化设计，支持组件独立集成与扩展。
- **Unified Development** — 统一第三方库的集成方式，降低使用成本。
- **Network Communication** — 集成 HTTP、WebSocket、RPC 等网络通信组件。
- **Data Serialization** — 支持 JSON、Protobuf 等数据序列化方案。
- **Middleware Integration** — 规划 Redis、MySQL、RabbitMQ、etcd 等中间件集成。
- **Engineering Practices** — 提供配置管理、日志记录、单元测试等基础工程能力。

## Tech Stack

| Category                          | Technologies      | 说明                                          |
| --------------------------------- | ----------------- | --------------------------------------------- |
| Configuration（配置管理）         | gflags            | 命令行参数解析与程序配置管理                  |
| Logging（日志管理）               | spdlog            | 高性能日志记录与输出                          |
| Testing（单元测试）               | Google Test       | C++ 单元测试与自动化测试框架                  |
| Serialization（序列化）           | JsonCpp, Protobuf | JSON 与 Protocol Buffers 数据序列化及反序列化 |
| HTTP（HTTP 通信）                 | cpp-httplib       | HTTP 客户端与服务器开发                       |
| WebSocket（实时通信）             | WebSocket++       | WebSocket 客户端与服务器开发                  |
| RPC（远程过程调用）               | brpc              | 高性能 RPC 客户端与服务器开发                 |
| Multimedia（多媒体处理）          | FFmpeg            | 音视频编解码、格式转换与媒体处理              |
| Database（数据库）                | MySQL, Redis      | 关系型数据存储与高性能键值存储                |
| Message Queue（消息队列）         | RabbitMQ          | 异步消息传递、任务队列与服务解耦              |
| Service Coordination（服务协调）  | etcd              | 分布式键值存储、服务发现与配置管理            |
| Distributed Storage（分布式存储） | FastDFS           | 分布式文件存储与管理                          |
| Search Engine（搜索引擎）         | Elasticsearch     | 全文检索、数据索引与搜索分析                  |
| Cloud Services（云服务）          | Alibaba Cloud SMS | 阿里云短信发送与短信服务集成                  |

## Quick Start

项目处于开发阶段。以下是计划采用的标准 CMake 构建流程，实际使用前需要确保仓库已配置相应的构建文件及依赖。

```bash
# Clone the repository
git clone https://github.com/YOUR_USERNAME/cpp-service-kit.git

# Enter the project
cd cpp-service-kit

# Configure
cmake -S . -B build

# Build
cmake --build build -j
```

## Roadmap

- [ ] 建立统一的 C++ 工程结构与构建配置
- [ ] 封装基础配置、日志和单元测试组件
- [ ] 集成 JSON、Protobuf 序列化组件
- [ ] 集成 HTTP、WebSocket 和 RPC 通信框架
- [ ] 封装常用数据库及中间件客户端
- [ ] 完善使用示例与技术文档

## Project Goals

CppServiceKit 致力于构建一个轻量、模块化、可复用的 C++ 服务端开发基础组件集合。

**Focus on Business Logic, Not Boilerplate.**

---

<div align="center">
