#ifndef ZSET_H
#define ZSET_H
#include "skiplist.h"
#include <unordered_map>
#include "sds.h"

class ZSetManager
{
private:
    std::unordered_map<SDS, std::unique_ptr<SkipList>> zsets;

public:
    ZSetManager() = default;
    ~ZSetManager() = default;

    // 获取或创建有序集合
    SkipList *get_zset(const SDS &key);

    // 删除有序集合
    bool delete_zset(const SDS &key);

    // 获取所有有序集合的键
    std::vector<SDS> get_all_keys();

    std::vector<std::pair<SDS, std::vector<std::pair<double, SDS>>>> get_all_zset_data();
};
#endif