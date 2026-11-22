# OpenXLSX库

在C++语言中，操作Excel文件的第三方库比较多，以下是一些第三方库的对比：

| 库名称 | 支持格式 | 跨平台 | 依赖情况 | 协议 / 商用 | 上手难度 | 核心特点 |
|---|---|---|---|---|---|---|
| OpenXLSX | .xlsx（仅新版） | 全平台 | 无外部依赖，头文件集成 | MIT 开源，可商用 | 低 | C++17、轻量、API 简洁、内存友好 |
| QXlsx | .xlsx | 全平台 | 依赖 Qt | MIT 开源 | 中 | 功能全，适合 Qt 项目，样式 / 图表强 |
| xlnt | .xlsx | 全平台 | zlib 等小依赖 | MIT 开源 | 中 | 现代 C++，功能完整，文档友好 |
| libxlsxwriter | .xlsx（只写） | 全平台 | zlib | 宽松开源 | 低 | 写性能极强，适合大量导出数据 |
| LibXL | .xls + .xlsx | 全平台 | 无 | 商业付费 | 中 | 性能最强，双格式支持，稳定 |
| libxls | .xls（仅旧版） | 全平台 | 小依赖 | 开源 | 中 | 只读 .xls，不支持新版 |
| COM/Office | .xls/.xlsx | 仅Windows | 必须安装 Office | 依赖系统 | 高 | 功能最全，但平台绑定严重 |

## OpenXLSX库简介

OpenXLSX 是基于C++17实现的一个轻量级、跨平台、易上手的C++开源库，专注于创建、读写、修改现代.xlsx 格式的Excel文件。提供了简洁易用的 API 接口，让开发者无需深入了解复杂的 Excel 文件格式即可高效操作电子表格数据。

Excel文件实际是由多个XML文件描述的，最后经过压缩成为.xlsx后缀的文件。任意一个 .xlsx 文件后缀改成 .zip，直接解压，就能看到里面文件夹和 XML 文件。

三个核心技术的协同工作：

- **XML解析引擎(PugiXML)**

  负责解析文件中的XML内容，将其转换为程序可以理解的数据结构。每次打开Excel文件时，OpenXLSX都会使用PugiXML读取并解析其中的各个XML部分，如工作表数据、样式信息和共享字符串等。

- **ZIP压缩处理(Zippy)**

  负责处理Excel文件的ZIP压缩格式，包括文件的打包、解包和压缩率优化。当保存Excel文件时，Zippy会将所有修改过的XML文件重新压缩成标准的.xlsx格式。

- **跨平台字符处理(nowide)**

  解决Windows系统上处理非ASCII字符文件名的难题，确保在不同操作系统上都能正确读取和写入包含特殊字符的文件路径。

这些库均为头文件，已包含在代码库中，无需单独下载和构建。

**核心优势**

- **零Office依赖：** 无需安装Microsoft Office
- **跨平台：** 完美支持Windows、macOS和Linux系统，兼容主流编译器如GCC、Clang和MSVC
- **高效性能：** 采用优化的XML解析引擎和内存管理，支持流式解析技术操作大Excel文件，避免了将整个文件加载到内存导致的崩溃风险。
- **丰富功能：** 支持单元格操作、样式设置、公式写入、批注管理、工作表保护等完整功能集
- **简洁API：** 直观的面向对象接口设计，大幅降低Excel文件处理的开发复杂度

## OpenXLSX库安装

下载OpenXLSX源码并编译库

```bash
# 安装pugixml依赖
sudo apt update
sudo apt install libpugixml-dev

# 源码拉取
git clone https://github.com/troldal/OpenXLSX.git
cd OpenXLSX

# 创建构建目录并编译
mkdir build
cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/usr
# -j参数表示启用并行编译，允许同时运行多个编译任务，显著加快编译速度
# $(nproc): shell命令替换
# nproc命令会返回系统的CPU核心数量
# $(nproc) 会被替换为实际的CPU核心数
make -j$(nproc)

# 安装库
sudo make install
```

安装好之后，在编译程序时可能会遇到如下错误：

```text
In file included from /usr/local/include/OpenXLSX/headers/XLCellValue.hpp:66,
                 from /usr/local/include/OpenXLSX/headers/XLCell.hpp:62,
                 from /usr/local/include/OpenXLSX/OpenXLSX.hpp:49,
                 from testOpenXLSX.cpp:1:
/usr/local/include/OpenXLSX/headers/XLXmlParser.hpp:52:10: fatal error:
external/pugixml/pugixml.hpp: No such file or directory
   52 | #include <external/pugixml/pugixml.hpp> // not sure why the full include path is needed within the header file
```

