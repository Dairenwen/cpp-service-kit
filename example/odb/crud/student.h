#pragma once
#include <string>
#include <cstddef> // std::size_t
#include <boost/date_time/posix_time/posix_time.hpp>
#include <odb/nullable.hxx>
#include <odb/core.hxx>

#pragma db object table("tbl_student") // 指定表名
class Student
{
public:
    Student() {}
    Student(const std::string &name) : _name(name) {}
    size_t sn() const { return _sn; }
    void set_sn(size_t sn) { _sn = sn; }
    const std::string &name() const { return _name; }
    void set_name(const std::string &name) { _name = name; }
    odb::nullable<int> age() const { return _age; } // 允许 age 为 null
    void set_age(odb::nullable<int> age) { _age = age; }
    odb::nullable<double> score() const { return _score; } // 允许 score 为 null
    void set_score(odb::nullable<double> score) { _score = score; }
    odb::nullable<boost::posix_time::ptime> birthday() const { return _birthday; } // 允许 birthday 为 null
    void set_birthday(const boost::posix_time::ptime &birthday) { _birthday = birthday; }
    size_t classes_id() const { return _classes_id; }
    void set_classes_id(size_t classes_id) { _classes_id = classes_id; }

private:
    friend class odb::access; // 允许 ODB 访问私有成员
#pragma db id auto            // 指定主键为自增
    size_t _sn;
#pragma db index
    size_t _classes_id;
    std::string _name;
    odb::nullable<int> _age;
    odb::nullable<double> _score;
    odb::nullable<boost::posix_time::ptime> _birthday;
};
