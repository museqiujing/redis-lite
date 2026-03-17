 #ifndef SDS_H
#define SDS_H

#include <cstdint>
#include <cstring>
#include <string>
#include <memory>
#include <stdexcept>

// SDS（Simple Dynamic String）类 
class SDS {
public:
    // 构造函数
   SDS() : len(0), free(0) { buf = new char[1]; buf[0] = '\0'; }
    explicit SDS(const char* init);
    explicit SDS(const std::string& init);
    SDS(const char* init, size_t initlen);
    
    // 复制构造函数和赋值运算符
    SDS(const SDS& other);
    SDS& operator=(const SDS& other);
    
    // 移动构造函数和移动赋值运算符
    SDS(SDS&& other) noexcept;
    SDS& operator=(SDS&& other) noexcept;
    
    // 析构函数
    ~SDS();
    
    // 获取长度
    size_t length() const { return len; }
    
    // 获取可用空间
    size_t available() const { return free; }
    
    // 获取C风格字符串
    const char* c_str() const { return buf; }
    
    // 字符串操作
    void append(const char* t);
    void append(const std::string& t);
    void append(const SDS& t);
    void truncate(size_t new_len);
    
    // 比较操作
    int compare(const SDS& other) const;
    bool operator==(const SDS& other) const;
    bool operator!=(const SDS& other) const;
    bool operator<(const SDS& other) const;
    bool operator<=(const SDS& other) const;
    bool operator>(const SDS& other) const;
    bool operator>=(const SDS& other) const;
    
    // 转换为std::string
    std::string to_string() const;
    
    // 内存管理
    void reserve(size_t needed);
    
private:
    uint32_t len;    // 字符串长度
    uint32_t free;   // 可用空间
    char* buf;       // 存储字符串数据
    
    // 空间预分配策略
    static constexpr size_t SDS_MAX_PREALLOC = 1024 * 1024;
    
    // 辅助方法
    void allocate(size_t size);
    void reallocate(size_t new_cap);
};

#endif // SDS_H