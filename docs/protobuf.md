# Protobuf 快速上手

Protobuf 用 `.proto` 描述数据结构，再生成各语言的消息类，适合二进制数据交换和 RPC 消息定义。

## 安装与基本组成

Ubuntu 开发包为 `protobuf-compiler`、`libprotobuf-dev`。需要生成 gRPC 代码时，另选 `protobuf-compiler-grpc` 及配套 gRPC 依赖。

| 组成                 | 含义                                   |
| -------------------- | -------------------------------------- |
| `message`          | 一种结构化消息                         |
| 字段类型与编号       | 类型说明值的形式，编号标识传输中的字段 |
| `enum`             | 一组具名的枚举值                       |
| `service`、`rpc` | 服务方法及其请求、响应类型             |

## 协议语法

使用 `proto3`。`package` 对应 C++ 命名空间：`example` 对应 `example::`，点分隔的包名对应多层命名空间。

| 字段形式          | 用途与访问方式                                                   |
| ----------------- | ---------------------------------------------------------------- |
| 普通标量          | 保存字符串、整数、浮点数等值，通过字段名和 `set_字段名` 访问   |
| `optional` 标量 | 记录值及其设置状态，使用 `has_字段名` 查询存在性               |
| `repeated`      | 保存列表，通过 `add_` 添加、下标读取、`字段名_size` 查询数量 |
| `map`           | 保存键值映射，通过 `mutable_字段名` 获取可写容器               |

