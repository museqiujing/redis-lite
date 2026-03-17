#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <vector>
#include "../src/list.h"

void test_basic_operations() {
    std::cout << "=== 测试基本操作 ===" << std::endl;
    
    List list;
    
    // 测试lpush
    list.lpush("mylist", "value1");
    list.lpush("mylist", "value2");
    list.lpush("mylist", "value3");
    
    // 测试llen
    long long len = list.llen("mylist");
    std::cout << "llen mylist: " << len << " (expected: 3)" << std::endl;
    
    // 测试lrange
    std::vector<std::string> range = list.lrange("mylist", 0, -1);
    std::cout << "lrange mylist 0 -1: ";
    for (const auto& val : range) {
        std::cout << val << " ";
    }
    std::cout << "(expected: value3 value2 value1)" << std::endl;
    
    // 测试lpop
    std::string value = list.lpop("mylist");
    std::cout << "lpop mylist: " << value << " (expected: value3)" << std::endl;
    
    len = list.llen("mylist");
    std::cout << "llen mylist after lpop: " << len << " (expected: 2)" << std::endl;
    
    // 测试rpush
    list.rpush("mylist", "value4");
    len = list.llen("mylist");
    std::cout << "llen mylist after rpush: " << len << " (expected: 3)" << std::endl;
    
    // 测试rpop
    value = list.rpop("mylist");
    std::cout << "rpop mylist: " << value << " (expected: value4)" << std::endl;
    
    len = list.llen("mylist");
    std::cout << "llen mylist after rpop: " << len << " (expected: 2)" << std::endl;
    
    // 测试空链表
    value = list.lpop("empty");
    std::cout << "lpop empty: " << value << " (expected: empty)" << std::endl;
    
    len = list.llen("empty");
    std::cout << "llen empty: " << len << " (expected: 0)" << std::endl;
}

void test_range_operations() {
    std::cout << "\n=== 测试范围操作 ===" << std::endl;
    
    List list;
    
    // 插入测试数据
    for (int i = 1; i <= 5; i++) {
        list.rpush("numbers", "value" + std::to_string(i));
    }
    
    // 测试正索引范围
    std::vector<std::string> range = list.lrange("numbers", 0, 2);
    std::cout << "lrange numbers 0 2: ";
    for (const auto& val : range) {
        std::cout << val << " ";
    }
    std::cout << "(expected: value1 value2 value3)" << std::endl;
    
    // 测试负索引
    range = list.lrange("numbers", -3, -1);
    std::cout << "lrange numbers -3 -1: ";
    for (const auto& val : range) {
        std::cout << val << " ";
    }
    std::cout << "(expected: value3 value4 value5)" << std::endl;
    
    // 测试超出范围
    range = list.lrange("numbers", 10, 20);
    std::cout << "lrange numbers 10 20: " << range.size() << " elements (expected: 0)" << std::endl;
    
    // 测试start > stop
    range = list.lrange("numbers", 3, 1);
    std::cout << "lrange numbers 3 1: " << range.size() << " elements (expected: 0)" << std::endl;
}

void test_expire() {
    std::cout << "\n=== 测试过期时间 ===" << std::endl;
    
    List list;
    
    // 测试lpush和expire
    list.lpush("expire_list", "value1");
    list.lpush("expire_list", "value2");
    
    bool expired = list.expire("expire_list", 1);
    std::cout << "expire expire_list: " << expired << " (expected: 1)" << std::endl;
    
    // 测试ttl
    int ttl = list.ttl("expire_list");
    std::cout << "ttl expire_list: " << ttl << " (expected: 0 or 1)" << std::endl;
    
    // 测试exists
    bool exists = list.exists("expire_list");
    std::cout << "exists expire_list: " << exists << " (expected: 1)" << std::endl;
    
    // 等待过期
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // 测试过期后
    exists = list.exists("expire_list");
    std::cout << "exists expire_list after 2s: " << exists << " (expected: 0)" << std::endl;
    
    long long len = list.llen("expire_list");
    std::cout << "llen expire_list after 2s: " << len << " (expected: 0)" << std::endl;
    
    ttl = list.ttl("expire_list");
    std::cout << "ttl expire_list after 2s: " << ttl << " (expected: -2)" << std::endl;
}

void test_del() {
    std::cout << "\n=== 测试删除操作 ===" << std::endl;
    
    List list;
    
    // 测试lpush
    list.lpush("del_list", "value1");
    list.lpush("del_list", "value2");
    
    // 测试del
    bool deleted = list.del("del_list");
    std::cout << "del del_list: " << deleted << " (expected: 1)" << std::endl;
    
    bool exists = list.exists("del_list");
    std::cout << "exists del_list after del: " << exists << " (expected: 0)" << std::endl;
    
    long long len = list.llen("del_list");
    std::cout << "llen del_list after del: " << len << " (expected: 0)" << std::endl;
    
    // 测试删除不存在的键
    deleted = list.del("non_existent");
    std::cout << "del non_existent: " << deleted << " (expected: 0)" << std::endl;
}

int main() {
    test_basic_operations();
    test_range_operations();
    test_expire();
    test_del();
    std::cout << "\n=== 所有测试完成 ===" << std::endl;
    return 0;
}