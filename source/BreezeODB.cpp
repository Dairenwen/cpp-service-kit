#include "BreezeODB.hpp"

#include <odb/mysql/database.hxx>
#include <utility>

namespace BreezeODB
{
    std::shared_ptr<odb::database> DBFactory::mysql(const mysql_settings &settings)
    {
        // 创建连接池并将所有权交给 MySQL 数据库句柄。
        std::unique_ptr<odb::mysql::connection_factory> pool(new odb::mysql::connection_pool_factory(settings.connection_pool_size));
        return std::make_shared<odb::mysql::database>(
            settings.user, settings.passwd, settings.db,
            settings.host, settings.port, nullptr,
            settings.cset, 0, std::move(pool));
    }
}
