# ElasticClient使用

Elasticsearch， 简称ES，它是个开源分布式搜索引擎，它的特点有：**分布式，零配置，自动发现，索引自动分片，索引副本机制，restful风格接口，多数据源，自动搜索负载**等。它可以近乎实时的存储、检索数据；本身扩展性很好，可以扩展到上百台服务器，处理PB级别的数据。es也使用Java开发并使用Lucene作为其核心来实现所有索引和搜索的功能，但是它的目的是通过简单的RESTful API来隐藏Lucene的复杂性，从而让全文搜索变得简单。

Elasticsearch是面向文档(document oriented)的，这意味着它可以存储整个对象或文档(document)。然而它不仅仅是存储，还会索引(index)每个文档的内容使之可以被搜索。在Elasticsearch中，你可以对文档（而非成行成列的数据）进行索引、搜索、排序、过滤。

### ES检索原理

ES 的全文检索主要依靠倒排索引，下面看看正排索引和倒排索引的区别：

#### 正排索引：

正排索引，也称为前向索引，是一种将文档或数据记录按照某种特定顺序组织的索引机制。在正排索引中，索引的键通常是文档的标识符，如文档ID，而索引的值则包含文档的详细信息，例如标题、内容摘要、发布日期等。这种结构使得正排索引非常适合执行基于特定标识符的查找操作。

正排索引的优点在于能够直接根据文档ID快速访问文档，适合于需要按照文档顺序进行操作的场景。

```mermaid
flowchart LR
    A["正排索引"] --> B["文档 ID：1"]
    A --> C["文档 ID：2"]
    B --> D["姓名：张三<br/>年龄：18"]
    C --> E["姓名：李四<br/>年龄：20"]
```

#### 倒排索引：

倒排索引，又称反向索引或逆向索引，是按照文档中的词汇来组织数据的索引方法。在倒排索引中，每个独特的词汇都会有一个索引条目，该条目包含指向包含该词汇的所有文档的指针或引用。这使得倒排索引非常适合全文搜索，能够快速找到包含特定关键词的文档。

倒排索引则适合于全文搜索，可以快速找到包含特定关键词的所有文档，索引的大小相对较小，**因为它只记录关键词和文档的映射关系。**但是，倒排索引不能直接通过索引访问文档，需要结合正排索引来获取文档的详细信息。

```mermaid
flowchart LR
    A["倒排索引"] --> B["关键词：张"]
    A --> C["关键词：三"]
    A --> D["关键词：李"]
    B --> E["文档 ID：1"]
    C --> E
    D --> F["文档 ID：2"]
```

## ES核心概念

Elasticsearch与传统关系型数据库的辅助理解相比如下：

#### 索引（Index）

一个索引就是一个拥有几分相似特征的文档的集合。比如说，你可以有一个客户数据的索引，一个产品目录的索引，还有一个订单数据的索引。一个索引由一个名字来标识（必须全部是小写字母的），并且当我们要对应于这个索引中的文档进行索引、搜索、更新和删除的时候，都要使用到这个名字。在一个集群中，可以定义任意多的索引。**可以类比为MySQL中的数据表。**

#### 类型（Type）

在一个索引中，可以定义一种或多种类型。一个类型是你的索引的一个逻辑上的分类/分区，其语义完全由你来定。通常，会为具有一组共同字段的文档定义一个类型。**旧版 ES 的文档分类机制，现已淘汰。**

#### 字段（Field）

字段相当于是数据表的字段，对文档数据根据不同属性进行的分类标识。**可以直接类比为MySQL中的列。**

字段的数据类型：

