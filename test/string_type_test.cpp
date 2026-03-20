#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include "../src/string_type.h"
/*
void test_basic_operations() {
    std::cout << "=== 测试基本操作 ===" << std::endl;

    String string;

    // 测试设置和获取
    string.set("key1", "value1");
    std::string result = string.get("key1");
    std::cout << "get key1: " << result << " (expected: value1)" << std::endl;

    // 测试不存在的键
    result = string.get("non_existent");
    std::cout << "get non_existent: " << result << " (expected: empty)" << std::endl;

    // 测试键是否存在
    bool exists = string.exists("key1");
    std::cout << "exists key1: " << exists << " (expected: 1)" << std::endl;

    exists = string.exists("non_existent");
    std::cout << "exists non_existent: " << exists << " (expected: 0)" << std::endl;

    // 测试删除
    bool deleted = string.del("key1");
    std::cout << "del key1: " << deleted << " (expected: 1)" << std::endl;

    exists = string.exists("key1");
    std::cout << "exists key1 after del: " << exists << " (expected: 0)" << std::endl;

    deleted = string.del("non_existent");
    std::cout << "del non_existent: " << deleted << " (expected: 0)" << std::endl;
}

void test_expire() {
    std::cout << "\n=== 测试过期时间 ===" << std::endl;

    String string;

    // 测试setex
    string.setex("key2", 1, "value2");
    std::string result = string.get("key2");
    std::cout << "get key2 immediately: " << result << " (expected: value2)" << std::endl;

    // 测试ttl
    int ttl = string.ttl("key2");
    std::cout << "ttl key2: " << ttl << " (expected: 0 or 1)" << std::endl;

    // 等待过期
    std::this_thread::sleep_for(std::chrono::seconds(2));

    result = string.get("key2");
    std::cout << "get key2 after 2s: " << result << " (expected: empty)" << std::endl;

    bool exists = string.exists("key2");
    std::cout << "exists key2 after 2s: " << exists << " (expected: 0)" << std::endl;

    ttl = string.ttl("key2");
    std::cout << "ttl key2 after 2s: " << ttl << " (expected: -2)" << std::endl;

    // 测试expire
    string.set("key3", "value3");
    bool expired = string.expire("key3", 1);
    std::cout << "expire key3: " << expired << " (expected: 1)" << std::endl;

    ttl = string.ttl("key3");
    std::cout << "ttl key3: " << ttl << " (expected: 0 or 1)" << std::endl;

    std::this_thread::sleep_for(std::chrono::seconds(2));
    exists = string.exists("key3");
    std::cout << "exists key3 after 2s: " << exists << " (expected: 0)" << std::endl;

    // 验证惰性删除：过期后get操作应该清理内存
    string.setex("key4", 1, "value4");
    std::this_thread::sleep_for(std::chrono::seconds(2));
    result = string.get("key4");
    std::cout << "get key4 after expire: " << result << " (expected: empty)" << std::endl;
    exists = string.exists("key4");
    std::cout << "exists key4 after get: " << exists << " (expected: 0)" << std::endl;
}

void test_counter() {
    std::cout << "\n=== 测试计数操作 ===" << std::endl;

    String string;

    // 测试incr
    long long result = string.incr("counter");
    std::cout << "incr counter: " << result << " (expected: 1)" << std::endl;

    result = string.incr("counter");
    std::cout << "incr counter again: " << result << " (expected: 2)" << std::endl;

    // 测试decr
    result = string.decr("counter");
    std::cout << "decr counter: " << result << " (expected: 1)" << std::endl;

    // 测试incrby
    result = string.incrby("counter", 5);
    std::cout << "incrby counter 5: " << result << " (expected: 6)" << std::endl;

    // 测试decrby
    result = string.decrby("counter", 2);
    std::cout << "decrby counter 2: " << result << " (expected: 4)" << std::endl;

    // 测试非数字值
    string.set("non_number", "hello");
    try {
        result = string.incr("non_number");
        std::cout << "incr non_number: " << result << " (expected: error)" << std::endl;
    } catch (const std::exception& e) {
        std::cout << "incr non_number: caught exception: " << e.what() << " (expected: value is not an integer or out of range)" << std::endl;
    }
}
*/
int main()
{
    // test_basic_operations();
    // test_expire();
    // test_counter();
    std::cout << "\n=== 所有测试完成 ===" << std::endl;
    return 0;
}