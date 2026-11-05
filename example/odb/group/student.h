#pragma once
#include <string>
#include <cstddef> // std::size_t
#include <boost/date_time/posix_time/posix_time.hpp>
#include <odb/nullable.hxx>
#include <odb/core.hxx>

#pragma db object table("tbl_student")
class Student
{
public:
    Student() {}
    Student(const std::string &name) : _name(name) {}
    size_t sn() const { return _sn; }
    void set_sn(size_t sn) { _sn = sn; }
    size_t classes_id() const { return _classes_id; }
    void set_classes_id(size_t classes_id) { _classes_id = classes_id; }
    const std::string &name() const { return _name; }
    void set_name(const std::string &name) { _name = name; }
    odb::nullable<int> age() const { return _age; }
    void set_age(odb::nullable<int> age) { _age = age; }
    odb::nullable<double> score() const { return _score; }
    void set_score(odb::nullable<double> score) { _score = score; }
    odb::nullable<boost::posix_time::ptime> birthday() const { return _birthday; }
    void set_birthday(const boost::posix_time::ptime &birthday) { _birthday = birthday; }

private:
    friend class odb::access;
#pragma db id auto
    size_t _sn;
#pragma db index // 指定索引，便于按班级ID查询
    size_t _classes_id;
    std::string _name;
    odb::nullable<int> _age;
    odb::nullable<double> _score;
    odb::nullable<boost::posix_time::ptime> _birthday;
};

#pragma db object table("tbl_classes")
class Classes
{
public:
    Classes() {}
    Classes(const std::string &name) : _name(name) {}
    size_t id() const { return _id; }
    void set_id(size_t id) { _id = id; }
    const std::string &name() const { return _name; }
    void set_name(const std::string &name) { _name = name; }

private:
    friend class odb::access;
#pragma db id auto
    size_t _id;                       // 班级ID
#pragma db unique type("VARCHAR(32)") // 指定唯一索引，类型为 VARCHAR(32)
    std::string _name;                // 班级名称
};

// 以班级ID进行分组，统计每个班级的学生总数，以及学生平均分
#pragma db view object(Student) \
    query("group by " + Student::_classes_id + " having " + (?)) // ？代表占位符，表示可以在查询时指定条件
struct GroupCount
{
#pragma db column(Student::_classes_id) // 等价于 #pragma db column("Student.classes_id")
    size_t classes_id;
#pragma db column("count(" + Student::_sn + ")")
    size_t total_stu;
#pragma db column("avg(" + Student::_score + ") as avg_score")
    double avg_score;
};
