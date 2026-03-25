#ifndef SDS_H
#define SDS_H

#include <cstdint>
#include <cstring>
#include <string>
#include <memory>
#include <stdexcept>
#include <functional>
// SDS（Simple Dynamic String）类
class SDS
{
public:
    // 构造函数
    SDS() : len(0), free(0)
    {
        buf = new char[1];
        buf[0] = '\0';
    }
    SDS(const char *init);
    SDS(const std::string &init);

    // 赋值运算符重载（支持字符串字面量）
    SDS &operator=(const char *str);
    SDS &operator=(const std::string &str);

    // 迭代器支持
    typedef char *iterator;
    typedef const char *const_iterator;

    iterator begin();
    iterator end();
    const_iterator begin() const;
    const_iterator end() const;
    SDS(const char *init, size_t initlen);

    // 复制构造函数和赋值运算符
    SDS(const SDS &other);
    SDS &operator=(const SDS &other);

    // 移动构造函数和移动赋值运算符
    SDS(SDS &&other) noexcept;
    SDS &operator=(SDS &&other) noexcept;

    // 析构函数
    ~SDS();

    // 获取长度
    size_t size() const { return len; }

    // 检查是否为空
    bool empty() const { return len == 0; }

    // 获取可用空间
    size_t available() const { return free; }

    // 获取C风格字符串
    const char *c_str() const { return buf; }

    // 清空字符串
    void clear();

    // 字符串操作
    void append(const char *t);
    void append(const std::string &t);
    void append(const SDS &t);
    void append(const char *data, size_t length);
    void truncate(size_t new_len);

    // 查找操作
    size_t find(const char *substr, size_t pos = 0) const;
    size_t find(const std::string &substr, size_t pos = 0) const;
    size_t find(const SDS &substr, size_t pos = 0) const;

    // 运算符重载
    SDS &operator+=(const char *str);
    SDS &operator+=(const std::string &str);
    SDS &operator+=(const SDS &other);

    // 全局运算符重载
    friend SDS operator+(const SDS &lhs, const SDS &rhs);
    friend SDS operator+(const SDS &lhs, const char *rhs);
    friend SDS operator+(const char *lhs, const SDS &rhs);
    friend SDS operator+(const SDS &lhs, const std::string &rhs);
    friend SDS operator+(const std::string &lhs, const SDS &rhs);

    // 子字符串
    SDS substr(size_t pos, size_t len = std::string::npos) const;

    // 下标运算符重载
    char &operator[](size_t index);
    const char &operator[](size_t index) const;

    // 类型转换
    operator std::string() const;
    operator const char *() const;

    // 比较操作
    int compare(const SDS &other) const;
    bool operator==(const SDS &other) const;
    bool operator==(const char *str) const;
    bool operator==(const std::string &str) const;
    bool operator!=(const SDS &other) const;
    bool operator!=(const char *str) const;
    bool operator!=(const std::string &str) const;
    bool operator<(const SDS &other) const;
    bool operator<=(const SDS &other) const;
    bool operator>(const SDS &other) const;
    bool operator>=(const SDS &other) const;

    // 转换为std::string
    std::string to_string() const;

    // 内存管理
    void reserve(size_t needed);

    void swap(SDS &other) noexcept;

    // 删除操作
    void erase(size_t pos, size_t count = std::string::npos);
    // 删除前缀
    void erase_prefix(size_t count);

private:
    uint32_t len;  // 字符串长度
    uint32_t free; // 可用空间
    char *buf;     // 存储字符串数据

    // 空间预分配策略
    static constexpr size_t SDS_MAX_PREALLOC = 1024 * 1024;

    // 辅助方法
    void allocate(size_t size);      // 分配内存
    void reallocate(size_t new_cap); // 重新分配内存
};

// 为SDS添加std::hash特化
namespace std
{
    template <>
    struct hash<SDS>
    {
        size_t operator()(const SDS &sds) const
        {
            // 使用更高效的djb2哈希算法
            size_t hash = 5381; // djb2算法的初始值
            const char *data = sds.c_str();
            size_t len = sds.size();

            // 批量处理，减少循环次数
            for (size_t i = 0; i < len; i++)
            {
                // djb2算法: hash * 33 + c
                hash = ((hash << 5) + hash) + static_cast<unsigned char>(data[i]);
            }
            return hash;
        }
    };
}

#endif // SDS_H