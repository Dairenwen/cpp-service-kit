# 17. ODB-Mysql使用样例

ODB（Object-Relational Bridge）是一个开源的C++对象关系映射（ORM）框架，旨在简化数据库操作，允许开发者以面向对象的方式直接操作数据库，而无需手动编写SQL语句。它通过自动映射C++类与数据库表，实现对象持久化，支持多种数据库后端（如MySQL、PostgreSQL、SQLite等）

核心目标：

• 类型安全：所有数据库操作均通过C++对象完成，避免SQL注入和类型转换错误。

• 开发效率：自动生成SQL代码，减少重复劳动，开发者可专注于业务逻辑

## 安装：

官网链接

• https://github.com/codesynthesis-com/odb.git

• https://codesynthesis.com/products/odb/doc/manual.xhtml

• https://codesynthesis.com/products/odb/

• https://codesynthesis.com/products/odb/doc/install-build2.xhtml

### 安装build2：

因为build2安装时，有可能会版本更新，从16变成17，或从17变18，因此注意，先从build2官网查看安装步骤...

安装步骤：https://build2.org/install.xhtml#unix

```text
dev@dev-host:~/workspace$ curl -sSfO https://download.build2.org/0.17.0/build2-install-0.17.0.sh
dev@dev-host:~/workspace$ sh build2-install-0.17.0.sh
```

安装中因为网络问题，超时失败，解决：将超时时间设置的更长一些

```text
Continue? [y/n] y
+ rm -rf build2-toolchain-0.16
+ rm -rf build2-toolchain-0.16.0
+ curl -fLO --connect-timeout 60 --max-time 600 --progress-bar https://download.build2.org/0.16.0/build2-toolchain-0.16.0.tar.xz
97.8%curl: (28) Operation timed out after 600001 milliseconds with 5423104 out of 5545392 bytes received
```

```text
dev@dev-host:~/workspace$ sh build2-install-0.17.0.sh --timeout 1800
```

### 安装odb-compiler

```text
dev@dev-host:~/workspace$ #注意这里的gcc-11需要根据你自己版本而定
dev@dev-host:~/workspace$ sudo apt-get install gcc-11-plugin-dev
dev@dev-host:~/workspace$ mkdir odb-build && cd odb-build
dev@dev-host:~/workspace/odb-build$ bpkg create -d odb-gcc-N cc     \
 config.cxx=g++                  \
 config.cc.coptions=-O3          \
 config.bin.rpath=/usr/lib \
 config.install.root=/usr/  \
 config.install.sudo=sudo
dev@dev-host:~/workspace/odb-build$ cd odb-gcc-N
dev@dev-host:~/workspace/odb-build/odb-gcc-N$ bpkg build odb@https://pkg.cppget.org/1/beta
dev@dev-host:~/workspace/odb-build/odb-gcc-N$ bpkg test odb
test odb-2.5.0-b.25+1/tests/testscript{testscript}
tested odb/2.5.0-b.25+1
dev@dev-host:~/workspace/odb-build/odb-gcc-N$ bpkg install odb
dev@dev-host:~/workspace/odb-build/odb-gcc-N$ odb --version
bash: /usr/bin/odb: No such file or directory
#如果报错了，找不到odb，那就在执行下边的命令
dev@dev-host:~/workspace/odb-build/odb-gcc-N$ sudo echo 'export PATH=${PATH}:/usr/local/bin' >> ~/.bashrc 
dev@dev-host:~/workspace/odb-build/odb-gcc-N$ export PATH=${PATH}:/usr/local/bin
dev@dev-host:~/workspace/odb-build/odb-gcc-N$ odb --version
ODB object-relational mapping (ORM) compiler for C++ 2.5.0-b.25
Copyright (c) 2009-2023 Code Synthesis Tools CC.
This is free software; see the source for copying conditions. There is NO
warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

```

### 安装ODB运行时库：

```text
dev@dev-host:~/workspace/odb-build/odb-gcc-N$ cd ..
dev@dev-host:~/workspace/odb-build$ bpkg create -d libodb-gcc-N cc  \
 config.cxx=g++                  \
 config.cc.coptions=-O3          \
 config.install.root=/usr/  \
 config.install.sudo=sudo
dev@dev-host:~/workspace/odb-build$ cd libodb-gcc-N
dev@dev-host:~/workspace/odb-build/libodb-gcc-N$ bpkg add https://pkg.cppget.org/1/beta
dev@dev-host:~/workspace/odb-build/libodb-gcc-N$ bpkg fetch
dev@dev-host:~/workspace/odb-build/libodb-gcc-N$ bpkg build libodb
dev@dev-host:~/workspace/odb-build/libodb-gcc-N$ bpkg build libodb-mysql

```

### 安装和配置mysql和客户端开发包

```text
sudo apt install mysql-server
sudo apt install -y libmysqlclient-dev
```

配置mysql

```text
sudo vim /etc/my.cnf  或者  /etc/mysql/my.cnf 有哪个修改哪个就行
#添加以下内容
[client]
default-character-set=utf8
[mysql]
default-character-set=utf8
[mysqld]
character-set-server=utf8
bind-address = 0.0.0.0

```

