#include "BreezeODB.hpp"
#include "test_record.hxx"
#include "test_record-odb.hxx"

#include <gtest/gtest.h>
#include <odb/connection.hxx>
#include <odb/transaction.hxx>
#include <cstdlib>
#include <memory>
#include <string>

namespace
{
    // 环境变量可覆盖开发容器中的默认连接配置。
    std::string env_or_default(const char *name, const char *fallback)
    {
        const char *value = std::getenv(name);
        return value ? value : fallback;
    }

    TEST(BreezeODBTest, SettingsDefaults)
    {
        BreezeODB::mysql_settings settings;
        EXPECT_EQ(settings.user, "root");
        EXPECT_EQ(settings.cset, "utf8");
        EXPECT_EQ(settings.port, 3306u);
        EXPECT_EQ(settings.connection_pool_size, 3u);
    }

    // 使用临时表验证工厂创建的数据库句柄和常见对象操作。
    TEST(BreezeODBTest, CreateAndCrud)
    {
        BreezeODB::mysql_settings settings;
        settings.host = env_or_default("BREEZE_ODB_HOST", "mysql-service");
        settings.user = env_or_default("BREEZE_ODB_USER", "root");
        settings.passwd = env_or_default("BREEZE_ODB_PASSWORD", "123456");
        settings.db = env_or_default("BREEZE_ODB_DATABASE", "mytest");
        auto database = BreezeODB::DBFactory::mysql(settings);
        ASSERT_NE(database, nullptr);

        odb::transaction transaction(database->begin());
        // 临时表只属于当前连接，连接关闭后由 MySQL 自动释放。
        transaction.connection().execute(
            "CREATE TEMPORARY TABLE breeze_odb_test_record "
            "(id BIGINT UNSIGNED NOT NULL PRIMARY KEY, name VARCHAR(64) NOT NULL)");

        TestRecord record(1, "Alice");
        database->persist(record);
        auto found = database->load<TestRecord>(1);
        ASSERT_NE(found, nullptr);
        EXPECT_EQ(found->name(), "Alice");

        found->name("Bob");
        database->update(*found);
        auto updated = database->load<TestRecord>(1);
        ASSERT_NE(updated, nullptr);
        EXPECT_EQ(updated->name(), "Bob");

        database->erase<TestRecord>(1);
        EXPECT_EQ(database->find<TestRecord>(1), nullptr);
        transaction.commit();
    }
}
