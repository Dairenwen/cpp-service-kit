#include <OpenXLSX.hpp>
#include <exception>
#include <iostream>

// 读取当前工作目录下的学生信息表，显示工作表信息和 1 班数据。
int main()
{
    try
    {
        OpenXLSX::XLDocument doc;
        doc.open("studentInfo.xlsx");
        auto workbook = doc.workbook(); // 获取工作簿对象

        std::cout << "工作表数量：" << workbook.worksheetCount() << std::endl;
        // worksheetNames() 返回普通工作表名称。
        for (const auto &name : workbook.worksheetNames())
        {
            std::cout << "工作表名称：" << name << "，类型：";
            if (workbook.typeOfSheet(name) == OpenXLSX::XLSheetType::Worksheet)
            {
                std::cout << "普通工作表" << std::endl;
            }
            else
            {
                std::cout << "其他工作表" << std::endl;
            }
        }

        auto sheet = workbook.worksheet("1班");
        auto rows = sheet.rowCount();
        auto cols = sheet.columnCount();

        // 第一行是表头，后续各行是学生信息。
        for (unsigned int row = 1; row <= rows; ++row)
        {
            for (unsigned int col = 1; col <= cols; ++col)
            {
                std::cout << sheet.cell(row, col).getString(); // cell传入行号和列号，返回单元格对象
                if (col < cols)
                    std::cout << "\t";
            }
            std::cout << std::endl;
        }
        doc.close();
    }
    catch (const std::exception &e)
    {
        std::cerr << "解析 Excel 文件失败：" << e.what() << std::endl;
        return 1;
    }
    return 0;
}
