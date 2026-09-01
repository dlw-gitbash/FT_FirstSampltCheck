cmake_minimum_required(VERSION 3.5)

project(FT_FirstSampltCheck LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ================================================================
# 自动探测 Qt6 / Qt5（优先 Qt6，找不到再找 Qt5）
# ================================================================
set(QT_COMPONENTS Core Gui Widgets)
set(QT_VERSION_MAJOR "")

find_package(Qt6 COMPONENTS ${QT_COMPONENTS} QUIET)
if(Qt6_FOUND)
    set(QT_VERSION_MAJOR 6)
else()
    find_package(Qt5 COMPONENTS ${QT_COMPONENTS} REQUIRED)
    set(QT_VERSION_MAJOR 5)
endif()

message(STATUS "Found Qt${QT_VERSION_MAJOR}")

# ================================================================
# 项目源文件：头文件 + 源文件 统一显式列出（不要 GLOB，增量构建才可靠）
#   - 头文件加入 add_executable：让 Qt AUTOMOC 正确扫描所有 Q_OBJECT 类
#   - FT_AbstractHintWidget：重构后新增的 hint 绘制复用基类（cpp 要编，h 要 MOC 扫描）
# ================================================================
add_executable(FT_FirstSampltCheck
    # ---- 头文件（给 AUTOMOC / IDE 索引）----
    FT_Core.h
    FT_AbstractHintWidget.h
    FT_TextEdit.h
    FT_LineEdit.h
    FT_ComBox.h
    FT_SpinBox.h
    FT_AdvanceDialog.h
    FT_FunctionWidget.h
    FT_TBoxFunctionWidget.h
    FT_FunctionWidgetItem.h
    FT_Edit.h
    # ---- 源文件 ----
    FT_Core.cpp
    FT_AbstractHintWidget.cpp
    FT_TextEdit.cpp
    FT_LineEdit.cpp
    FT_ComBox.cpp
    FT_SpinBox.cpp
    FT_AdvanceDialog.cpp
    FT_FunctionWidget.cpp
    FT_TBoxFunctionWidget.cpp
    FT_FunctionWidgetItem.cpp
    FT_Edit.cpp
    main.cpp
)

# MSVC 下的编译选项：
#   /utf-8            ：源文件/执行字符集 UTF-8（避免中文 hint 乱码）
#   /Zc:__cplusplus   ：让 __cplusplus 宏正确反映 C++17（C++ 标准库头文件需要）
#   /permissive-      ：关闭 MSVC 扩展，两阶段名称查找，减少 Qt MOC 生成代码的诡异编译错误
if(MSVC)
    target_compile_options(FT_FirstSampltCheck PRIVATE /utf-8 /Zc:__cplusplus /permissive-)
endif()

if(QT_VERSION_MAJOR EQUAL 6)
    target_link_libraries(FT_FirstSampltCheck PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets)
else()
    target_link_libraries(FT_FirstSampltCheck PRIVATE Qt5::Core Qt5::Gui Qt5::Widgets)
endif()