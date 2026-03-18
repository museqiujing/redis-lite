#ifndef __SKIPLIST_H
#define __SKIPLIST_H 

#include <memory>
#include <vector>
#include <string>
#include <stdexcept>
#include <random>
#include <iostream>
#include <unordered_map>
#include "sds.h"

class SkipList {
public:
    SkipList();
    ~SkipList() = default;
    
    // 核心操作
    void insert(double score, const std::string& member); // 插入分数和成员
    bool remove(const std::string& member); // 删除成员
    double get_score(const std::string& member); // 获取成员的分数
    
    // 排名操作
    int get_rank(const std::string& member); // 获取成员的排名（从1开始）
    std::string get_by_rank(int rank); // 获取排名对应的成员
    
    // 基本统计
    int size() const { return count; }
    int get_level() const { return level; }
    
     // 获取所有数据
    std::vector<std::pair<double, std::string>> get_all_data();
    
    // 范围查询
    std::vector<std::pair<double, std::string>> range(double min_score, double max_score);
    std::vector<std::pair<double, std::string>> range(int start_rank, int end_rank);

private:
    // 跳表节点结构
    struct Node {
        double score;
        SDS member;
        struct Level {
            std::shared_ptr<Node> forward; // 指向下一个节点的指针
            unsigned long span; // 到下一个节点的距离
        };
        std::vector<Level> levels; // 节点在各层的指针和跨度
        int node_level;  // 节点当前有多少层数
        
        Node(int level, double score, const std::string& member);
    };
    //哈希反向索引
    std::unordered_map<std::string, std::shared_ptr<Node>> node_map;


    // 跳表属性
    std::shared_ptr<Node> header;
    int count;
    int level;
    
    // 随机数生成器
    std::mt19937_64 rng;
    std::uniform_real_distribution<double> dist;
    
    // 配置参数
    static constexpr int MAX_LEVEL = 16;
    static constexpr double P = 0.25;
    
    // 辅助函数
    int random_level();
    int compare(double score1, double score2) const; // 比较两个分数
    int compare(double score, const std::string& member, const Node* node) const; 
};

#endif