补充：普通 proto3 标量以默认值表示缺省状态；需要区分“缺省”和“显式设置为零或空字符串”时选择 `optional`。显式设置的 `optional` 零值会保留在 JSON 中；列表和映射通过元素数量判断内容。[字段存在性说明](https://protobuf.dev/programming-guides/field_presence/)

## service 生成选项

| `cc_generic_services` | 行为                                         | 补充：适用场景                                  |
| ----------------------- | -------------------------------------------- | ----------------------------------------------- |
| `true`                | 根据 `service` 生成通用服务类和客户端 Stub | RPC 框架基于 `google::protobuf::Service` 工作 |
| `false`               | 保留消息类的生成，服务代码交给专用插件       | 纯消息序列化，或采用 gRPC 等专用生成器          |

通用服务类描述调用接口，Stub 通过 `RpcChannel` 发起调用，网络收发由 RPC 框架处理。example中两份 `Person` 协议只含消息定义，采用默认的 `false` 即可完成演示。[服务代码生成说明](https://protobuf.dev/reference/cpp/cpp-generated/#services)

## protoc 与 C++ 构建

| 编译参数                                 | 作用                                        |
| ---------------------------------------- | ------------------------------------------- |
| `-I`、`--proto_path`                 | 指定协议文件搜索根目录，可设置多个目录      |
| `--cpp_out`                            | 指定生成的 C++ 文件存放位置                 |
| `--experimental_allow_proto3_optional` | 为  Protobuf 3.12 启用 proto3 `optional` |

生成文件沿用协议文件名：`person.proto` 生成 `person.pb.h` 和 `person.pb.cc`。`package` 决定类所在的命名空间，文件路径决定生成文件的相对位置。[文件生成规则](https://protobuf.dev/reference/cpp/cpp-generated/#compiler-invocation)

构建分为两个阶段：先用 `protoc` 生成代码，再将业务源文件与 `.pb.cc` 一起编译并链接 `protobuf`。编译器的头文件搜索路径应包含 `.pb.h` 所在目录。

## Message 常用接口

默认生成的消息类继承 `google::protobuf::Message`，字段访问接口由协议定义生成，通用接口由基类提供。

| 接口                                       | 用途                             |
| ------------------------------------------ | -------------------------------- |
| `GetTypeName`                            | 获取完整消息类型名               |
| `descriptor`、`GetDescriptor`          | 获取消息及字段的描述信息         |
| `GetReflection`                          | 通过描述信息操作字段             |
| `Clear`、`CopyFrom`、`Swap`          | 清空、复制、交换消息             |
| `SerializeToString`、`ParseFromString` | 通过字符串保存或解析二进制数据   |
| `SerializeToArray`、`ParseFromArray`   | 通过指定地址和长度操作二进制数据 |
| `SerializeAsString`                      | 直接取得二进制字符串             |

补充：接口报文已存放在内存缓冲区时选择 Array 系列；通过 `std::string` 传递数据时选择 String 系列。输出缓冲区按 `ByteSizeLong` 准备容量。

需要明确判断序列化结果时，使用 `SerializeToString` 的布尔返回值。空消息可以编码为零字节。解析接口返回 `true` 后读取目标对象，返回 `false` 时处理格式或协议错误。

## Map 与列表

`google::protobuf::Map` 提供 `begin`、`end`、`size`、`empty`、`clear`、`find`、`insert`、下标和 `erase`。通过 `mutable_scores` 取得可写成绩映射，通过 `scores` 读取映射；`MapPair` 可构造插入项。

`skills_size` 获取技能数量，`skills` 按下标读取，`add_skills` 追加，`set_skills` 修改指定元素，`mutable_skills` 提供可写入口，`clear_skills` 清空列表。父消息管理这些字段的存储。

## Protobuf 与 JSON 转换

头文件为 `google/protobuf/util/json_util.h`。输出配置使用 `JsonPrintOptions`，`JsonOptions` 是其别名；解析配置使用 `JsonParseOptions`。

| 接口                    | 数据方向                                  |
| ----------------------- | ----------------------------------------- |
| `MessageToJsonString` | Protobuf 对象转换为 JSON 字符串           |
| `JsonStringToMessage` | JSON 字符串转换为指定类型的 Protobuf 对象 |

返回的 `Status` 表示转换状态，结果写入传入的输出对象。`ok` 为 `true` 后使用结果，为 `false` 时通过 `ToString` 查看原因；`message` 提供错误说明。

| 输出选项                          | 补充：选择 `true`                    | 补充：选择 `false`                                          |
| --------------------------------- | -------------------------------------- | ------------------------------------------------------------- |
| `add_whitespace`                | 添加缩进和换行，便于阅读               | 输出紧凑文本，适合传输                                        |
| `always_print_primitive_fields` | 输出普通标量的默认值，适合固定字段接口 | 采用默认的字段省略规则，缩小报文                              |
| `always_print_enums_as_ints`    | 使用枚举数值，适合数值约定的接口       | 使用枚举名称，便于阅读                                        |
| `preserve_proto_field_names`    | 保留协议中的字段名                     | 使用 ProtoJSON 的小驼峰字段名，即首词小写、后续单词首字母大写 |

`optional` 字段按自身的设置状态输出。输出选项在传入转换函数后生效。相关定义见 [JSON 转换接口](https://protobuf.dev/reference/cpp/api-docs/google.protobuf.util.json_util/)。

| 解析选项                          | 补充：选择 `true`                    | 补充：选择 `false`                 |
| --------------------------------- | -------------------------------------- | ------------------------------------ |
| `ignore_unknown_fields`         | 跳过协议之外的字段，适合兼容较新发送方 | 遇到额外字段时报告错误，适合严格校验 |
| `case_insensitive_enum_parsing` | 兼容已有输入中的枚举大小写差异         | 按枚举名称精确匹配                   |

## 本仓库示例

在项目根目录执行 `cmake -S . -B build`，再通过 `cmake --build build --target 目标名` 构建。产物位于 `build/bin`。

| 目标名                 | PDF 对应内容                                                             |
| ---------------------- | ------------------------------------------------------------------------ |
| `protobuf_serialize` | 构造 `Person`，完成二进制编码，再解析并读取字段                        |
| `protobuf_json`      | Protobuf 与 JSON 双向转换，观察 `optional age` 显式设置为 `0` 的输出 |

JSON 示例中，`p2j` 接收 `Message` 基类引用并返回 JSON；`j2p` 在函数内部创建 `Person`，解析 JSON 后读取字段，最后调用 `p2j` 再次输出，便于核对转换结果。