| 分类   |                                                类型                                                | 是否分词 | 默认建立索引 | 备注                                               |
| ------ | :------------------------------------------------------------------------------------------------: | -------- | ------------ | -------------------------------------------------- |
| 字符串 |                                              `text`                                              | 是       | 是           | 使用分词器生成倒排索引，支持全文检索               |
| 字符串 |                                            `keyword`                                            | 否       | 是           | 整体作为一个词项建立索引，适合精确匹配、排序和聚合 |
| 整型   |                             `integer`、`long`、`short`、`byte`                             | 否       | 是           | 支持精确查询、范围查询，通常使用 BKD Tree 索引     |
| 浮点   |                                       `double`、`float`                                       | 否       | 是           | 支持数值查询、范围查询，通常使用 BKD Tree 索引     |
| 逻辑   |                                            `boolean`                                            | 否       | 是           | 取值为 true 或 false，支持精确查询                 |
| 日期   |                                      `date`、`date_nanos`                                      | 否       | 是           | 支持日期范围查询，通常使用 BKD Tree 索引           |
| 二进制 |                                             `binary`                                             | 否       | 否           | Base64 编码的二进制数据，通常只存储，不支持搜索    |
| 范围   | `integer_range`、`long_range`、`float_range`、`double_range`、`date_range`、`ip_range` | 否       | 是           | 存储一个范围，支持范围之间的包含、相交等查询       |

#### 映射（mapping）

映射是在处理数据的方式和规则方面做一些限制，如某个字段的数据类型、默认值、分析器、是否被索引等等，这些都是映射里面可以设置的，其它就是处理es里面数据的一些使用规则设置也叫做映射，按着最优规则处理数据对性能提高很大，因此才需要建立映射，并且需要思考如何建立映射才能对性能更好。**可以直接类比为MySQL中的表结构定义。**

| 名称           | 数值                                                                               | 备注                                                                                                                       |
| -------------- | ---------------------------------------------------------------------------------- | -------------------------------------------------------------------------------------------------------------------------- |
| enabled        | true(默认) \| false                                                                | 是否仅作存储，不做搜索和分析                                                                                               |
| index          | true(默认) \| false                                                                | 是否构建倒排索引（决定了是否分词，是否被索引）                                                                             |
| index_option   |                                                                                    |                                                                                                                            |
| dynamic        | true（缺省）\| false                                                               | 控制mapping的自动更新                                                                                                      |
| doc_value      | true(默认) \| false                                                                | 是否开启doc_value，用于聚合和排序分析，分词字段不能使用                                                                    |
| fielddata      | fielddata": {"format": "disabled"}                                                 | 是否为text类型启动fielddata，实现排序和聚合分析。针对分词字段，参与排序或聚合时能提高性能，不分词字段统一建议使用doc_value |
| store          | true \| false(默认)                                                                | 是否单独设置此字段的是否存储而从_source字段中分离，只能搜索，不能获取值                                                    |
| coerce         | true(默认) \| false                                                                | 是否开启自动数据类型转换功能，比如：字符串转数字，浮点转整型                                                               |
| analyzer       | "analyzer": "ik"                                                                   | 指定分词器，默认分词器为standard analyzer                                                                                  |
| boost          | "boost": 1.23                                                                      | 字段级别的分数加权，默认值是1.0                                                                                            |
| fields         | "fields": {　　"name": {　　　　"type": "text",　　　　"index": true　　},　　...} | 对一个字段提供多种索引模式，同一个字段的值，一个分词，一个不分词                                                           |
| data_detection | true(默认) \| false                                                                | 是否自动识别日期类型                                                                                                       |

#### 文档 （document）

一个文档是一个可被索引的基础信息单元。**可以直接类比为MySQL中的行**

---

假设 ES 中有一个名为 `students` 的 Index（索引），存储三个学生的信息。

| Document（文档） | Field：name | Field：age | Field：class |
| ---------------- | ----------- | ---------- | ------------ |
| ID：1            | 张三        | 18         | 一班         |
| ID：2            | 李四        | 19         | 二班         |
| ID：3            | 王五        | 20         | 一班         |

对应 ES 中的实际 JSON 文档：

```json
{
  "_index": "students",
  "_id": "1",
  "_source": {
    "name": "张三",
    "age": 18,
    "class": "一班"
  }
}
```

| ES 概念  | 此例中的具体内容                                  | MySQL 类比       |
| -------- | ------------------------------------------------- | ---------------- |
| Index    | `students`                                      | 学生表           |
| Type     | 现代 ES 不再使用                                  | 无               |
| Document | ID 为 1 的学生 JSON 文档                          | 一行记录         |
| Field    | `name`、`age`、`class`                      | 表的列           |
| Mapping  | `name:text`、`age:integer`、`class:keyword` | 表结构及字段规则 |

