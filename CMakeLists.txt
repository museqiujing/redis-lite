cmake_minimum_required(VERSION 3.10)
project(redis-list)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Release)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")

# 源文件
set(SOURCE_FILES
    src/skiplist.cpp
    src/sds.cpp
    src/epoll_server.cpp
    src/resp_parser.cpp
    src/resp_serializer.cpp
    src/command_handler.cpp
    src/string_type.cpp
    src/list.cpp
    src/aof.cpp
    src/zset.cpp
    src/main.cpp
    src/timewheel.cpp
     src/lockfree_expiration.cpp 
)

# 可执行文件
add_executable(redis-list ${SOURCE_FILES})
# 测试可执行文件
add_executable(skiplist-test src/skiplist.cpp src/sds.cpp test/skiplist_test.cpp)
add_executable(sds-test src/sds.cpp test/sds_test.cpp)
add_executable(string-test src/sds.cpp src/string_type.cpp src/timewheel.cpp src/lockfree_expiration.cpp test/string_type_test.cpp)
add_executable(list-test src/sds.cpp src/list.cpp test/list_test.cpp)
add_executable(lockfree-test src/lockfree_expiration.cpp src/sds.cpp test/lockfree_expiration_test.cpp)
 

# 包含目录
target_include_directories(redis-list PRIVATE src)
target_include_directories(sds-test PRIVATE src)
target_include_directories(string-test PRIVATE src)
target_include_directories(list-test PRIVATE src)
target_include_directories(skiplist-test PRIVATE src)