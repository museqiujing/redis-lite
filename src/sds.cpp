#include "sds.h"
#include <cstdlib>
#include <cstdio>

// 空间预分配策略
#define SDS_MAX_PREALLOC (1024 * 1024)

// 构造函数：从C风格字符串初始化
SDS::SDS(const char *init)
{
    size_t initlen = init ? strlen(init) : 0;
    allocate(initlen);
    if (initlen && init)
    {
        memcpy(buf, init, initlen);
        buf[len] = '\0';
    }
}

// 构造函数：从std::string初始化
SDS::SDS(const std::string &init)
{
    size_t initlen = init.length();
    allocate(initlen);
    if (initlen)
    {
        memcpy(buf, init.c_str(), initlen);
        buf[len] = '\0';
    }
}

// 构造函数：从指定长度的字符串初始化
SDS::SDS(const char *init, size_t initlen)
{
    allocate(initlen);
    if (initlen && init)
    {
        memcpy(buf, init, initlen);
        buf[len] = '\0';
    }
}

// 复制构造函数
SDS::SDS(const SDS &other)
{
    allocate(other.len);
    if (other.len)
    {
        memcpy(buf, other.buf, other.len);
        buf[len] = '\0';
    }
}

// 赋值运算符
SDS &SDS::operator=(const SDS &other)
{
    if (this != &other)
    {
        // 只有当现有空间不足时才重新分配
        if (other.len > len + free)
        {
            // 需要扩容
            if (buf)
                delete[] buf;
            len = other.len;
            free = 0;
            buf = new char[len + 1];
        }
        else
        {
            // 现有空间足够，直接复用
            free = (len + free) - other.len;
            len = other.len;
        }

        // 拷贝数据
        if (other.len > 0)
        {
            memcpy(buf, other.buf, other.len);
        }
        buf[len] = '\0';
    }
    return *this;
}

// 移动构造函数
SDS::SDS(SDS &&other) noexcept
    : len(other.len), free(other.free), buf(other.buf)
{
    other.len = 0;
    other.free = 0;
    other.buf = nullptr;
}

// 移动赋值运算符
SDS &SDS::operator=(SDS &&other) noexcept
{
    if (this != &other)
    {
        if (buf)
        {
            delete[] buf;
        }
        len = other.len;
        free = other.free;
        buf = other.buf;
        other.len = 0;
        other.free = 0;
        other.buf = nullptr;
    }
    return *this;
}

// 析构函数
SDS::~SDS()
{
    if (buf)
    {
        delete[] buf;
    }
}

// 分配内存
void SDS::allocate(size_t size)
{
    len = size;
    free = 0;
    if (size > 0)
    {
        buf = new char[size + 1];
        buf[0] = '\0'; // 确保空字符串有效
    }
    else
    {
        buf = new char[1];
        buf[0] = '\0';
    }
}

// 重新分配内存
void SDS::reallocate(size_t new_cap)
{
    if (new_cap <= len + free)
        return;

    // 智能容量计算
    size_t newcap = new_cap;
    if (newcap < len * 2)
        newcap = len * 2;
    if (newcap < 1024 && newcap < len * 4)
        newcap = len * 4;

    // 优先使用realloc
    char *new_buf = (char *)realloc(buf, newcap + 1);
    if (new_buf)
    {
        buf = new_buf;
        free = newcap - len;
        buf[len] = '\0';
        return;
    }

    // realloc失败，使用安全方法
    char *temp_buf = new char[newcap + 1];
    if (len > 0)
    {
        memcpy(temp_buf, buf, len);
        temp_buf[len] = '\0';
    }
    if (buf)
        delete[] buf;
    buf = temp_buf;
    free = newcap - len;
}

// 预留空间
void SDS::reserve(size_t needed)
{
    size_t required = len + needed;
    if (required > len + free)
    {
        reallocate(required);
    }
}

void SDS::swap(SDS &other) noexcept
{
    std::swap(buf, other.buf);
    std::swap(len, other.len);
    std::swap(free, other.free);
}

void SDS::erase(size_t pos, size_t count)
{
    if (pos >= len)
    {
        return;
    }

    size_t actual_count = count;
    if (count == std::string::npos || pos + count > len)
    {
        actual_count = len - pos;
    }

    if (actual_count == 0)
    {
        return;
    }

    // 把后半段数据往前搬，包含结尾 '\0'
    memmove(buf + pos, buf + pos + actual_count, len - pos - actual_count + 1);

    len -= actual_count;
    free += actual_count;
}

void SDS::erase_prefix(size_t count)
{
    erase(0, count);
}

// 追加C风格字符串
void SDS::append(const char *t)
{
    size_t t_len = strlen(t);
    reserve(t_len);
    memcpy(buf + len, t, t_len);
    len += t_len;
    free -= t_len;
    buf[len] = '\0';
}

// 追加std::string
void SDS::append(const std::string &t)
{
    append(t.c_str());
}

// 追加SDS
void SDS::append(const SDS &t)
{
    size_t t_len = t.len;
    reserve(t_len);
    memcpy(buf + len, t.buf, t_len);
    len += t_len;
    free -= t_len;
    buf[len] = '\0';
}

void SDS::append(const char *data, size_t length)
{
    if (!data || length == 0)
        return;

    reserve(length);
    memcpy(buf + len, data, length);
    len += length;
    free -= length;
    buf[len] = '\0';
}