**注意： ES 实际存储的是 JSON 文档，并根据字段的 Mapping 建立相应的索引结构，而不是像 MySQL 那样直接以关系表形式组织数据。**

## 典型操作：

### API路径：

以下以 Elasticsearch 7.17 为例，`user` 是索引名称（相当于 MySQL 的数据表），`_doc`、`_search`、`_update` 等都是 ES 提供的固定 API 路径。

#### ① 索引（Index）操作

| 操作             | HTTP 请求                    | 说明                               |
| ---------------- | ---------------------------- | ---------------------------------- |
| 创建索引         | `PUT /user`                | 创建 user 索引，可同时设置 Mapping |
| 查看索引         | `GET /user`                | 查看索引信息                       |
| 判断索引是否存在 | `HEAD /user`               | 存在返回 200，不存在返回 404       |
| 删除索引         | `DELETE /user`             | 删除索引及其中的所有文档           |
| 查看所有索引     | `GET /_cat/indices?v`      | 列出索引及其状态                   |
| 查看索引状态     | `GET /_cat/indices/user?v` | 查看 user 索引的状态和文档数量     |
| 查看 Mapping     | `GET /user/_mapping`       | 查看字段类型与映射规则             |
| 更新 Mapping     | `PUT /user/_mapping`       | 添加字段或修改允许更新的映射参数   |
| 查看 Settings    | `GET /user/_settings`      | 查看分片、副本等索引配置           |
| 更新 Settings    | `PUT /user/_settings`      | 修改允许动态更新的索引配置         |

#### ② 文档（Document）增删改查

| 操作                 | HTTP 请求                       | 说明                        |
| -------------------- | ------------------------------- | --------------------------- |
| 新增文档（自动 ID）  | `POST /user/_doc`             | 自动生成文档 ID             |
| 新增或覆盖指定 ID    | `PUT /user/_doc/1`            | 不存在则创建，存在则覆盖    |
| 仅创建指定 ID        | `PUT /user/_create/1`         | 已存在则返回冲突            |
| 根据 ID 查询         | `GET /user/_doc/1`            | 获取完整文档及元数据        |
| 获取原始 JSON        | `GET /user/_source/1`         | 只返回原始文档内容          |
| 判断文档是否存在     | `HEAD /user/_doc/1`           | 判断 ID 为 1 的文档是否存在 |
| 局部更新文档         | `POST /user/_update/1`        | 只更新指定字段              |
| 删除指定文档         | `DELETE /user/_doc/1`         | 删除 ID 为 1 的文档         |
| 批量新增、更新或删除 | `POST /user/_bulk`            | 在一个请求中执行多个操作    |
| 批量查询指定 ID      | `GET /user/_mget`             | 一次获取多个文档            |
| 根据条件更新         | `POST /user/_update_by_query` | 更新满足条件的所有文档      |
| 根据条件删除         | `POST /user/_delete_by_query` | 删除满足条件的所有文档      |

#### ③ 文档查询与搜索

| 操作         | HTTP 请求                     | 说明                     |
| ------------ | ----------------------------- | ------------------------ |
| 搜索文档     | `GET /user/_search`         | 使用 Query DSL 检索      |
| 搜索文档     | `POST /user/_search`        | 也支持 POST 携带查询体   |
| 查询全部文档 | `GET /user/_search`         | 请求体使用 `match_all` |
| 精确查询     | `POST /user/_search`        | 使用 `term`、`terms` |
| 全文检索     | `POST /user/_search`        | 使用 `match`           |
| 多字段检索   | `POST /user/_search`        | 使用 `multi_match`     |
| 范围查询     | `POST /user/_search`        | 使用 `range`           |
| 多条件查询   | `POST /user/_search`        | 使用 `bool`            |
| 排序查询     | `POST /user/_search`        | 使用 `sort`            |
| 分页查询     | `POST /user/_search`        | 使用 `from`、`size`  |
| 聚合分析     | `POST /user/_search`        | 使用 `aggs`            |
| 统计文档数量 | `GET /user/_count`          | 返回符合条件的文档数量   |
| 查看分词结果 | `POST /user/_analyze`       | 测试文本分词效果         |
| 查看评分解释 | `GET /user/_explain/1`      | 分析文档匹配与评分原因   |
| 验证查询语法 | `GET /user/_validate/query` | 检查 Query DSL 是否有效  |

