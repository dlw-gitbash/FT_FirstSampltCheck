cmake_minimum_required(VERSION 3.16)
project(FT_Edit LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_executable(FT_Edit WIN32
    main.cpp
    FT_Edit.cpp
    FT_FunctionRow.cpp
    FT_FunctionFactory.cpp
    FT_TboxFunctionRow.cpp
    FT_AdvanceDialog.cpp
    FT_FunctionCard.cpp
    FHintWidgets.cpp
    resources.qrc
)

target_include_directories(FT_Edit PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party
)

target_link_libraries(FT_Edit PRIVATE Qt6::Widgets)