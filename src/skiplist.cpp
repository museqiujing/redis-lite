#include "skiplist.h"
#include <algorithm>
#include <chrono>

// 节点构造函数
SkipList::Node::Node(int level, double score, const std::string& member)
    : score(score), member(member), node_level(level), levels(level + 1)
{
    for (auto& l : levels) {
        l.forward = nullptr;
        l.span = 0;
    }
}

// 跳表构造函数
SkipList::SkipList()
    : count(0), level(0),
      rng(std::chrono::steady_clock::now().time_since_epoch().count()),
      dist(0.0, 1.0)
{
    // 头节点，层数为MAX_LEVEL
    header = std::make_shared<Node>(MAX_LEVEL, 0.0, "");
}

// 随机生成层数（Redis的思想）
int SkipList::random_level() {
    int level = 0; 
    while (dist(rng) < P ) {
        level++;
    }
    return std::min(level, MAX_LEVEL - 1);
}

// 分数比较函数
int SkipList::compare(double score1, double score2) const {
    if (score1 < score2) return -1;
    if (score1 > score2) return 1;
    return 0;
}

// 分数和成员比较函数（处理分数相同的情况）
int SkipList::compare(double score, const std::string& member, const Node* node) const {
    int score_cmp = compare(score, node->score);
    if (score_cmp != 0) return score_cmp;
    SDS member_sds(member); 
    return member_sds.compare(node->member); 
}

// 插入操作
void SkipList::insert(double score, const std::string& member) {
       // 先检查成员是否已存在
    if (member.empty()) {
        throw std::invalid_argument("Member cannot be empty");
    }
    
   /* auto temp = header->levels[0].forward;  
    bool member_exist = false;
    while (temp) {
        if (temp->member == member) {
            member_exist = true;
            break;
        }
        temp = temp->levels[0].forward;
    }
        // 如果成员已存在，先删除再重新插入
        if (member_exist) {
            remove(member);
        }
    */

    // 检查成员是否已存在
    auto it = node_map.find(member);
        if (it != node_map.end()) {
        // 成员已存在，先删除再重新插入
        remove(member);
        }

    std::vector<std::shared_ptr<Node>> update(MAX_LEVEL + 1); 
    std::vector<unsigned long> rank(MAX_LEVEL + 1); 
    auto current = header; 
   
    // 从最高层开始查找
    for (int i = level; i >= 0; i--) {
        rank[i] = (i == level) ? 0 : rank[i + 1];
        while (current->levels[i].forward) {
            int cmp = compare(score, member, current->levels[i].forward.get());
            if (cmp < 0) {
                break;
            }
            rank[i] += current->levels[i].span;
            current = current->levels[i].forward;
        }
        update[i] = current;
    }
    
    // 生成随机层数
    int new_level = random_level();
    
    // 如果新层数大于当前最大层数，更新update数组
    if (new_level > level) {
        for (int i = level + 1; i <= new_level; i++) {
            rank[i] = 0;
            update[i] = header;
            update[i]->levels[i].span = count;
        }
        level = new_level;
    }
    
    // 创建新节点
    auto new_node = std::make_shared<Node>(new_level, score, member);
    
    // 更新指针和跨度
    for (int i = 0; i <= new_level; i++) {
        new_node->levels[i].forward = update[i]->levels[i].forward;
        update[i]->levels[i].forward = new_node;
        
        // 更新跨度
        new_node->levels[i].span = update[i]->levels[i].span - (rank[0] - rank[i]);
        update[i]->levels[i].span = (rank[0] - rank[i]) + 1;
    }
    
    // 更新更高层的跨度
    for (int i = new_level + 1; i <= level; i++) {
        update[i]->levels[i].span++;
    }
    
    count++;
    // 插入到哈希表
    node_map[member] = new_node;
}

// 删除操作
bool SkipList::remove(const std::string& member) {
    if (member.empty()) {
        throw std::invalid_argument("Member cannot be empty");
    }
    
    // 从哈希表获取节点
    auto it = node_map.find(member);
    if (it == node_map.end()) {
        return false;
    }
    double score = it->second->score;
    
    std::vector<std::shared_ptr<Node>> update(MAX_LEVEL + 1);
    auto current = header;
    bool found = false;
    
    // 从最高层开始查找
    for (int i = level; i >= 0; i--) {
        while (current->levels[i].forward) {
            int cmp = compare(score, member, current->levels[i].forward.get());
            if (cmp < 0) {
                break;
            } else if (cmp == 0) {
                // 找到目标节点
                update[i] = current;
                found = true;
                break;
            }
            current = current->levels[i].forward;
        }
        if (!found) {
            update[i] = current;
        }
    }
    
    // 找到目标节点
    auto target_node = update[0]->levels[0].forward;
   /* while (target_node) {
        if (target_node->member == member) {
            found = true;
            break;
        }
        target_node = target_node->levels[0].forward;
    }*/



    // 如果成员不存在
    if (!found || !target_node||target_node->member.to_string()!=member) {
        return false;
    }
  
    // 更新指针和跨度
    for (int i = 0; i <= level; i++) {
        if (update[i]->levels[i].forward != target_node) {
            break;
        }
        update[i]->levels[i].span += target_node->levels[i].span - 1;
        update[i]->levels[i].forward = target_node->levels[i].forward;
    }
    
    // 更新更高层的跨度
    for (int i = target_node->node_level + 1; i <= level; i++) {
        update[i]->levels[i].span--;
    }
    
    // 更新跳表最大层数
    while (level > 0 && !header->levels[level].forward) {
        level--;
    }
    
    // 从哈希表中删除
    node_map.erase(member);
    
    count--;
    return true;
}

