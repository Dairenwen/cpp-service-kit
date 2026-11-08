#include "BreezeElastic.hpp"

#include <gtest/gtest.h>
#include <chrono>
#include <cstdlib>
#include <memory>
#include <string>
#include <unistd.h>

namespace
{
  // 先在本地检查索引配置的 JSON
  TEST(BreezeElasticRequestTest, BuildIndexSettingsAndMappings)
  {
    BreezeElastic::Indexer index("students"); // 配置的目标索引名为 students。
    // 使用标准分词器
    auto analyzer = index.tokenizer("student_analyzer"); // 创建名为 student_analyzer 的分析器配置。
    analyzer->type("custom");                            // type=custom：由用户指定该分析器的组成。
    analyzer->tokenizer("standard");                     // tokenizer=standard：使用 ES 内置的标准分词器。
    index.field("name")->type("text");                   // name 为 text 类型，适合全文检索。
    index.field("age")->type("integer");                 // age 为 integer 类型，适合整数及范围查询。
    // Mappings 的构造函数默认设置 dynamic=false，这里不再重复设置

    const auto body = index.val();
    EXPECT_EQ(body["settings"]["analysis"]["analyzer"]["student_analyzer"]["tokenizer"].asString(), "standard");
    EXPECT_EQ(body["mappings"]["properties"]["name"]["type"].asString(), "text");
    EXPECT_EQ(body["mappings"]["properties"]["age"]["type"].asString(), "integer");
    EXPECT_FALSE(body["mappings"]["dynamic"].asBool());
  }
  /*
  构造的索引配置 JSON（字段顺序可能不同）：
  {
    "settings": {
      "analysis": {
        "analyzer": {
          "student_analyzer": { "type": "custom", "tokenizer": "standard" }
        }
      }
    },
    "mappings": {
      "dynamic": false,
      "properties": { "name": { "type": "text" }, "age": { "type": "integer" } }
    }
  }
  */

  // 演示组合精确查询、全文查询和范围查询，并验证生成的请求正文。
  TEST(BreezeElasticRequestTest, BuildBoolSearchAndPagination)
  {
    BreezeElastic::Searcher search("students"); // 查询目标是 students 索引。
    search.size(10);                            // 每页最多 10 条。
    search.from(20);                            // 从第 21 条开始。
    auto must = search.query()->must();         // bool.must 中的条件必须同时满足。
    must->term("status")->setValue("active");   // status 必须精确等于 active。
    must->match("name")->setValue("alice");     // name 使用 match 全文查询 alice。
    must->range("age")->setRange(18, 30);       // gt 18 且 lt 30，不包含边界。

    const auto body = search.val();
    EXPECT_EQ(body["size"].asInt(), 10);
    EXPECT_EQ(body["from"].asInt(), 20);
    const auto &conditions = body["query"]["bool"]["must"];
    ASSERT_EQ(conditions.size(), 3U);
    EXPECT_EQ(conditions[0]["term"]["status"].asString(), "active");
    EXPECT_EQ(conditions[1]["match"]["name"].asString(), "alice");
    EXPECT_EQ(conditions[2]["range"]["age"]["gt"].asInt(), 18);
    EXPECT_EQ(conditions[2]["range"]["age"]["lt"].asInt(), 30);
  }
  /*
  构造的搜索请求 JSON：
  {
    "size": 10,
    "from": 20,
    "query": {
      "bool": {
        "must": [
          { "term": { "status": "active" } },
          { "match": { "name": "alice" } },
          { "range": { "age": { "gt": 18, "lt": 30 } } }
        ]
      }
    }
  }
  */

  class BreezeElasticTest : public ::testing::Test
  {
  protected:
    void SetUp() override
    {
      const std::string url = "http://elastic:123456@dev-elasticsearch:9200/"; // 容器中的 ES 连接地址和认证信息。
      // client_ = BreezeElastic::ESClientFactory::create({url});
      raw_client_ = std::make_unique<elasticlient::Client>(std::vector<std::string>{url});

      // 每次运行使用独立索引，只清理本次成功创建的索引
      index_ = "breeze_elastic_test_" + std::to_string(getpid()) + "_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
      BreezeElastic::Indexer index(index_);   // 对本次生成的临时索引配置字段映射。
      index.field("name")->type("text");      // name 为可进行全文检索的文本。
      index.field("status")->type("keyword"); // status 为不分词的精确值，适合 term 查询。
      index.field("age")->type("integer");    // age 为整数，适合进行数值范围查询。
      // 映射默认 dynamic=false，本测试需要的字段都在上面显式声明。

      // 这里临时调用底层 PUT /索引名，后续文档操作均使用封装接口。
      const auto response = raw_client_->performRequest(elasticlient::Client::HTTPMethod::PUT, index_, index.to_string());
      ASSERT_GE(response.status_code, 200) << response.text;
      ASSERT_LT(response.status_code, 300) << response.text;
      index_created_ = true;
    }
    /*
    创建临时索引时使用的 JSON：
    {
      "mappings": {
        "dynamic": false,
        "properties": {
          "name": { "type": "text" },
          "status": { "type": "keyword" },
          "age": { "type": "integer" }
        }
      }
    }
    */

