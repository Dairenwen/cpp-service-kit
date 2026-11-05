#pragma once

#include <string>
#include <odb/core.hxx>

// 测试模型映射到连接专用的临时表。
#pragma db object table("breeze_odb_test_record")
class TestRecord
{
public:
    TestRecord() = default;
    TestRecord(unsigned long id, const std::string &name) : id_(id), name_(name) {}

    unsigned long id() const { return id_; }
    const std::string &name() const { return name_; }
    void name(const std::string &value) { name_ = value; }

private:
    friend class odb::access;
    #pragma db id
    unsigned long id_ = 0;
    #pragma db type("VARCHAR(64)")
    std::string name_;
};
