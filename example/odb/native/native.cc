// 使用原生 SQL 视图读取学生信息。
#include <odb/database.hxx>
#include <odb/mysql/transaction.hxx>
#include <odb/mysql/database.hxx>

#include "student.h"
#include "student-odb.hxx"

const std::string HOST = "mysql-service";
const std::string USER = "root";
const std::string PASS = "123456";
const std::string DBNAME = "mytest";
const unsigned int PORT = 3306;
const std::string CSET = "utf8";

void native_select(std::unique_ptr<odb::mysql::database> &handler)
{
    try
    {
        odb::transaction tx(handler->begin());

        tx.tracer(odb::stderr_full_tracer);

        auto &db = tx.database();
        typedef odb::query<NativeStudent> Query;
        typedef odb::result<NativeStudent> Result;
        Result res(db.query<NativeStudent>());
        for (auto it = res.begin(); it != res.end(); ++it)
        {
            std::cout << "班级:" << it->classes_id << "\t";
            std::cout << "姓名" << it->name << "\t";
            if (it->score)
                std::cout << "分数:" << it->score.get() << std::endl;
        }
        tx.commit();
    }
    catch (odb::exception &e)
    {
        std::cout << "odb error: " << e.what() << std::endl;
    }
}

int main()
{
    // 1. 构造连接池
    std::unique_ptr<odb::mysql::connection_factory> pool(new odb::mysql::connection_pool_factory(5));
    // 2. 构造数据库操作句柄
    auto handler = std::make_unique<odb::mysql::database>(USER, PASS, DBNAME, HOST, PORT, nullptr, CSET, 0, std::move(pool));
    // 3. 数据操作
    native_select(handler);
    return 0;
}
