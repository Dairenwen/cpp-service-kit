# Curl邮件通知服务

libcurl 是一个跨平台、开源的客户端网络传输库，支持多种协议（如 HTTP、HTTPS、FTP、SMTP等），广泛应用于网络通信、数据抓取、API 交互等场景。

### 安装

```bash
sudo apt install libcurl4-openssl-dev
```

### 启用邮箱授权码

#### QQ邮箱

官网地址：https://mail.qq.com/，点击"继续获取授权码"，跳转页面进行手机号绑定及验证，验证完毕后进入页面，点击"生成授权码"，进入短信验证弹框进行短信验证，获得授权码

#### 163邮箱

官网地址：https://mail.163.com/

## 使用

### 头文件

```cpp
#include <curl/curl.h>
```

### 链接库

```bash
-lcurl -lssl -lcrypto
```

### 核心接口

```cpp
#define CURL_GLOBAL_SSL (1<<0) /* no purpose since since 7.57.0 */
#define CURL_GLOBAL_WIN32 (1<<1)
#define CURL_GLOBAL_ALL (CURL_GLOBAL_SSL|CURL_GLOBAL_WIN32)
#define CURL_GLOBAL_NOTHING 0
#define CURL_GLOBAL_DEFAULT CURL_GLOBAL_ALL
#define CURL_GLOBAL_ACK_EINTR (1<<2)
// 初始化全局配置
CURLcode curl_global_init(long flags);
// 销毁全局配置
void curl_global_cleanup(void);

// 创建并初始化curl操作句柄
CURL *curl_easy_init(void);
// 设置curl操作选项
CURLcode curl_easy_setopt(CURL *curl, CURLoption option, ...);
// 同步阻塞函数，会一次性执行所有通过curl_easy_setopt 设置的选项
//  如： URL,请求方法,回调函数等
CURLcode curl_easy_perform(CURL *curl);
// 仅清除通过 curl_easy_setopt 设置的选项，恢复为初始状态
void curl_easy_reset(CURL *curl);
// 关闭所有与该句柄相关的连接，释放所有资源
void curl_easy_cleanup(CURL *curl);
// 获取上次函数执行错误信息
const char *curl_easy_strerror(CURLcode);
```

### 链表操作

```cpp
struct curl_slist {
  char *data;
  struct curl_slist *next;
};
struct curl_slist *curl_slist_append(struct curl_slist *, const char *);
void curl_slist_free_all(struct curl_slist *);
```

### 常用选项

#### HTTP选项

| 选项 | 功能 | 备注 |
| --- | --- | --- |
| CURLOPT_URL | 设置请求 URL | 所有请求的基础配置 |
| CURLOPT_READFUNCTION | 处理请求数据的回调函数 | 添加请求正文 |
| CURLOPT_WRITEDATA | 设置给请求回调函数的用户数据 | 文件句柄或数据对象 |
| CURLOPT_WRITEFUNCTION | 处理响应数据的回调函数 | 下载文件或保存 API 响应 |
| CURLOPT_POSTFIELDS | 指定 POST 请求正文 | 提交表单或 JSON 数据 |
| CURLOPT_HTTPHEADER | 自定义 HTTP 头部 | 设置认证头或内容类型 |
| CURLOPT_FOLLOWLOCATION | 自动跟随重定向 | 处理短链接或跳转页面 |
| CURLOPT_VERBOSE | 输出调试信息 | 开发阶段问题排查 |
| CURLOPT_SSL_VERIFYPEER | 控制是否验证对端证书 | 生产环境不推荐使用 |
| CURLOPT_TIMEOUT | 设置传输超时时间 | 请求到完成 |
| CURLOPT_CONNECTTIMEOUT | 设置连接超时时间 | 握手超时时间 |

#### SMTP选项

| 选项 | 功能 | 典型值示例 |
| --- | --- | --- |
| CURLOPT_URL | SMTP 服务器地址 | "smtps://smtp.example.com:465" |
| CURLOPT_MAIL_FROM | 发件人地址 | `"<user@example.com>"` |
| CURLOPT_MAIL_RCPT | 收件人列表（链表） | `curl_slist_append(recipients, "<to@example.com>")` |
| CURLOPT_READFUNCTION | 处理请求数据的回调函数 | 添加请求正文 |
| CURLOPT_READDATA | 通信正文内容 | 代码块 |
| CURLOPT_SSL_VERIFYPEER | 启用服务器证书验证 | 1L （启用）或 0L （禁用） |
| CURLOPT_USERNAME | SMTP 登录用户名 | "user@example.com" |
| CURLOPT_PASSWORD | SMTP登录密码 |  |
| CURLOPT_USE_SSL | 控制 SSL/TLS 加密行为 |  |
| CURLOPT_UPLOAD | 启用上传模式 | 1L（启用）或 0L（禁用），与 CURLOPT_READDATA & CURLOPT_READFUNCTION 搭配使用 |
```text
From: from@example.com\r\n
To:to@example.com\r\n
Subject: title\r\n
Content-Type: text/html\r\n
\r\n
Body\r\n
```

## 封装

### 设计

#### 邮件服务配置结构

- 邮件服务器

- 访问用户

- 访问密码

- 发送者列表邮箱

#### 验证码客户端基类

- 方法：虚函数-发送验证码

#### 邮件客户端派生类

- 成员：邮件服务配置信息，验证码标题

- 方法：基于邮件发送验证码

