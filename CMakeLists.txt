cmake_minimum_required(VERSION 3.17)
project(
    ConcurBench
    LANGUAGES C CXX
    VERSION 1.0.0
    DESCRIPTION "Concurrency Testing"
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

set(SOURCES
    main.cpp
    src/ThreadPool.cpp
)

add_executable(ConcurBench ${SOURCES})

# $<BUILD_INTERFACE:...>：这个表达式指定了构建接口路径，通常指代源代码目录中的头文件路径。
# CMAKE_CURRENT_SOURCE_DIR 是 CMake 内置的变量，它指向当前 CMakeLists.txt 所在的目录。
target_include_directories(ConcurBench PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)

