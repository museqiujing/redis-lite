#include "list.h"
#include "sds.h"
#include <chrono>

// 检查键是否过期
bool List::is_expired(const std::string& key) const {
    auto it = storage.find(key);
    if (it == storage.end()) {
        return true;  // 键不存在，视为过期
    }
    
    int64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    return it->second->expire_at != -1 && it->second->expire_at < now;
}

// 清理过期键
void List::clean_expired(const std::string& key) {
    if (is_expired(key)) {
        storage.erase(key);
    }
}

// 在链表头部插入元素
void List::lpush(const std::string& key, const std::string& value) {
    clean_expired(key);
    if (storage.find(key) == storage.end()) {
        storage[key] = std::make_shared<Value>();
    }
    storage[key]->data.push_front(SDS(value.c_str()));
}

// 在链表尾部插入元素
void List::rpush(const std::string& key, const std::string& value) {
    clean_expired(key);
    if (storage.find(key) == storage.end()) {
        storage[key] = std::make_shared<Value>();
    }
    storage[key]->data.push_back(SDS(value.c_str()));
}

// 移除并返回链表头部元素
std::string List::lpop(const std::string& key) {
    clean_expired(key);
    auto it = storage.find(key);
    if (it == storage.end() || it->second->data.empty()) {
        return "";
    }
    std::string value = it->second->data.front().to_string();
    it->second->data.pop_front();
    if (it->second->data.empty()) {
        storage.erase(it);
    }
    return value;
}

// 移除并返回链表尾部元素
std::string List::rpop(const std::string& key) {
    clean_expired(key);
    auto it = storage.find(key);
    if (it == storage.end() || it->second->data.empty()) {
        return "";
    }
    std::string value = it->second->data.back().to_string();
    it->second->data.pop_back();
    if (it->second->data.empty()) {
        storage.erase(it);
    }
    return value;
}

// 返回链表指定范围的元素
std::vector<std::string> List::lrange(const std::string& key, int start, int stop) {
    clean_expired(key);
    auto it = storage.find(key);
    if (it == storage.end() || it->second->data.empty()) {
        return {};
    }
    
    std::vector<std::string> result;
    int len = it->second->data.size();
    
    // 处理负索引
    if (start < 0) {
        start += len;
        if (start < 0) start = 0;
    }
    if (stop < 0) {
        stop += len;
        if (stop < 0) return {};
    }
    
    // 超出范围
    if (start >= len) return {};
    if (stop >= len) stop = len - 1;
    if (start > stop) return {};
    
    int count = 0;
    for (const auto& element : it->second->data) {
        if (count >= start && count <= stop) {
            result.push_back(element.to_string());
        }
        if (count > stop) break;
        count++;
    }
    
    return result;
}

// 返回链表长度
long long List::llen(const std::string& key) {
    clean_expired(key);
    auto it = storage.find(key);
    if (it == storage.end()) {
        return 0;
    }
    return it->second->data.size();
}

// 设置过期时间（秒）
bool List::expire(const std::string& key, int seconds) {
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

// 检查键是否存在
bool List::exists(const std::string& key) {
    clean_expired(key);
    return storage.find(key) != storage.end();
}

// 删除键
bool List::del(const std::string& key) {
    clean_expired(key);
    auto it = storage.find(key);
    if (it == storage.end()) {
        return false;
    }
    storage.erase(it);
    return true;
}

// 获取键的剩余过期时间（秒）
int List::ttl(const std::string& key) {
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