// 获取成员分数
double SkipList::get_score(const std::string& member) {
    if (member.empty()) {
        throw std::invalid_argument("Member cannot be empty");
    }
    
    // 从哈希表获取节点，时间复杂度O(1)
    auto it = node_map.find(member);
    if (it != node_map.end()) {
        return it->second->score;
    }
    
    throw std::runtime_error("Member not found");
}

// 获取成员排名（从1开始）
int SkipList::get_rank(const std::string& member) {
    if (member.empty()) {
        throw std::invalid_argument("Member cannot be empty");
    }
    
    // 从哈希表获取节点，时间复杂度O(1)
    auto it = node_map.find(member);
    if (it == node_map.end()) {
        return 0; // 未找到
    }
    
    auto node = it->second;
    double score = node->score;
    std::string member_name = node->member.to_string();
    
    // 使用分数和成员名称计算排名
    unsigned long rank = 0;
    auto current = header;
    
    for (int i = level; i >= 0; i--) {
        while (current->levels[i].forward) {
            int cmp = compare(score, member_name, current->levels[i].forward.get());
            if (cmp < 0) {
                break;
            } else if (cmp == 0) {
                rank+=current->levels[i].span;
                return rank;
            }
            rank += current->levels[i].span;
            current = current->levels[i].forward;
        }
    }
    
    return 0; // 未找到
}

// 根据排名获取成员
std::string SkipList::get_by_rank(int rank) {
    if (rank <= 0 || rank > count) {
        return "";
    }
    
    auto current = header;
    unsigned long traversed = 0;
    
    for (int i = level; i >= 0; i--) {
        while (current->levels[i].forward && traversed + current->levels[i].span < rank) {
            traversed += current->levels[i].span;
            current = current->levels[i].forward;
        }
    }
    
    current = current->levels[0].forward;
    return current->member.to_string();
}

// 获取所有数据
std::vector<std::pair<double, std::string>> SkipList::get_all_data() {
    std::vector<std::pair<double, std::string>> result;
    auto current = header->levels[0].forward;
    while (current) {
        result.emplace_back(current->score, current->member.to_string());
        current = current->levels[0].forward;
    }
    return result;
}

// 根据分数范围查询
std::vector<std::pair<double, std::string>> SkipList::range(double min_score, double max_score) { 
    std::vector<std::pair<double, std::string>> result;
    
    auto current = header;
    
    // 找到起始位置
    for (int i = level; i >= 0; i--) {
        while (current->levels[i].forward && current->levels[i].forward->score < min_score) {
            current = current->levels[i].forward;
        }
    }
    
    // 开始收集结果
    current = current->levels[0].forward;
    while (current && current->score <= max_score) {
        result.emplace_back(current->score, current->member.to_string());
        current = current->levels[0].forward;
    }

    if (result.empty()) {
       std::cout << "没有符合条件的成员" << std::endl;
    }

    return result;
}

// 根据排名范围查询
std::vector<std::pair<double, std::string>> SkipList::range(int start, int stop) {
    std::vector<std::pair<double, std::string>> result;
    // 验证排名范围是否有效
     // 处理负索引
     // 处理负索引
    if (start < 0) {
        start = count + start;
    }
    if (stop < 0) {
        stop = count + stop;
    }
    
    // 验证范围
    if (start < 0 || start >= count || stop < start) {
        return result;
    }
    
    // 确保结束索引不超过总元素数
    if (stop >= count) {
        stop = count - 1;
    }

    // 转换为1-based排名
    int start_rank = start + 1;
    int end_rank = stop + 1;

    auto current = header;
    unsigned long traversed = 0;
    
    // 找到起始排名位置
    for (int i = level; i >= 0; i--) {
        while (current->levels[i].forward && traversed + current->levels[i].span < start_rank) {
            traversed += current->levels[i].span;
            current = current->levels[i].forward;
        }
    }
    
    current = current->levels[0].forward;
    
    // 收集结果
    for (int i = start_rank; i <= end_rank && current; i++) {
        result.emplace_back(current->score, current->member.to_string());
        current = current->levels[0].forward;
    }
    
    return result;
}