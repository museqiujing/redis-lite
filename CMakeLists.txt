cmake_minimum_required(VERSION 3.10)
project(redis-list)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 源文件
set(SOURCE_FILES
    src/skiplist.cpp
    src/sds.cpp
    test/skiplist_test.cpp
)

# 可执行文件
add_executable(redis-list ${SOURCE_FILES})
add_executable(sds-test src/sds.cpp test/sds_test.cpp)

# 包含目录
target_include_directories(redis-list PRIVATE src)
target_include_directories(sds-test PRIVATE src)