    void TearDown() override
    {
      // 即使测试中途断言失败，也删除本次成功创建的索引及其中的文档。
      if (index_created_)
        EXPECT_TRUE(client_->remove(index_)) << "清理测试索引失败: " << index_;
    }

    bool refresh()
    {
      // ES 搜索不是实时可见的：手动刷新后再查询，避免依赖固定等待时间。
      const auto response = raw_client_->performRequest(elasticlient::Client::HTTPMethod::POST, index_ + "/_refresh", "");
      EXPECT_GE(response.status_code, 200) << response.text;
      EXPECT_LT(response.status_code, 300) << response.text;
      return response.status_code >= 200 && response.status_code < 300;
    }

    BreezeElastic::BaseClient::ptr client_;
    std::unique_ptr<elasticlient::Client> raw_client_;
    std::string index_;
    bool index_created_ = false;
  };

  // 模拟日常流程：写入学生文档、搜索、修改、再次搜索并删除。
  TEST_F(BreezeElasticTest, InsertSearchUpdateAndRemoveDocument)
  {
    const std::string id = "student_1";          // 要写入的文档 ID。
    BreezeElastic::Inserter student(index_, id); // 指定文档所在索引和 ID。
    student.add("name", "Alice");                // name 字段：学生姓名。
    student.add("status", "active");             // status 字段：学生状态。
    student.add("age", 20);                      // age 字段：学生年龄，JSON 中为数字。
    ASSERT_TRUE(client_->insert(student));
    ASSERT_TRUE(refresh());

    // bool.must 中多个条件同时成立，search 返回命中的 _source 数组。
    BreezeElastic::Searcher search(index_);                     // 在本次创建的临时索引中搜索。
    search.query()->must()->term("status")->setValue("active"); // 精确匹配状态 active。
    search.query()->must()->range("age")->setRange(18, 30);     // 年龄大于 18 且小于 30。
    search.size(10);                                            // 最多返回 10 条命中。
    auto results = client_->search(search);
    ASSERT_TRUE(results.has_value());
    ASSERT_TRUE(results->isArray());
    ASSERT_EQ(results->size(), 1U);
    EXPECT_EQ((*results)[0]["name"].asString(), "Alice");
    EXPECT_EQ((*results)[0]["age"].asInt(), 20);

    // Updater 将字段放在 doc 中，只修改给定字段。
    BreezeElastic::Updater update(index_, id); // 指定要更新的索引和文档 ID。
    update.add("age", 21);                     // 只将 age 改为 21，其余字段保持原值。
    ASSERT_TRUE(client_->update(update));
    ASSERT_TRUE(refresh());
    results = client_->search(search);
    ASSERT_TRUE(results.has_value());
    ASSERT_EQ(results->size(), 1U);
    EXPECT_EQ((*results)[0]["age"].asInt(), 21);
    EXPECT_EQ((*results)[0]["name"].asString(), "Alice");

    // 按文档 ID 删除，再确认搜索结果为空数组。
    BreezeElastic::Deleter remove(index_, id);
    ASSERT_TRUE(client_->remove(remove));
    ASSERT_TRUE(refresh());
    results = client_->search(search);
    ASSERT_TRUE(results.has_value());
    ASSERT_TRUE(results->isArray());
    EXPECT_TRUE(results->empty());
  }
  /*
  插入文档的 JSON：
  { "name": "Alice", "status": "active", "age": 20 }

  搜索文档的 JSON（插入、更新、删除后均使用同一查询）：
  {
    "size": 10,
    "query": {
      "bool": {
        "must": [
          { "term": { "status": "active" } },
          { "range": { "age": { "gt": 18, "lt": 30 } } }
        ]
      }
    }
  }

  更新文档的 JSON：
  { "doc": { "age": 21 } }
  */
} // namespace
