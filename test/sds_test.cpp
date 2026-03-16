#include "../src/sds.h"
#include <iostream>
#include <string>

void test_sds_construction() {
    std::cout << "=== 测试SDS构造函数 ===" << std::endl;
    
    // 测试默认构造函数
    SDS s1;
    std::cout << "默认构造函数: len=" << s1.length() << ", available=" << s1.available() << ", c_str()=" << s1.c_str() << std::endl;
    
    // 测试从C字符串构造
    SDS s2("Hello, SDS!");
    std::cout << "从C字符串构造: len=" << s2.length() << ", available=" << s2.available() << ", c_str()=" << s2.c_str() << std::endl;
    
    // 测试从std::string构造
    std::string str = "Hello, std::string!";
    SDS s3(str);
    std::cout << "从std::string构造: len=" << s3.length() << ", available=" << s3.available() << ", c_str()=" << s3.c_str() << std::endl;
    
    // 测试从指定长度的字符串构造
    SDS s4("Hello, world!", 5);
    std::cout << "从指定长度构造: len=" << s4.length() << ", available=" << s4.available() << ", c_str()=" << s4.c_str() << std::endl;
}

void test_sds_copy_and_move() {
    std::cout << "\n=== 测试SDS复制和移动操作 ===" << std::endl;
    
    // 测试复制构造函数
    SDS s1("Test string");
    SDS s2(s1);
    std::cout << "复制构造函数: s1.c_str()=" << s1.c_str() << ", s2.c_str()=" << s2.c_str() << std::endl;
    
    // 测试赋值运算符
    SDS s3;
    s3 = s1;
    std::cout << "赋值运算符: s1.c_str()=" << s1.c_str() << ", s3.c_str()=" << s3.c_str() << std::endl;
    
    // 测试移动构造函数
    SDS s4(std::move(s1));
    std::cout << "移动构造函数: s4.c_str()=" << s4.c_str() << std::endl;
    
    // 测试移动赋值运算符
    SDS s5;
    s5 = std::move(s4);
    std::cout << "移动赋值运算符: s5.c_str()=" << s5.c_str() << std::endl;
}

void test_sds_string_operations() {
    std::cout << "\n=== 测试SDS字符串操作 ===" << std::endl;
    
    // 测试append操作
    SDS s1("Hello");
    s1.append(", ");
    s1.append("world!");
    std::cout << "append操作: " << s1.c_str() << ", len=" << s1.length() << std::endl;
    
    // 测试append std::string
    SDS s2("Hello");
    std::string append_str = ", SDS!";
    s2.append(append_str);
    std::cout << "append std::string: " << s2.c_str() << ", len=" << s2.length() << std::endl;
    
    // 测试append SDS
    SDS s3("Hello");
    SDS s4(", SDS!");
    s3.append(s4);
    std::cout << "append SDS: " << s3.c_str() << ", len=" << s3.length() << std::endl;
    
    // 测试truncate操作
    SDS s5("Hello, world!");
    s5.truncate(5);
    std::cout << "truncate操作: " << s5.c_str() << ", len=" << s5.length() << ", available=" << s5.available() << std::endl;
}

void test_sds_comparison() {
    std::cout << "\n=== 测试SDS比较操作 ===" << std::endl;
    
    SDS s1("apple");
    SDS s2("banana");
    SDS s3("apple");
    
    std::cout << "s1 = \"apple\", s2 = \"banana\", s3 = \"apple\"" << std::endl;
    std::cout << "s1 == s2: " << (s1 == s2) << std::endl;
    std::cout << "s1 == s3: " << (s1 == s3) << std::endl;
    std::cout << "s1 != s2: " << (s1 != s2) << std::endl;
    std::cout << "s1 < s2: " << (s1 < s2) << std::endl;
    std::cout << "s1 <= s3: " << (s1 <= s3) << std::endl;
    std::cout << "s1 > s2: " << (s1 > s2) << std::endl;
    std::cout << "s1 >= s3: " << (s1 >= s3) << std::endl;
    
    // 测试compare方法
    std::cout << "s1.compare(s2): " << s1.compare(s2) << std::endl;
    std::cout << "s1.compare(s3): " << s1.compare(s3) << std::endl;
    std::cout << "s2.compare(s1): " << s2.compare(s1) << std::endl;
}

void test_sds_memory_management() {
    std::cout << "\n=== 测试SDS内存管理 ===" << std::endl;
    
    // 测试reserve操作
    SDS s1("Hello");
    std::cout << "初始状态: len=" << s1.length() << ", available=" << s1.available() << std::endl;
    
    s1.reserve(100);
    std::cout << "reserve(100)后: len=" << s1.length() << ", available=" << s1.available() << std::endl;
    
    // 测试空间预分配
    SDS s2("Hello");
    std::cout << "\n初始状态: len=" << s2.length() << ", available=" << s2.available() << std::endl;
    
    s2.append(" world! This is a test to see if space preallocation works properly.");
    std::cout << "append后: len=" << s2.length() << ", available=" << s2.available() << std::endl;
    
    s2.append(" More text to test space preallocation.");
    std::cout << "再次append后: len=" << s2.length() << ", available=" << s2.available() << std::endl;
}

void test_sds_conversion() {
    std::cout << "\n=== 测试SDS转换操作 ===" << std::endl;
    
    // 测试to_string方法
    SDS s1("Hello, SDS!");
    std::string str = s1.to_string();
    std::cout << "to_string(): " << str << ", length=" << str.length() << std::endl;
    
    // 测试c_str方法
    const char* c_str = s1.c_str();
    std::cout << "c_str(): " << c_str << std::endl;
}

int main() {
    test_sds_construction();
    test_sds_copy_and_move();
    test_sds_string_operations();
    test_sds_comparison();
    test_sds_memory_management();
    test_sds_conversion();
    
    std::cout << "\n=== SDS测试完成 ===" << std::endl;
    return 0;
}