修改root用户密码

```text
dev@bite:~$ sudo cat /etc/mysql/debian.cnf
# Automatically generated for Debian scripts. DO NOT TOUCH!
[client]
host     = localhost
user     = debian-sys-maint
password = UWcn9vY0NkrbJMRC
socket   = /var/run/mysqld/mysqld.sock
[mysql_upgrade]
host     = localhost
user     = debian-sys-maint
password = UWcn9vY0NkrbJMRC
socket   = /var/run/mysqld/mysqld.sock
dev@bite:~$ sudo mysql -u debian-sys-maint -p
Enter password: #这里输入上边第6行看到的密码
mysql> ALTER USER 'root'@'localhost' IDENTIFIED WITH mysql_native_password BY 'xxxxxx';
Query OK, 0 rows affected (0.01 sec)

mysql> FLUSH PRIVILEGES;
Query OK, 0 rows affected (0.01 sec)

mysql> quit
```

重启mysql，并设置开机启动

```text
sudo systemctl restart mysql
sudo systemctl enable mysql
```

### 安装boost profile库

```text
dev@dev-host:~/workspace/odb-build/libodb-gcc-N$ bpkg build libodb-boost
```

### 总体打包安装：

```text
dev@dev-host:~/workspace/odb-build/libodb-gcc-N$ bpkg install --all --recursive
```

### 总体卸载：

```text
dev@dev-host:~/workspace/odb-build/libodb-gcc-N$ bpkg uninstall --all --recursive
```

### 总体升级：

```text
dev@dev-host:~/workspace/odb-build/libodb-gcc-N$ bpkg fetch
dev@dev-host:~/workspace/odb-build/libodb-gcc-N$ bpkg status
dev@dev-host:~/workspace/odb-build/libodb-gcc-N$ bpkg uninstall --all --recursive
dev@dev-host:~/workspace/odb-build/libodb-gcc-N$ bpkg build --upgrade --recursive
dev@dev-host:~/workspace/odb-build/libodb-gcc-N$ bpkg install --all --recursive
```

## ODB常见操作介绍：

### ODB类型映射：


| C++ Type | MySQL Type | Default NULL Semantics |
|---|---|---|
| bool | TINYINT(1) | NOT NULL |
| char | CHAR(1) | NOT NULL |
| signed char | TINYINT | NOT NULL |
| unsigned char | TINYINT UNSIGNED | NOT NULL |
| short | SMALLINT | NOT NULL |
| unsigned short | SMALLINT UNSIGNED | NOT NULL |
| int | INT | NOT NULL |
| unsigned int | INT UNSIGNED | NOT NULL |
| long | BIGINT | NOT NULL |
| unsigned long | BIGINT UNSIGNED | NOT NULL |
| long long | BIGINT | NOT NULL |
| unsigned long long | BIGINT UNSIGNED | NOT NULL |
| float | FLOAT | NOT NULL |
| double | DOUBLE | NOT NULL |
| std::string | TEXT/VARCHAR(255) | NOT NULL |
| char[N] | VARCHAR(N-1) | NOT NULL |

| Boost date_time Type | MySQL Type | Default NULL Semantics |
|---|---|---|
| gregorian::date | DATE | NULL |
| posix_time::ptime | DATETIME | NULL |
| posix_time::time_duration | TIME | NULL |

### ODB编程：

ODB（Open Database）在数据元结构定义时，使用预处理器指令**（#pragma ）**来提供元数据，这些元数据指示如何将C++类型映射到数据库模式。**这些#pragma 指令是在C++代码中使用的，它们不是C++语言的一部分，而是特定于ODB编译器的扩展**。以下是ODB中常用的一些#pragma 指令：

1. **#pragma db object:**用于声明一个类是数据库对象，即这个类将映射到数据库中的一个表。

2. **#pragma db table("table_name"):**指定类映射到数据库中的表名。如果不指定，**则默认使用类名，会将类名前后特殊字符自动去掉。**

3. **#pragma db id auto:**标记类中的一个成员变量作为数据库表的主键，auto则代表可以自增。

4. **#pragma db column("column_name"):**指定类成员映射到数据库表中的列名。如果不指定，则默认使用成员变量的名字，同样会会将前后特殊字符去掉。

5. **#pragma db view:**用于声明一个类是一个数据库视图，而不是一个表。

6. **#pragma db session:**用于声明一个全局或成员变量是数据库会话。

7. **#pragma db query("query"):**用于定义自定义的查询函数。

8. **#pragma db index("index_name"):**指定成员变量应该被索引。

9. **#pragma db default("default_value"):**指定成员变量的默认值。

10. **#pragma db unique:**指定成员变量或一组变量应该具有唯一性约束。

11. **#pragma db not_null:**指定成员变量不允许为空。`#pragma db null` 只规定数据库字段允许为空，而 `odb::nullable<T>` 让 C++ 对象本身也能表示“有值”和“无值”，从而避免数据含义丢失。


12. **#pragma db auto:**指定成员变量的值在插入时自动生成（例如，自动递增的主键）。