原因： OpenXLSX 头文件中硬编码了 external/pugixml/pugixml.hpp 的路径，但系统安装的 pugixml 头文件在标准位置。OpenXLSX 需要使用自己内置的 pugixml 版本，而不是系统的。

解决方法：

```bash
# 创建必要的目录结构
sudo mkdir -p /usr/include/external/pugixml

# 创建符号链接
sudo ln -s /home/dev/workspace/chat2Data/dependLib/OpenXLSX/OpenXLSX/external/pugixml/pugixml.hpp /usr/include/external/pugixml/pugixml.hpp
```

## OpenXLSX支持单元格数据类型

OpenXLSX 中，单元格值由 XLCellValue 表示，对应一个枚举：

```cpp
enum class XLValueType {
    Empty,
    Boolean,
    Integer,
    Float,
    String,
    Error
};
```

## OpenXLSX常用方法介绍

OpenXLSX中5个核心类：

| 类名 | 作用 |
|---|---|
| XLDocument | Excel 文件本身 |
| XLWorkbook | 工作簿 |
| XLWorksheet | 工作表 |
| XLCell | 单元格 |
| XLCellValue | 单元格的值 |

### XLDocment中常用函数

```cpp
/*
*   功能：用指定文件路径创建文档对象
*   参数：
*        docPath：指定Excel文件在磁盘上的位置
         zipArchive：压缩方式，默认使用自定你的ZIP压缩/解压缩实现
*/
explicit XLDocument(const std::string& docPath,
                    const IZipArchive& zipArchive = XLZipArchive());

/*
*   功能：打开 filename 指定的Excel文件，注意OpenXLSX库只支持.xlsx后缀的Excel文件
*   参数：
*        fileName：要打开的.xlsx后缀的excel文件的路径
*/
void open(const std::string& fileName);

/*
*   功能：创建全新的.xlsx格式的Excel文件
*   参数：
*        fileName：指定要创建的Excel文件的保存位置和名称，必须包含完整的文件路径和.xlsx扩展名
         forceOverwrite：是否强制覆盖
           false：保护模式，即如果目标文件已存在，则函数会抛出异常，阻止覆盖
           true：覆盖模式，如果文件已存在，直接覆盖它(不提示，不警告)
    异常：
    XLException异常：触发条件 - OpenXLSX内部验证失败，比如文件路径无效、权限不足、文件命名不规范等
    ZipRuntimeError：触发条件 - 底层Zippy库处理压缩文件出错
*/
void create(const std::string& fileName, bool forceOverwrite)

/*
*   功能：将内存中的Excel数据写入物理文件，会覆盖原有文件。相当于点击Excel软件的保存按钮，将当前所有修改持久化到磁盘。sava()之后可继续编辑
    异常：
         XLException：数据验证失败或库内部错误
         ZipRuntimeError：压缩或文件写入失败(磁盘满、权限不足等)
*/
void save()

/*
*   功能：关闭excel文件并清理资源。注意：close()之后不能编辑，除非重新load(xxx.xlsx)
          清理内存：释放所有工作表、单元格数据的内存(不删除物理文件)
          断开连接：接触与物理文件的关联
          状态重置：将对象恢复到可加载新文件的状态
          不保存：如果数据未保存，修改会丢失
    异常：
         XLException：数据验证失败或库内部错误
         ZipRuntimeError：压缩或文件写入失败(磁盘满、权限不足等)
*/
void close()

/*
*   功能：将当前文档另存为新文件。相当于Excel中的另存为功能，创建当前工作簿的一个独立副本
    参数：
          fileName：新文件路径，必须包含完整路径和.xlsx扩展名
          forceOverwrite：false - 保护模式   true - 覆盖模式
    异常：
         XLException：OpenXLSX检查失败触发(新路径无效、文件名非法、磁盘空间不足等)
         ZipRuntimeError：压缩或文件访问失败触发(目标目录不可写、文件被其他程序锁定等)
*/
void saveAs(const std::string& fileName, bool forceOverwrite);

/*
*   功能：获取当前文档的文件名
*/
std::string name() const;

/*
*   功能：获取当前文档的完整路径
*/
const std::string& path() const;

/*
*   功能：获取工作簿对象，提供对底层Excel工作簿的访问权限
    返回值：返回XLWorkbook对象
*/
XLWorkbook workbook() const;
```

### XLWorkbook中常用函数

