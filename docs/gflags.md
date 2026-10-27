# GFlags 快速上手

GFlags 将启动参数解析为带类型的变量，适合配置监听地址和日志选项。

## 安装与接入

Ubuntu 开发包为 `libgflags-dev`，头文件为 `gflags/gflags.h`，链接库为 `gflags`。定制版本可从上游源码构建，安装位置由 `CMAKE_INSTALL_PREFIX` 指定。

## 定义、声明与读取

`DEFINE_*` 的三个参数依次为选项名、默认值和帮助信息。每个选项集中定义一次，其他文件通过同类型的 `DECLARE_*` 声明引用；共享声明适合放在头文件中。

| 定义宏 | 类型与用途 |
| --- | --- |
| `DEFINE_bool` | 功能开关 |
| `DEFINE_int32` | 32 位有符号整数，如端口和数量 |
| `DEFINE_int64` | 64 位有符号整数，如时间戳 |
| `DEFINE_uint64` | 64 位整数，取值从 0 开始 |
| `DEFINE_double` | 浮点配置 |
| `DEFINE_string` | 地址、路径等文本 |

定义后通过 `FLAGS_选项名` 访问变量。`ParseCommandLineFlags` 通常在 `main` 开始处调用，解析完成后再使用运行参数。

## remove_flags 如何选择

`ParseCommandLineFlags` 接收 `argc` 和 `argv` 的地址，第三个参数控制解析后参数列表的组织方式。

| 取值 | 参数列表的变化 | 补充：适用场景 |
| --- | --- | --- |
| `true` | 移除已经解析的选项及其值，同时更新 `argc` | 后续逻辑只处理文件名等位置参数 |
| `false` | 保持 `argc`，将选项及其值移到位置参数前面 | 后续逻辑还要检查启动选项 |

两种取值都会更新 `FLAGS_*`。取 `false` 时，函数返回首个位置参数的下标。详见 [GFlags 参数解析说明](https://gflags.github.io/gflags/#together)。

## 命令行设置

| 参数形式 | 含义 |
| --- | --- |
| `--选项名=值`、`--选项名 值` | 设置字符串或数值 |
| `-选项名=值`、`-选项名 值` | 单横线形式，含义相同 |
| `--选项名`、`--选项名=true` | 将布尔选项设为 `true` |
| `--no选项名`、`--选项名=false` | 将布尔选项设为 `false` |
| `--` | 结束选项解析，其后的内容作为位置参数 |

布尔值表示应用定义的开关状态：启用某项功能选择 `true`，关闭该功能选择 `false`。字符串包含空格时，由 Shell 引号将其组织成一个参数。

## 配置文件

`--flagfile=路径` 从文件读取选项。每行写一项，名称与定义保持一致，名称和值使用等号连接。字符串直接写值，`#` 开头的行用作注释。

选项按出现顺序处理，后出现的赋值覆盖前面的赋值。希望命令行覆盖文件配置时，将覆盖项放在 `--flagfile` 后面。相对路径以进程工作目录为基准。配置文件加载规则见 [官方说明](https://gflags.github.io/gflags/#special)。

## 帮助与版本

| 选项 | 用途 |
| --- | --- |
| `--help`、`--helpfull` | 查看完整参数帮助 |
| `--helpshort` | 查看主程序源文件定义的参数 |
| `--helpxml` | 输出 XML 格式的帮助 |
| `--version` | 查看由 `SetVersionString` 设置的版本 |
| `--flagfile` | 指定参数配置文件 |

## 本仓库示例

本仓库使用 C++17。在项目根目录执行 `cmake -S . -B build`，再执行 `cmake --build build --target gflags_example`，产物为 `build/bin/gflags_example`。

`example/gflags/main.cc` 演示 `reuse_addr`、`log_level`、`log_file`。默认值依次为 `true`、`1`、`stdout`；`main.conf` 将日志等级设为 `3`，路径设为 `./log/main.log`。

该示例输出解析结果。套接字选项和日志器配置由后续业务代码应用；日志等级数字的含义以各程序的定义为准。
