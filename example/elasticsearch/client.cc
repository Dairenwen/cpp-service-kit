#include <cpr/response.h>
#include <elasticlient/client.h>
#include <iostream>
#include <thread>
#include <chrono>

// 使用 student 索引。
const std::string index_name = "student";
const std::string type_name = "_doc";

// 使用 PUT /索引名 创建索引；返回 false 表示本程序没有成功创建索引。
bool create_index(elasticlient::Client &client)
{
    // 1. 构造请求正文
    const std::string body = R"({
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
            "mappings": {
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
                        "format": "yyyy-MM-dd HH:mm:ss",
                        "index": false
                    }
                }
            }
        })";
    // PUT 索引名提交 settings 和 mappings；client.index 用于写入文档。
    auto resp = client.performRequest(elasticlient::Client::HTTPMethod::PUT, index_name, body);
    // 获取响应并检查状态码：2xx 表示请求成功。
    if (resp.status_code < 200 || resp.status_code >= 300)
    {
        std::cout << "create index failed: " << resp.status_code << " " << resp.text << std::endl;
        return false;
    }
    return true;
}

// 向示例索引写入 ID 为 2 的文档。
void add_data(elasticlient::Client &client)
{
    const std::string body = R"({
            "name": "张三",
            "age": 19,
            "phone": "13888888888",
            "skills": ["java","php","go"],
            "birth": "2007-05-21 12:35:32"
        })";
    auto resp = client.index(index_name, type_name, "2", body); // 2是文档 ID，必须是字符串类型
    if (resp.status_code < 200 || resp.status_code >= 300)
    {
        std::cout << "insert data failed: " << resp.status_code << " " << resp.text << std::endl;
        return;
    }
}

// 使用相同 ID 重新写入文档，替换原有文档。
void mod_data(elasticlient::Client &client)
{
    const std::string body = R"({
        "name": "李四",
        "age": 19,
        "phone": "13333333333",
        "skills": ["java","php","go"],
        "birth": "2007-05-21 12:35:32"
    })";
    // 使用index方式进行更新，必须是全量更新，它等价于，先删除，再新增，如果不是全量更新，则最终就只有更新字段
    // const std::string body = R"({
    //     "name": "李四"
    // })";
    auto resp = client.index(index_name, type_name, "2", body); // 如果调用update方法，则只会更新指定字段，其他字段保持不变
    if (resp.status_code < 200 || resp.status_code >= 300)
    {
        std::cout << "insert data failed: " << resp.status_code << " " << resp.text << std::endl;
        return;
    }
}

// 按文档 ID 删除数据。
void remove_data(elasticlient::Client &client)
{
    auto resp = client.remove(index_name, type_name, "2"); // 删除 ID 为 2 的文档
    if (resp.status_code < 200 || resp.status_code >= 300)
    {
        std::cout << "remove data failed: " << resp.status_code << " " << resp.text << std::endl;
        return;
    }
}
// 删除索引；已有索引及其全部文档也会被删除。
bool remove_index(elasticlient::Client &client)
{
    auto resp = client.performRequest(elasticlient::Client::HTTPMethod::DELETE, index_name, "");
    if (resp.status_code < 200 || resp.status_code >= 300)
    {
        std::cout << "remove index failed: " << resp.status_code << " " << resp.text << std::endl;
        return false;
    }
    return true;
}

// 使用 match_all 查询索引中的文档。
void get_data(elasticlient::Client &client)
{
    const std::string body = R"(
        {
            "query" :  {
                "match_all" :  {}
            }
        }
    )";
    auto resp = client.search(index_name, type_name, body); // 查询索引中的所有文档
    if (resp.status_code < 200 || resp.status_code >= 300)
    {
        std::cout << "search data failed: " << resp.status_code << " " << resp.text << std::endl;
        return;
    }
    std::cout << "resp:" << resp.text << std::endl;
}

int main()
{
    // 1. 定义es服务器url，dev-elasticsearch 是开发容器网络中的服务名。
    const std::string url = "http://elastic:123456@dev-elasticsearch:9200/";
    // 2. 实例化client对象
    elasticlient::Client client({url});
    // 3. 数据操作（创建索引，新增数据，全量查询，删除数据，删除索引）
    auto exists = client.performRequest(elasticlient::Client::HTTPMethod::HEAD, index_name, ""); // 检查索引是否存在
    if (exists.status_code == 200)
    {
        if (!remove_index(client)) // 删除索引
        {
            return 1;
        }
    }
    else if (exists.status_code != 404)
    {
        std::cout << "check index failed: " << exists.status_code << " " << exists.text << std::endl;
        return 1;
    }
    // 如果删除或创建失败，停止后续写入和删除操作。
    if (!create_index(client))
    {
        return 1;
    }
    std::this_thread::sleep_for(std::chrono::seconds(1));
    add_data(client);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    mod_data(client);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    get_data(client);
    remove_data(client);
    remove_index(client);
    return 0;
}