XLWorkbook类对应Excel文件的工作簿。工作簿(workbook)就是整个Excel文件本身。工作簿中包含多个工作表(worksheet)，工作表当中包含多个单元格(cell)。

```text
📁 工作簿（整个文件夹）
├── 📄 工作表1（Sheet1）
│   ├── A1单元格
│   ├── B1单元格
│   └── ...
├── 📄 工作表2（Sheet2）
├── 📄 图表工作表（Chart1）
└── 📄 ...（更多工作表）
```

Excel工作表类型总共有四种：

```cpp
enum class XLSheetType { Worksheet, Chartsheet, Dialogsheet, Macrosheet };
```

- **Worksheet：** 普通工作表，标准的网格状电子表，包含单元格(可输入数据和公式)、行列、格式设置(字体、颜色、边框)、图表(嵌入在工作表内部)、数据验证、条件格式等
- **Chartsheet：** 整个工作表只包含一个图表
- ~~**Dialogsheet：** 创建自定义对话框界面，包含元素按钮、文本框、复选框等控件，Excel 5.0/95时代的特性，先基本已废弃~~
- ~~**Macrosheet：** Excel 4.0(1992年) 宏语言的专用工作表，基本已经不用。~~

以下仅介绍Worksheet的相关函数

```cpp
/*
*  功能：通过工作表名获取普通工作表对象
*  参数：
*        sheetName：要查找的工作表的完整名称。大小写敏感、空格敏感，必须完全匹配
*  返回值： 成功时返回普通工作表 XLWorksheet 对象，失败时抛出异常
*/
XLWorksheet worksheet(const std::string& sheetName);

/*
*  功能：通过位置索引获取工作簿中的第N个普通工作表
*  参数：
*        index：要获取的工作表在普通工作表列表中的索引，取值范围 1 ≤ index ≤ 工作簿中的普通工作表总数，注意索引从1开始，图表工作表不计入索引
*  返回值： 成功时返回第 index 个普通工作表，失败时抛出越界异常
*/
XLWorksheet worksheet(uint16_t index);

/*
*  功能：在工作簿末尾添加一个新的空白普通工作表，添加失败时抛出 XLException 异常
*/
void addWorksheet(const std::string& sheetName);

/*
*  功能：永久删除工作簿中名为 sheetName 的工作表，添加失败时抛出 XLException 异常
*/
void deleteSheet(const std::string& sheetName);

/*
*  功能：复制现有工作表，添加到工作簿末尾
*  参数：
*        existingName：源工作表的名称，必须是存在的有效工作表名
         newName：新工作表的名称，不能与现有的工作表名称重复
*  完全复制的内容：
*        - 所有单元格数据(值、公式、格式)
*        - 行列设置(宽度、高度、隐藏状态)
*        - 合并单元格范围
*        - 数据验证规则
*        - 条件格式设置
*        - 页面设置(打印区域、页眉页脚)
*        - 工作表保护设置
*  复制后的工作表是独立实体，修改、移动、删除复制后的工作表不会影响源工作表
*/
void cloneSheet(const std::string& existingName, const std::string& newName);

/*
*  功能：移动指定工作表到目标位置，会改变工作表在工作簿中的物理排序顺序
*  参数：
*        sheetName：要移动的工作表名称，必须存在且有效
*        index：目标索引位置，取值范围 1 ≤ index ≤ 工作簿中的普通工作表总数
*/
void setSheetIndex(const std::string& sheetName, unsigned int index);

/*
*  功能：查询指定工作表的当前位置索引
*  参数：
*        sheetName：要查询的工作表名称，必须存在且有效
*  返回值：返回该工作表在当前工作簿中的位置序号，如果不存在则抛出异常
*/
unsigned int indexOfSheet(const std::string& sheetName) const;

/*
*  功能：按照工作表名称查询工作表的类型
*  参数：
*        sheetName：要查询的工作表名称，必须存在且有效
*  返回值：返回工作表的实际类型
*/
XLSheetType typeOfSheet(const std::string& sheetName) const;

/*
*  功能：按照工作表索引查询工作表的类型
*  参数：
*        index：工作表在当前工作簿中的物理顺序
*  返回值：返回工作表的实际类型
*/
XLSheetType typeOfSheet(unsigned int index) const;

/*
*  功能：获取当前工作簿中工作表的个数
*  返回值： 返回当前工作簿中工作表的个数
*/
unsigned int worksheetCount() const;

/*
*  功能：获取当前工作簿中所有工作表的名称
*  返回值： 返回当前工作簿中所有工作表的名称
*/
std::vector<std::string> worksheetNames() const;

/*
 *  功能：检测指定名称的工作表是否存在
 *  参数：
 *        sheetName：要检测的工作表的名称
 *  返回值： true表示存在，false表示不存在
 */
bool worksheetExists(const std::string& sheetName) const;
```