注意： 其中精确查询、全文检索、分页、排序、聚合等，并不是不同的 URL，而是在相同的 `_search` 接口中使用不同的 JSON 请求体。

#### ④ 索引维护与高级操作

| 操作             | HTTP 请求                          | 说明                                       |
| ---------------- | ---------------------------------- | ------------------------------------------ |
| 刷新索引         | `POST /user/_refresh`            | 让最近写入的数据可以被搜索                 |
| 刷盘操作         | `POST /user/_flush`              | 将事务日志中的操作持久化到 Lucene 索引     |
| 关闭索引         | `POST /user/_close`              | 暂时关闭索引                               |
| 打开索引         | `POST /user/_open`               | 重新打开索引                               |
| 查看索引统计     | `GET /user/_stats`               | 查看搜索、写入、存储等统计数据             |
| 清除缓存         | `POST /user/_cache/clear`        | 清理索引相关缓存                           |
| 合并索引段       | `POST /user/_forcemerge`         | 合并 Lucene 索引段，通常用于不再写入的索引 |
| 创建索引别名     | `PUT /user/_alias/user_alias`    | 为 user 创建别名                           |
| 查看索引别名     | `GET /user/_alias`               | 查看已有别名                               |
| 删除索引别名     | `DELETE /user/_alias/user_alias` | 删除指定别名                               |
| 克隆索引         | `POST /user/_clone/user_copy`    | 克隆索引，需要满足只读等条件               |
| 重建索引         | `POST /_reindex`                 | 根据请求体将 user 数据复制到新索引         |
| 查看集群健康状态 | `GET /_cluster/health`           | 查看集群整体运行状态                       |
| 查看 ES 节点     | `GET /_cat/nodes?v`              | 查看节点信息                               |
| 查看分片状态     | `GET /_cat/shards/user?v`        | 查看 user 索引的分片分布                   |

### 索引

#### 创建索引

```bash
POST /student/_doc
{
    "settings": {
        "analysis": {
            "analyzer": {
                "ikmax" : {
                    "type": "custom",
                    "tokenizer" : "ik_max_word"
                }
            }
        }
    },
    "mapping": {
        "dynamic": false,
        "properties": {
            "name": {
                "type": "text",
                "boost": 3.0,
                "analyzer": "ikmax"
            },
            "age": {
                "type": "integer"
            },
            "phone": {
                "type": "keyword",
                "boost": 1.0
            },
            "skills": {
                "type": "text"
            },
            "birth": {
                "type": "date",
                "index": false
            }
        }
    }
}
```

#### 删除索引

```
DELETE /student
```

### 新增

#### 单数据新增

```
URI组成：/索引名称/文档类型名称/[当前文档ID]
```

```bash
POST /student/_doc/2
{
    "name":"lisi",
    "age": 22,
    "phone":"15522222222",
    "skills":[
        "C++",
        "Python"
    ],
    "birth":"2018-08-19 22:22:33"
}
```

#### 批量新增

正文格式要求： 每个文档包含两行内容：1. 索引信息； 2. 文档信息

```bash
POST /student/_doc/_bulk
{"index":{"_id":"1"}}
{"name":"zhangsan","age":18,"phone":"15511111111","skills":["C++", "Java"],"birth":"2017-08-19 22:22:22"}
{"index":{"_id":"6"}}
{"name":"lisi","age":19,"phone":"15522222222","skills":["C++", "Python"],"birth":"2018-08-19 22:22:33"}
{"index":{"_id":"3"}}
{"name":"wangwu","age":30,"phone":"15533333333","skills":["Php", "Python"],"birth":"2028-08-19 22:22:44"}
{"index":{"_id":"4"}}
{"name":"lisi w","age":20,"phone":"15544444444","skills":["Php", "Java"],"birth":"2038-08-19 22:22:55"}
{"index":{"_id":"5"}}
{"name":"lisi z","age":10,"phone":"15555555555","skills":["YYY", "BBBB"],"birth":"2048-08-19 22:22:11"}
```

