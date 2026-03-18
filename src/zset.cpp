#include "zset.h"

// ZSetManager类的实现
SkipList* ZSetManager::get_zset(const std::string& key) {
    auto it = zsets.find(key);
    if (it == zsets.end()) {
        // 创建新的有序集合
        zsets[key] = std::make_unique<SkipList>();
    }
    return zsets[key].get();
}
 
bool ZSetManager::delete_zset(const std::string& key) {
    return zsets.erase(key) > 0;
}
 
std::vector<std::string> ZSetManager::get_all_keys() {
    std::vector<std::string> keys;
    for (const auto& pair : zsets) {
        keys.push_back(pair.first);
    }
    return keys;
}
 
std::vector<std::pair<std::string, std::vector<std::pair<double, std::string>>>> ZSetManager::get_all_zset_data() {
    std::vector<std::pair<std::string, std::vector<std::pair<double, std::string>>>> result;
    for (const auto& pair : zsets) {
        const std::string& key = pair.first;
        const auto& zset = pair.second;
        result.emplace_back(key, zset->get_all_data());
    }
    return result;
}