13. **#pragma db transient**:指定成员变量不应该被持久化到数据库中。

14. **#pragma db type("type_name"):**指定成员变量的数据库类型。

15. **#pragma db convert("converter"):**指定用于成员变量的自定义类型转换器。

16. **#pragma db pool("pool_name"):**指定用于数据库连接的连接池。

17. **#pragma db trigger("trigger_name"):**指定在插入、更新或删除操作时触发的触发器。

### 数据操作相关类与接口：

#### 代码生成指令

```bash
odb -d mysql --std c++11 --generate-query --generate-schema --profile boost/date-time
```

#### odb数据对象头文件

```cpp
#pragma once
#include <string>
#include <cstddef> // std::size_t
#include <boost/date_time/posix_time/posix_time.hpp>
#include <odb/nullable.hxx>
#include <odb/core.hxx>// 核心头文件
```

#### odb数据操作头文件

```cpp
#include <odb/database.hxx>
#include <odb/mysql/transaction.hxx>
#include <odb/mysql/database.hxx>
#include "xxx.h"
#include "xxx-odb.h"
```

#### 链接库

```bash
-lodb-mysql -lodb -lodb-boost  -lpthread
```

#### database与transaction

```cpp
namespace odb{
 namespace mysql {
   //mysql连接池对象类
   class LIBODB_MYSQL_EXPORT connection_pool_factory
       : public connection_factory {
       connection_pool_factory (std::size_t max_connections = 0,
                              std::size_t min_connections = 0,
                              bool ping = true)
   }
 }
 //操作句柄类
 class LIBODB_EXPORT database {
     database (const std::string& user,
               const std::string& passwd,
               const std::string& db,
               const std::string& host = "",
               unsigned int port = 0,
               const std::string* socket = 0,
               const std::string& charset = "",
               unsigned long client_flags = 0,
               details::transfer_ptr<connection_factory> =
                 details::transfer_ptr<connection_factory> ());
     //新增数据
     persist (T& object);
     //更新数据
     void update (T& object);
     void erase (T& object);
     unsigned long long erase_query (const std::string&);
     unsigned long long erase_query (const odb::query<T>&);
     result<T> query (const std::string&);
     result<T> query (const odb::query<T>&, bool cache = true);
     typename result<T>::pointer_type query_one(const odb::query<T>&);

     //odb中所有的数据操作都必须在事务内部完成
     // 该接口用于创建/获取一个事务对象
     virtual transaction_impl* begin () = 0;
 };
 //事务操作类
 class LIBODB_EXPORT transaction {
     transaction (transaction_impl*, bool make_current = true);
     database_type& database(); // 当前线程内部的输操作对象
     void commit ();
     void rollback ();
     void tracer (tracer_type& t)
 }；

}
```

#### query与result

```cpp

  //针对查询结果所封装的容器类
  template <typename T>
  class result: result_base<T, class_traits<T>::kind> {
      result ()
      result (const result& r)
      iterator begin ()
      iterator end ()
      size_type size ()
      bool empty ()
  }；
  //针对查询封装的条件类
  class LIBODB_EXPORT query_base {
      explicit query_base (const std::string& native)
      const clause_type& clause ()
  }；
  namespace mysql
  {
    template <typename T>
    class query: public query_base,
                 public query_selector<T, id_mysql>::columns_type {
         query (const std::string& q)
         query (const query_base& q)
         query (bool v)
    }
  }       
  template <typename T>
  class query<T, mysql::query_base>: public mysql::query<T>  {
      query (bool v)
      query (const std::string& q)
      query (const mysql::query_base& q)
  }
```

#### nullable

```cpp
  //针对可能为空的字段封装的类似于智能指针的类型
  template <typename T>
  class nullable {
      typedef T value_type;
      T&       get ();
      const T& get () const;

      T*       operator-> ();
      const T* operator-> () const;

      T&       operator* ();
      const T& operator* () const;
  }
```

#### sql追踪

```cpp
namespace odb
{
  namespace mysql
  {
    class LIBODB_MYSQL_EXPORT tracer: private odb::tracer
    {
      public:
      virtual
      ~tracer ();
      virtual void prepare (connection&, const statement&);
      virtual void execute (connection&, const statement&);
      virtual void execute (connection&, const char* statement) = 0;
      virtual void deallocate (connection&, const statement&);
    }

    class LIBODB_MYSQL_EXPORT statement: public odb::statement
    {
      public:
      virtual const char* text () const;
    }
  }

  extern LIBODB_EXPORT tracer& stderr_tracer;
  extern LIBODB_EXPORT tracer& stderr_full_tracer;
}
```

#### 异常

```cpp
namespace odb
{
  struct LIBODB_EXPORT exception: std::exception, details::shared_base
  {
    virtual const char* what () const ODB_NOTHROW_NOEXCEPT = 0;
    virtual exception* clone () const = 0;
  };

  namespace common
  {
    using odb::exception;
  }
}
```

## 封装

### 设计


#### 配置结构

• 服务器地址

• 服务器端口

• 访问用户名

• 访问密码

• 默认库名称

• 字符集

• 连接池最大连接数量

#### 工厂类

• 创建database对象