### XLWorksheet中常用函数介绍

```cpp
/*
 *  功能：获取工作表名称
 *  返回值： 返回当前工作表的名称
 */
std::string name() const;

/**
 *   功能：设置工作表名称
 *   参数：
 *         name：新的工作表名称，注意名称不能包含: \ / ? * [ ]
 *   异常：当名称已存在、包含非法字符或超过31个字符时抛出XLException
 */
void setName(const std::string& name);

/**
 *   功能：更新Worksheet表名
 *   参数：
 *         oldName：待更新的Worksheet表名
 *         newName：更新后的Worksheet表名
 */
void updateSheetName(const std::string& oldName, const std::string& newName);

///////////////////////// 单元格获取 //////////////////////////////
/**
 *   功能：通过单元格引用获取单元格，支持单个单元格和单元格范围
 *   参数：
 *        ref：为单元格引用，如"A1"
 *   返回值： 单元格对象。注意XLCellAssignable 继承自 XLCell
 */
XLCellAssignable cell(const std::string& ref);

/**
 *   功能：通过行号和列号获取单元格
 *   参数：
 *         rowNumber：行号
 *         columnNuumber：列号
 *   返回值： 返回单元格对象。
 *   异常：当行号或列号越界时抛出异常
 */
XLCellAssignable cell(uint32_t rowNumber, uint16_t columnNumber) const

///////////////////////// 范围访问 //////////////////////////////
/**
 *   功能：自动检测工作表中已使用的区域。从A1到最后一个包含数据/格式的单元格
 *   返回值：返回XLCellRange 对象，可迭代访问所有单元格
 */
XLCellRange range() const;

/**
 *   功能：通过字符串指定范围对角，获取指定范围字符串。
 *   参数：
 *         topLeft：左上单元格字符串编号  A2
 *         bottomRight：右下单元格字符串编号  E10
 *         toLeft 和 bottomRight 刚好可以包含一个指定的矩形区域
 *   返回值： 返回单元格对象。
 *   异常：字符串格式错误会抛出异常
 */
XLCellRange range(std::string const& topLeft, std::string const& bottomRight) const;

/**
 *   功能：解析 Excel 风格的范围字符串（如 "A2:B10"）
 *   参数：
 *         rangeReference：冒号分隔的两个单元格引用
 *   返回值： 返回单元格对象。
 *   异常：字符串格式错误会抛出异常
 */
XLCellRange range(std::string const& rangeReference) const;

/**
 *   功能：获取工作表总列数
 *   返回值： 返回工作表总列数
 *   异常：字符串格式错误会抛出异常
 */
uint16_t columnCount() const noexcept;

/**
 *   功能：获取工作表的总行数
 *   返回值： 返回工作表总行数
 */
uint32_t rowCount() const noexcept;

/**
 *   功能：删除工作表的指定行
 *   返回值： true-存在并删除成功，false-该行不存在
 */
bool deleteRow(uint32_t rowNumber);

/**
 *   功能：获取所有行
 *   返回值： 返回所有行数据范围
 */
XLRowRange rows() const;

/**
 *   功能：获取第 1 到 指定第 n 行
 *   参数：
 *         rowCount：指定获取结束行号
 *   返回值： 返回固定标题行数的数据
 */
XLRowRange rows(uint32_t rowCount) const;

/**
 *   功能：获取指定范围的行
 *   参数：
 *         firstRow：起始行号
 *         lastRow：结束行号
 *   返回值： 返回firstRow 和 lastRow范围内的行
 */
XLRowRange rows(uint32_t firstRow, uint32_t lastRow) const;

/**
 *   功能：获取指定行的单行数据
 *   参数：
 *         rowNumber：要获取行数据的行号
 *   返回值： 返回 rowNumber 行数据
 */
XLRow row(uint32_t rowNumber) const;

/**
 *   功能：获取指定列号的一整列数据
 *   参数：
 *         columnNumber：要获取整列数据的列号
 *   返回值： 返回 columnNumber 列数据
 */
XLColumn column(uint16_t columnNumber) const;

/**
 *   功能：通过字符获取整列
 *   参数：
 *         columnRef：列标，比如"A"
 *   返回值： 返回 columnRef 列数据
 */
XLColumn column(std::string const& columnRef) const;
```

