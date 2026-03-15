cmake_minimum_required(VERSION 3.10)
project(redis-list)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 源文件
set(SOURCE_FILES
    test/skiplist_test.cpp
    src/skiplist.cpp
)

# 可执行文件
add_executable(redis-list ${SOURCE_FILES})

# 包含目录
target_include_directories(redis-list PRIVATE src)