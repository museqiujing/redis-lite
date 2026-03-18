#ifndef ZSET_H
#define ZSET_H
#include "skiplist.h"
#include <unordered_map>


class ZSetManager {
private:
    std::unordered_map<std::string, std::unique_ptr<SkipList>> zsets;
    
public:
    ZSetManager() = default;
    ~ZSetManager() = default;
    
    // 获取或创建有序集合
    SkipList* get_zset(const std::string& key);
    
    // 删除有序集合
    bool delete_zset(const std::string& key);
    
    // 获取所有有序集合的键
    std::vector<std::string> get_all_keys();

    std::vector<std::pair<std::string, std::vector<std::pair<double, std::string>>>> get_all_zset_data();
    
};
#endif