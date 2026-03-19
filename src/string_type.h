#ifndef STRING_H
#define STRING_H

#include <unordered_map>
#include <string>
#include <memory>
#include "sds.h"
#include <vector>

class String {
private:
    struct Value {
        SDS data;           // 存储字符串数据
        int64_t expire_at;  // 过期时间戳（毫秒），-1表示永不过期
        
        Value(const std::string& str, int64_t expire = -1)
             //: data(str.c_str()), expire_at(expire) {}// 构造函数，将字符串转换为SDS
             : data(str), expire_at(expire) {} 
    };
    
    std::unordered_map<std::string, std::shared_ptr<Value>> storage;  // 键值存储
    
    // 检查键是否过期
    bool is_expired(const std::string& key) const;
    
    // 清理过期键
    void clean_expired(const std::string& key);

public:
    String() = default;
    ~String() = default;
    
    // 设置键值对
    void set(const std::string& key, const std::string& value);
    
    // 设置键值对并添加过期时间（秒）
    void setex(const std::string& key, int seconds, const std::string& value);
    
    // 获取值
    std::string get(const std::string& key);
    
    // 设置过期时间（秒）
    bool expire(const std::string& key, int seconds);
    
    // 递增操作
    long long incr(const std::string& key);
    
    // 递减操作
    long long decr(const std::string& key);
    
    // 递增指定值
    long long incrby(const std::string& key, long long increment);
    
    // 递减指定值
    long long decrby(const std::string& key, long long decrement);
    
    // 检查键是否存在
    bool exists(const std::string& key);
    
    // 删除键
    bool del(const std::string& key);
    
    // 获取键的剩余过期时间（秒）
    int ttl(const std::string& key);

    // 获取所有String数据
    std::vector<std::pair<std::string, std::string>> get_all_data();
};

#endif // STRING_H