#### 查询所有

```bash
POST /student/_doc/_search # 这是旧写法，不推荐，新版推荐去掉_doc
{
  "query": {
    "match_all": {}
  }
}
```

### 删除

#### 删除指定ID的数据

```bash
DELETE /student/_doc/1
```

#### 批量删除指定ID的数据

```bash
POST /student/_doc/_bulk
{ "delete": { "_id": "2" } }
{ "delete": { "_id": "3" } }
```

#### 根据条件删除数据

```bash
POST /student/_doc/_delete_by_query
{
  "query": {
    "match": {
      "name": "lisi" # 使用match全文检索，对name进行分词后包含lisi的都会删除
    }
  }
}
```

### 更新

#### 单数据更新

```bash
POST /student/_update/1
{
  "doc": {
    "phone": "18888888888",
    "age": 19
  }
}
```

#### 批量更新

```bash
POST /student/_doc/_bulk
{"update":{"_id":"1"}}
{ "doc": { "phone": "11111111111" } }
{"update":{"_id":"2"}}
{ "doc": { "phone": "22222222222" } }
{"update":{"_id":"3"}}
{ "doc": { "phone": "33333333333" } }
```

除了以上两种方式进行更新，也可以以新增单个数据的方式进行全字段更新。

### 查询

#### 单条件检索

##### term **单字段精确匹配**

```bash
POST /student/_search
{
    "query" :  {
        "term" :  {
            "name" : "lisi"
        }
    }
}
```

##### terms 单字段多值精确匹配

```bash
{
    "query" :  {
        "terms" :  {
            "phone" :  [
                "15511111111",
                "15522222222"
            ]
        }
    }
}
```

##### match 单字段分词匹配（全文检索）

```bash
{
    "query" :  {
        "match" :  {
            "name" : "z"
        }
    }
}
```

##### range 数组字段区间匹配

```bash
{
    "query" :  {
        "range" :  {
            "age" :  {
                "gte" : 10, # 大于10，小于20
                "lte" : 20
            }
        }
    }
}
```

##### multi_match 多字段内容模糊匹配

```bash
{
    "query" :  {
        "multi_match" :   {
            "fields" :  [ // 从多个字段进行匹配，只要有一个匹配到就进行输出
                "name",
                "phone"
            ],
            "query" : "zhangsan"
        }
    }
}
```

#### 多条件检索

##### bool检索

```bash
{
    "query": {
        "bool": { // 布尔查询：组合多个查询条件
            "must": [ // 必须满足的条件，相当于 AND
                {
                    "match": {
                        "skills": "C++" // skills 字段必须匹配 C++（分词匹配）
                    }
                }
            ],
            "must_not": [ // 必须排除的条件，相当于 NOT
                {
                    "terms": {
                        "phone": [ // phone 字段不能等于以下任意一个值
                            "15522222222",
                            "15555555555"
                        ]
                    }
                }
            ],
            "should": [ // 可选匹配条件，满足后可提高相关性评分
                {
                    "match": {
                        "name": "lisi" // name 字段匹配 lisi（分词匹配）
                    }
                },
                {
                    "terms": {
                        "phone": [ // phone 字段精确匹配以下任意一个值
                            "15511111111",
                            "15555555555"
                        ]
                    }
                }
            ],
            "minimum_should_match": 1 // should 中至少满足 1 个条件，因此本例中 should 是必选的
        }
    }
}
```

##### sort检索

