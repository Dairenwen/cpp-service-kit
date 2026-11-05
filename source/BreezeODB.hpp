#pragma once

#include <memory>
#include <string>

#include <odb/database.hxx>

namespace BreezeODB
{
    // MySQL 连接及连接池配置。
    struct mysql_settings
    {
        std::string host;
        std::string user = "root";
        std::string passwd;
        std::string db;
        std::string cset = "utf8";
        unsigned int port = 3306;
        unsigned int connection_pool_size = 3;
    };

    // 根据配置创建使用连接池的 ODB 数据库句柄。
    class DBFactory
    {
    public:
        static std::shared_ptr<odb::database> mysql(const mysql_settings &settings);
    };
}
