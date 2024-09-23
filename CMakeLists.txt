cmake_minimum_required(VERSION 3.29)
project(BloonsAutoFarm)

set(CMAKE_CXX_STANDARD 20)

include_directories(libs/jsonParser)

set(SOURCES
    src/main.cpp
        src/CoordinateHandler.cpp
        include/CoordinateHandler.h)

file(GLOB_RECURSE HEADERS include/*.h)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

qt_add_executable(BloonsAutoFarm ${SOURCES})

target_link_libraries(BloonsAutoFarm PRIVATE Qt6::Widgets)

target_include_directories(BloonsAutoFarm PUBLIC include)