```bash
{
    "query": {
        "bool": { // 布尔查询：组合多个查询条件
            "minimum_should_match": 1, // should 中至少满足 1 个条件
            "should": [ // 多个可选条件，满足任意一个即可
                {
                    "match": {
                        "name": "lisi" // name 字段分词匹配 lisi
                    }
                },
                {
                    "terms": {
                        "phone": [ // phone 字段精确匹配以下任意一个手机号
                            "15511111111",
                            "15533333333"
                        ]
                    }
                }
            ]
        }
    },
    "sort": [ // 对查询结果进行多字段排序，按数组顺序依次排序
        {
            "name.keyword": "desc" // 首先按姓名完整值降序排列
        },
        {
            "age": "asc" // 姓名相同时，再按年龄升序排列
        }
    ]
}
```

##### 分页查找

对检索按指定方式进行排序，并限制获取结果数量，以及偏移量

```bash
{
    "query": {
        "bool": { // 布尔查询：组合多个查询条件
            "minimum_should_match": 1, // should 中至少满足 1 个条件
            "should": [ // 满足以下任意一个条件即可
                {
                    "match": {
                        "name": "lisi" // name 字段分词匹配 lisi
                    }
                },
                {
                    "terms": {
                        "phone": [ // phone 字段精确匹配以下任意一个手机号
                            "15511111111",
                            "15533333333"
                        ]
                    }
                }
            ]
        }
    },
    "size": 10, // 每次最多返回 10 条文档
    "from": 2, // 跳过前 2 条文档，从第 3 条开始返回（下标从 0 开始）
    "sort": [ // 按优先级依次进行多字段排序
        {
            "name.keyword": "desc" // 优先按姓名完整值降序排序
        },
        {
            "age": "asc" // 姓名相同时，再按照年龄升序排序
        }
    ]
}
```

## ES客户端SDK

代码：https://github.com/seznam/elasticlient

官网：https://seznam.github.io/elasticlient/index.html

### 接口介绍

#### 响应结构

**cpr/response.h**

```cpp
namespace cpr {
class Response {
    public:
        long status_code{};
        std::string text{};
        Header header{};
        Url url{};
        double elapsed{};
        Cookies cookies{};
        Error error{};
        std::string raw_header{};
        std::string status_line{};
        std::string reason{};
};
```

#### 典型接口

**elasticlient/client.h**

```cpp
namespace elasticlient {
class Client {
// http://user:password@localhost:9200/
Client(const std::vector<std::string> &hostUrlList,
                    std::int32_t timeout = 6000);
/**
 * Perform search on nodes until it is successful. Throws exception if all nodes
 * has failed to respond.
 * \param indexName specification of an Elasticsearch index.
 * \param docType specification of an Elasticsearch document type.
 * \param body Elasticsearch request body.
 * \param routing Elasticsearch routing. If empty, no routing has been used.
 *
 * \return cpr::Response if any of node responds to request.
 * \throws ConnectionException if all hosts in cluster failed to respond.
 */
cpr::Response search(const std::string &indexName,
                     const std::string &docType,
                     const std::string &body,
                     const std::string &routing = std::string());

/**
 * Get document with specified id from cluster. Throws exception if all nodes
 * has failed to respond.
 * \param indexName specification of an Elasticsearch index.
 * \param docType specification of an Elasticsearch document type.
 * \param id Id of document which should be retrieved.
 * \param routing Elasticsearch routing. If empty, no routing has been used.
 *
 * \return cpr::Response if any of node responds to request.
 * \throws ConnectionException if all hosts in cluster failed to respond.
 */
cpr::Response get(const std::string &indexName,
                  const std::string &docType,
                  const std::string &id = std::string(),
                  const std::string &routing = std::string());

/**
 * Index new document to cluster. Throws exception if all nodes has failed to respond.
 * \param indexName specification of an Elasticsearch index.
 * \param docType specification of an Elasticsearch document type.
 * \param body Elasticsearch request body.
 * \param id Id of document which should be indexed. If empty, id will be generated
 *           automatically by Elasticsearch cluster.
 * \param routing Elasticsearch routing. If empty, no routing has been used.
 *
 * \return cpr::Response if any of node responds to request.
 * \throws ConnectionException if all hosts in cluster failed to respond.
 */
cpr::Response index(const std::string &indexName,
                    const std::string &docType,
                    const std::string &id,
                    const std::string &body,
                    const std::string &routing = std::string());

/**
 * Delete document with specified id from cluster. Throws exception if all nodes
 * has failed to respond.
 * \param indexName specification of an Elasticsearch index.
 * \param docType specification of an Elasticsearch document type.
 * \param id Id of document which should be deleted.
 * \param routing Elasticsearch routing. If empty, no routing has been used.
 *
 * \return cpr::Response if any of node responds to request.
 * \throws ConnectionException if all hosts in cluster failed to respond.
 */
cpr::Response remove(const std::string &indexName,
                     const std::string &docType,
                     const std::string &id,
                     const std::string &routing = std::string());
}
}
```

