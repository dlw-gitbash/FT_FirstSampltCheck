cmake_minimum_required(VERSION 3.16)
project(FT_Edit VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_executable(FT_Edit WIN32
    main.cpp
    FT_Edit.h
    FT_Edit.cpp
    Module/FT_Type.h
    Module/FT_Data.h
    Module/FT_Data.cpp
    Module/FT_Widget.h
    Module/FT_Widget.cpp
    Module/FT_Function.h
    Module/FT_Function.cpp
    Module/FT_FunctionGroup.h
    Module/FT_FunctionGroup.cpp
    Module/FT_FunctionItem.h
    Module/FT_FunctionItem.cpp
    Module/FT_FunctionTree.h
    Module/FT_FunctionTree.cpp
    Module/FT_FunctionList.h
    Module/FT_FunctionList.cpp
    Module/FT_Project.h
    Module/FT_Project.cpp
    Module/FT_Test.h
    Module/FT_Test.cpp
    Widgets/LogModel.cpp
    Widgets/LogFilterProxy.cpp
    Widgets/LogHighlighter.cpp
    Widgets/LogTableView.cpp
    Widgets/LogToolBar.cpp
    resources.qrc
)

target_include_directories(FT_Edit PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party
    ${CMAKE_CURRENT_SOURCE_DIR}/Module
    ${CMAKE_CURRENT_SOURCE_DIR}/Widgets
)

target_link_libraries(FT_Edit PRIVATE Qt6::Widgets)