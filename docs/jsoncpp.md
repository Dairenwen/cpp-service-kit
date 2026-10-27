# JsonCpp 快速上手

JsonCpp 在 C++ 对象与 JSON 文本之间转换数据，适合接口报文和配置文件。

## JSON 基础

对象用花括号组织键值对，数组用方括号组织有序元素。键和字符串使用双引号，键值之间使用冒号，元素之间使用逗号。布尔值为小写 `true`、`false`，空值为 `null`。

## 安装与接入

Ubuntu 开发包为 `libjsoncpp-dev`，头文件为 `jsoncpp/json/json.h`，链接库为 `jsoncpp`。本仓库使用 CMake 目标 `JsonCpp::JsonCpp`。

## Json::Value 数据转储

`Json::Value` 保存任意一种 JSON 值，其类型包括 `nullValue`、`intValue`、`uintValue`、`realValue`、`stringValue`、`booleanValue`、`arrayValue`、`objectValue`。

| 数据 | 类型检查 | 读取接口 |
| --- | --- | --- |
| 字符串 | `isString` | `asString`、`asCString` |
| 整数 | `isInt`、`isUInt`、`isInt64`、`isUInt64`、`isIntegral` | 对应的 `asInt`、`asUInt`、`asInt64`、`asUInt64` |
| 浮点数 | `isDouble`、`isNumeric` | `asFloat`、`asDouble` |
| 布尔值 | `isBool` | `asBool` |
| 数组 | `isArray` | 下标访问 |
| 对象 | `isObject` | 成员名访问 |
| 空值 | `isNull` | 按接口约定处理空值 |

读取前先检查类型与数值范围，再调用对应转换接口。`asCString` 返回由该值管理的字符指针；需要独立保存文本时使用 `asString`。

## 对象和数组操作

| 操作 | 接口 |
| --- | --- |
| 读取或设置对象成员 | 字符串下标 |
| 判断键是否存在、删除键 | `isMember`、`removeMember` |
| 读取或设置数组元素 | 数值下标 |
| 追加元素、调整数组长度 | `append`、`resize` |
| 查询数量、判断空容器 | `size`、`empty` |
| 清理对象或数组 | `clear` |
| 复制或交换值 | 赋值、`copy`、`swap` |

补充：`isMember` 查询键的存在状态，`isNull` 查询值是否为 `null`；显式空值与缺省字段由这两个接口区分。可写下标访问新键时会创建 `null` 成员，纯查询适合使用只读对象。

## 序列化

先组织 `Json::Value`，再配置 `StreamWriterBuilder`。通过 `newStreamWriter` 创建写入器，由 `std::unique_ptr` 管理其生命周期。

`StreamWriter::write` 将数据写入输出流，返回 `0` 表示成功；`writeString` 直接返回字符串。`toStyledString` 适合临时查看对象内容。

| 输出选项 | 补充：取值与场景 |
| --- | --- |
| `indentation` | 空字符串用于紧凑报文；空格或制表符用于人工阅读 |
| `commentStyle` | `None` 用于接口报文；`All` 用于保留已有注释的配置文件 |

写入配置通过构建器设置，随后创建写入器。配置项说明见 [StreamWriterBuilder 文档](https://open-source-parsers.github.io/jsoncpp-docs/doxygen/class_json_1_1_stream_writer_builder.html)。

## 反序列化

`CharReaderBuilder` 创建 `CharReader`，同样适合交给 `std::unique_ptr` 管理。`parse` 接收文本起始地址、末尾后一位地址、结果对象地址和错误文本地址。

返回 `true` 表示解析成功；返回 `false` 时，错误文本说明失败位置。参数配置或类型转换抛出的 `Json::Exception` 交给上层记录和处理。

## 本仓库示例

在项目根目录执行 `cmake -S . -B build`，再通过 `cmake --build build --target 目标名` 构建 `jsoncpp_serialize` 或 `jsoncpp_unserialize`。产物位于 `build/bin`。

`example/jsoncpp/serialize` 将个人信息组织为 JSON，包含技能数组和嵌套地址对象。`example/jsoncpp/unserialize` 解析同类文本，逐项读取字段，并通过字段检查处理可选的 `weight`。