# ElasticClient 封装设计

## 设计

对于 ES 的操作其实都是通过 RESTful 风格 API 实现的，需要考虑的问题其实是如何组织出合适的 JSON 格式的正文。如果使用 JSON 来进行组织，为了简化使用要求，咱们的目标就是封装出一套类，能够直接序列化出指定功能的 JSON 正文。

- `settings`（object）
- `analysis`（object）
- `analyzer`（object）
- `mapping`
- `dynamic`（bool）
- `properties`（object）

### Base 基类

**成员变量：**

- `std::string` 对象：保存当前自身 key
- `Json::Value` 对象：保存最基础的字段数据

**成员方法：**

1. 设置 & 获取 key 值
2. 添加基础类型字段
3. 添加数组类型字段
4. 虚函数：获取 `Json::Value` 对象
5. 虚函数：获取序列化后的字符串

### 派生对象 Object 类

**成员变量：**

- `<string, Base::ptr>` 映射 map：存储自身的子对象

**成员方法：**

1. 虚函数：重写获取 `Json::Value` 对象
2. 清理数据
3. 添加传入子对象
4. 获取指定子对象

### 派生数组 Array 类

**成员变量：**

- `Base::ptr` 数组：存储自身的子对象

**成员方法：**

1. 虚函数：重写获取 `Json::Value` 对象
2. 清理数据
3. 添加传入子对象

### 根据需要从对象/数组类派生匹配类

**1. 索引配置与映射相关**

- `Filed` 类：mapping 中的字段类
- `Properties` 类：mapping 中的文档属性类
- `Mapping` 类：索引结构映射类
- `Tokenizer` 类：自定义分词器类
- `Analyzer` 类：分词器父类
- `Analysis` 类：分词器相关父类
- `Setting` 类：索引配置类

**2. 查询条件相关**

- `QObject` 类：查询操作对象基类，用于派生 query、bool...
  - 包含创建一系列查询操作子对象：term、terms、match、multi_match...
- `QArray` 类：查询操作数组基类，用于派生 must、should...
  - 包含创建一系列查询操作子对象：term、terms、match、multi_match...
- `Bool` 类：多条件查询 bool 类
- `Query` 类：查询父类

**3. ES 增删改查相关**

- `Index` 类：用于组织创建索引结构映射的类
- `Insert` 类：用于进行数据新增的类
- `Search` 类：用于进行数据查询的类
- `Update` 类：用于进行数据更新的类
- `Remove` 类：用于进行数据删除的类

```mermaid
flowchart LR
    A["① 基础层<br/>Base"]
    B["② JSON 结构层<br/>Object / Array"]
    C["③ ES 业务层<br/>Mapping / Properties / Setting<br/>Query / Bool / QObject / QArray 等"]
    D["④ 操作层<br/>Index / Insert / Search<br/>Update / Remove"]
    E["JSON 请求正文"]

    A -->|提供基础能力| B
    B -->|支持结构组合| C
    C -->|组织业务参数| D
    D -->|序列化输出| E

    style A fill:#e8f1ff,stroke:#5285c5,color:#17365d
    style B fill:#e8f1ff,stroke:#5285c5,color:#17365d
    style C fill:#fff2d9,stroke:#c79a42,color:#61450a
    style D fill:#e4f5e8,stroke:#4b9b65,color:#205735
    style E fill:#e4f5e8,stroke:#4b9b65,color:#205735
```
