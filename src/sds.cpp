#include "sds.h"
#include <cstdlib>
#include <cstdio>

// 空间预分配策略
#define SDS_MAX_PREALLOC (1024*1024)

// 构造函数：从C风格字符串初始化
SDS::SDS(const char* init) {
    size_t initlen = init ? strlen(init) : 0;
    allocate(initlen);
    if (initlen && init) {
        memcpy(buf, init, initlen);
        buf[len] = '\0';
    }
}

// 构造函数：从std::string初始化
SDS::SDS(const std::string& init) {
    size_t initlen = init.length();
    allocate(initlen);
    if (initlen) {
        memcpy(buf, init.c_str(), initlen);
        buf[len] = '\0';
    }
}

// 构造函数：从指定长度的字符串初始化
SDS::SDS(const char* init, size_t initlen) {
    allocate(initlen);
    if (initlen && init) {
        memcpy(buf, init, initlen);
        buf[len] = '\0';
    }
}

// 复制构造函数
SDS::SDS(const SDS& other) {
    allocate(other.len);
    if (other.len) {
        memcpy(buf, other.buf, other.len);
        buf[len] = '\0';
    }
}

// 赋值运算符
SDS& SDS::operator=(const SDS& other) {
    if (this != &other) {
        // 释放旧内存
        if (buf) {
            delete[] buf;
        }
        // 分配新内存
        len = other.len;
        free = other.free;
        buf = new char[len + free + 1];
        if (len > 0) {
            memcpy(buf, other.buf, len);
        }
        buf[len] = '\0';
    }
    return *this;
}

// 移动构造函数
SDS::SDS(SDS&& other) noexcept
    : len(other.len), free(other.free), buf(other.buf) {
    other.len = 0;
    other.free = 0;
    other.buf = nullptr;
}

// 移动赋值运算符
SDS& SDS::operator=(SDS&& other) noexcept {
    if (this != &other) {
        if (buf) {
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
SDS::~SDS() {
    if (buf) {
        delete[] buf;
    }
}

// 分配内存
void SDS::allocate(size_t size) {
    len = size;
    free = 0;
    if (size > 0) {
        buf = new char[size + 1];
    } else {
        buf = new char[1];
        buf[0] = '\0';
    }
}

// 重新分配内存
void SDS::reallocate(size_t new_cap) {
    if (new_cap <= len + free) {
        return; // 空间足够
    }
    
    // 空间预分配策略
    size_t newlen = len;
    size_t newcap;
    if (newlen < SDS_MAX_PREALLOC) {
        newcap = newlen * 2;
    } else {
        newcap = newlen + SDS_MAX_PREALLOC;
    }
    
    // 确保newcap至少为new_cap
    if (newcap < new_cap) {
        newcap = new_cap;
    }
    
    char* new_buf = new char[newcap + 1];
    if (len > 0) {
        memcpy(new_buf, buf, len);
    }
    new_buf[len] = '\0';
    
    if (buf) {
        delete[] buf;
    }
    
    buf = new_buf;
    free = newcap - len;
}

// 预留空间
void SDS::reserve(size_t needed) {
    size_t required = len + needed;
    if (required > len + free) {
        reallocate(required);
    }
}

// 追加C风格字符串
void SDS::append(const char* t) {
    size_t t_len = strlen(t);
    reserve(t_len);
    memcpy(buf + len, t, t_len);
    len += t_len;
    free -= t_len;
    buf[len] = '\0';
}

// 追加std::string
void SDS::append(const std::string& t) {
    append(t.c_str());
}

// 追加SDS
void SDS::append(const SDS& t) {
    size_t t_len = t.len;
    reserve(t_len);
    memcpy(buf + len, t.buf, t_len);
    len += t_len;
    free -= t_len;
    buf[len] = '\0';
}

// 截断字符串
void SDS::truncate(size_t new_len) {
    if (new_len >= len) {
        return; // 不需要截断
    }
    
    len = new_len;
    free += len - new_len;
    buf[len] = '\0';
}

// 比较两个SDS
int SDS::compare(const SDS& other) const {
    size_t minlen = len < other.len ? len : other.len;
    int cmp = memcmp(buf, other.buf, minlen);
    if (cmp == 0) {
        return len - other.len;
    }
    return cmp;
}

// 比较运算符
bool SDS::operator==(const SDS& other) const {
    return compare(other) == 0;
}

bool SDS::operator!=(const SDS& other) const {
    return compare(other) != 0;
}

bool SDS::operator<(const SDS& other) const {
    return compare(other) < 0;
}

bool SDS::operator<=(const SDS& other) const {
    return compare(other) <= 0;
}

bool SDS::operator>(const SDS& other) const {
    return compare(other) > 0;
}

bool SDS::operator>=(const SDS& other) const {
    return compare(other) >= 0;
}

// 转换为std::string
std::string SDS::to_string() const {
    return std::string(buf, len);
}