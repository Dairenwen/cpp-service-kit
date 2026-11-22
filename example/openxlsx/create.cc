#include <OpenXLSX.hpp>
#include <exception>
#include <iostream>

// 创建包含两个班级的学生信息表，并通过克隆生成第二张工作表。
int main()
{
    try
    {
        OpenXLSX::XLDocument doc;
        // 文件保存在当前工作目录；已有同名文件时不覆盖。
        doc.create("studentInfo.xlsx", false);

        auto workbook = doc.workbook();            // 获取工作簿对象
        auto sheet = workbook.worksheet("Sheet1"); // 获取默认工作表对象
        sheet.setName("1班");

        // 写入表头和 1 班的四名学生。
        sheet.cell("A1").value() = "姓名";
        sheet.cell("B1").value() = "学号";
        sheet.cell("C1").value() = "年龄";
        sheet.cell("D1").value() = "成绩";

        sheet.cell("A2").value() = "张三";
        sheet.cell("B2").value() = "20230001";
        sheet.cell("C2").value() = 20;
        sheet.cell("D2").value() = 90;

        sheet.cell("A3").value() = "李四";
        sheet.cell("B3").value() = "20230002";
        sheet.cell("C3").value() = 21;
        sheet.cell("D3").value() = 85;

        sheet.cell("A4").value() = "王五";
        sheet.cell("B4").value() = "20230003";
        sheet.cell("C4").value() = 22;
        sheet.cell("D4").value() = 95;

        sheet.cell("A5").value() = "赵六";
        sheet.cell("B5").value() = "20230004";
        sheet.cell("C5").value() = 23;
        sheet.cell("D5").value() = 88;

        // 克隆 1 班，修改 2 班最后一名学生的信息。
        workbook.cloneSheet("1班", "2班");
        auto sheet2 = workbook.worksheet("2班"); // 获取 2 班工作表对象
        auto lastRow = sheet2.rowCount();
        sheet2.cell(lastRow, 1).value() = "田七"; // 修改 2 班最后一名学生的姓名
        sheet2.cell(lastRow, 2).value() = "20230004";
        sheet2.cell(lastRow, 3).value() = 22;
        sheet2.cell(lastRow, 4).value() = 90; //

        doc.save();
        std::cout << "已创建 studentInfo.xlsx，工作表数量：" << workbook.worksheetCount() << std::endl;
        doc.close();
    }
    catch (const std::exception &e)
    {
        std::cerr << "创建 Excel 文件失败：" << e.what() << std::endl;
        return 1;
    }
    return 0;
}
