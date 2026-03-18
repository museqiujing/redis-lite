#include "string_type.h"
#include <chrono>


// 检查键是否过期
bool String::is_expired(const std::string& key) const {
    auto it = storage.find(key);
    if (it == storage.end()) {
        return true;  // 键不存在，视为过期
    }
    
    int64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    return it->second->expire_at != -1 && it->second->expire_at < now;
}

// 清理过期键
void String::clean_expired(const std::string& key) {
    if (is_expired(key)) {
        storage.erase(key);
    }
}

// 设置键值对
void String::set(const std::string& key, const std::string& value) {
    storage[key] = std::make_shared<Value>(value, -1);
}

// 设置键值对并添加过期时间（秒）
void String::setex(const std::string& key, int seconds, const std::string& value) {
    int64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    int64_t expire_at = now + seconds * 1000LL;
    storage[key] = std::make_shared<Value>(value, expire_at);
}

// 获取值
std::string String::get(const std::string& key) {
    clean_expired(key); 
    auto it = storage.find(key);
    if (it == storage.end()) {
        return "";
    }
    return it->second->data.to_string(); 
}

// 设置过期时间（秒）
bool String::expire(const std::string& key, int seconds) {
    clean_expired(key);
    auto it = storage.find(key);
    if (it == storage.end()) {
        return false;
    }
    
    int64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    it->second->expire_at = now + seconds * 1000LL;
    return true;
}

// 递增操作
long long String::incr(const std::string& key) {
    return incrby(key, 1);
}

// 递减操作
long long String::decr(const std::string& key) {
    return decrby(key, 1);
}

// 递增指定值
long long String::incrby(const std::string& key, long long increment) {
    clean_expired(key);
    auto it = storage.find(key);
    
    long long value = 0;
    if (it != storage.end()) {
        try {
            value = std::stoll(it->second->data.to_string());
        } catch (...) {
            throw std::runtime_error("value is not an integer or out of range");
        }
    }
    
    value += increment;
    storage[key] = std::make_shared<Value>(std::to_string(value), -1);
    return value;
}

// 递减指定值
long long String::decrby(const std::string& key, long long decrement) {
    return incrby(key, -decrement);
}

// 检查键是否存在
bool String::exists(const std::string& key) {
    clean_expired(key);
    return storage.find(key) != storage.end();
}

// 删除键
bool String::del(const std::string& key) {
    clean_expired(key);
    auto it = storage.find(key);
    if (it == storage.end()) {
        return false;
    }
    storage.erase(it);
    return true;
}

// 获取键的剩余过期时间（秒）
int String::ttl(const std::string& key) {
    clean_expired(key);
    auto it = storage.find(key);
    if (it == storage.end()) {
        return -2;  // 键不存在
    }
    
    if (it->second->expire_at == -1) {
        return -1;  // 永不过期
    }
    
    int64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    int64_t ttl_ms = it->second->expire_at - now;
    if (ttl_ms < 0) {
        return 0;  // 已过期
    }
    int ttl_seconds = static_cast<int>(ttl_ms / 1000);
    return ttl_seconds;
}

std::vector<std::pair<std::string, std::string>> String::get_all_data() {
    std::vector<std::pair<std::string, std::string>> result;
    for (const auto& pair : storage) {
        // 跳过过期的键
        if (!is_expired(pair.first)) {
            result.emplace_back(pair.first, pair.second->data.to_string());
        }
    }
    return result;
}