// 截断字符串
void SDS::truncate(size_t new_len)
{
    if (new_len >= len)
    {
        return;
    }

    size_t old_len = len;
    len = new_len;
    free += old_len - new_len;
    buf[len] = '\0';
}

// 比较两个SDS
int SDS::compare(const SDS &other) const
{
    size_t minlen = len < other.len ? len : other.len;
    int cmp = memcmp(buf, other.buf, minlen);
    if (cmp == 0)
    {
        return len - other.len;
    }
    return cmp;
}

// 比较运算符
bool SDS::operator==(const SDS &other) const
{
    return compare(other) == 0;
}

bool SDS::operator!=(const SDS &other) const
{
    return compare(other) != 0;
}

bool SDS::operator<(const SDS &other) const
{
    return compare(other) < 0;
}

bool SDS::operator<=(const SDS &other) const
{
    return compare(other) <= 0;
}

bool SDS::operator>(const SDS &other) const
{
    return compare(other) > 0;
}

bool SDS::operator>=(const SDS &other) const
{
    return compare(other) >= 0;
}

// 转换为std::string
std::string SDS::to_string() const
{
    return std::string(buf, len);
}

// 清空字符串
void SDS::clear()
{
    size_t old_len = len;
    len = 0;
    if (buf)
    {
        buf[0] = '\0';
    }
    free += old_len;
}

// 查找C风格字符串
size_t SDS::find(const char *substr, size_t pos) const
{
    if (!substr || pos >= len)
        return std::string::npos;

    const char *result = strstr(buf + pos, substr);
    if (result)
    {
        return result - buf;
    }
    return std::string::npos;
}

// 查找std::string
size_t SDS::find(const std::string &substr, size_t pos) const
{
    return find(substr.c_str(), pos);
}

// 查找SDS
size_t SDS::find(const SDS &substr, size_t pos) const
{
    return find(substr.c_str(), pos);
}

// += 运算符重载（C风格字符串）
SDS &SDS::operator+=(const char *str)
{
    append(str);
    return *this;
}

// += 运算符重载（std::string）
SDS &SDS::operator+=(const std::string &str)
{
    append(str);
    return *this;
}

// += 运算符重载（SDS）
SDS &SDS::operator+=(const SDS &other)
{
    append(other);
    return *this;
}

// 子字符串
SDS SDS::substr(size_t pos, size_t sublen) const
{
    if (pos >= len)
    {
        return SDS(); // 返回空字符串
    }

    size_t actual_len = sublen;
    if (sublen == std::string::npos || pos + sublen > len)
    {
        actual_len = len - pos;
    }

    return SDS(buf + pos, actual_len);
}

// 下标运算符重载
char &SDS::operator[](size_t index)
{
    if (index >= len)
    {
        throw std::out_of_range("SDS index out of range");
    }
    return buf[index];
}

// 下标运算符重载
const char &SDS::operator[](size_t index) const
{
    if (index >= len)
    {
        throw std::out_of_range("SDS index out of range");
    }
    return buf[index];
}

// 类型转换为std::string
SDS::operator std::string() const
{
    return std::string(buf, len);
}

// 类型转换为const char*
SDS::operator const char *() const
{
    return buf;
}

// 赋值运算符重载（C风格字符串）
SDS &SDS::operator=(const char *str)
{
    if (!str)
    {
        clear();
        return *this;
    }

    size_t str_len = strlen(str);
    if (str_len > len + free)
    {
        reallocate(str_len);
    }

    len = str_len;
    free = (len + free) - len;
    memcpy(buf, str, len);
    buf[len] = '\0';
    return *this;
}

// 赋值运算符重载（std::string）
SDS &SDS::operator=(const std::string &str)
{
    return operator=(str.c_str());
}

// 迭代器支持
SDS::iterator SDS::begin()
{
    return buf;
}

SDS::iterator SDS::end()
{
    return buf + len;
}

SDS::const_iterator SDS::begin() const
{
    return buf;
}

SDS::const_iterator SDS::end() const
{
    return buf + len;
}

bool SDS::operator==(const char *str) const
{
    if (!str)
    {
        return len == 0;
    }
    size_t str_len = strlen(str);
    if (len != str_len)
    {
        return false;
    }
    return memcmp(buf, str, len) == 0;
}

bool SDS::operator==(const std::string &str) const
{
    if (len != str.length())
    {
        return false;
    }
    return memcmp(buf, str.c_str(), len) == 0;
}

bool SDS::operator!=(const char *str) const
{
    return !(*this == str);
}

bool SDS::operator!=(const std::string &str) const
{
    return !(*this == str);
}

// 全局+运算符重载
SDS operator+(const SDS &lhs, const SDS &rhs)
{
    SDS result = lhs;
    result += rhs;
    return result;
}

SDS operator+(const SDS &lhs, const char *rhs)
{
    SDS result = lhs;
    result += rhs;
    return result;
}

SDS operator+(const char *lhs, const SDS &rhs)
{
    SDS result = lhs;
    result += rhs;
    return result;
}

SDS operator+(const SDS &lhs, const std::string &rhs)
{
    SDS result = lhs;
    result += rhs;
    return result;
}

SDS operator+(const std::string &lhs, const SDS &rhs)
{
    SDS result = lhs;
    result += rhs;
    return result;
}