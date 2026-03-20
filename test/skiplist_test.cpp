#include "../src/skiplist.h"
#include <iostream>
#include <vector>
#include <chrono>
/*
// 跳表功能测试
void test_skiplist() {
    std::cout << "=== 跳表功能测试 ===" << std::endl;

    SkipList sl;

    // 测试基本操作
    std::cout << "1. 测试插入操作" << std::endl;
    sl.insert(100.5, "张三");
    sl.insert(95.0, "李四");
    sl.insert(105.5, "王五");
    sl.insert(90.0, "赵六");
    sl.insert(110.0, "钱七");
    std::cout << "插入完成，当前大小: " << sl.size() << std::endl;
    std::cout << "当前最大层数: " << sl.get_level() << std::endl;

    std::cout << "\n2. 测试获取分数操作" << std::endl;
    try {
        double score = sl.get_score("张三");
        std::cout << "张三的分数: " << score << std::endl;

        score = sl.get_score("李四");
        std::cout << "李四的分数: " << score << std::endl;

        score = sl.get_score("王五");
        std::cout << "王五的分数: " << score << std::endl;

        score = sl.get_score("赵六");
        std::cout << "赵六的分数: " << score << std::endl;

        score = sl.get_score("钱七");
        std::cout << "钱七的分数: " << score << std::endl;
    } catch (const std::runtime_error& e) {
        std::cout << "错误: " << e.what() << std::endl;
    }

    std::cout << "\n3. 测试排名操作" << std::endl;
    int rank = sl.get_rank("张三");
    std::cout << "张三的排名: " << rank << std::endl;

    rank = sl.get_rank("李四");
    std::cout << "李四的排名: " << rank << std::endl;

    rank = sl.get_rank("王五");
    std::cout << "王五的排名: " << rank << std::endl;

    rank = sl.get_rank("赵六");
    std::cout << "赵六的排名: " << rank << std::endl;

    rank = sl.get_rank("钱七");
    std::cout << "钱七的排名: " << rank << std::endl;

    std::cout << "\n4. 测试按排名查找" << std::endl;
    std::string member = sl.get_by_rank(1);
    if (!member.empty()) std::cout << "排名第1的玩家: " << member << std::endl;
    else std::cout << "未找到排名第1的玩家" << std::endl;

    member = sl.get_by_rank(3);
    if (!member.empty()) std::cout << "排名第3的玩家: " << member << std::endl;
    else std::cout << "未找到排名第3的玩家" << std::endl;

    member = sl.get_by_rank(5);
    if (!member.empty()) std::cout << "排名第5的玩家: " << member << std::endl;
    else std::cout << "未找到排名第5的玩家" << std::endl;

    std::cout << "\n5. 测试删除操作" << std::endl;
    bool removed = sl.remove("李四");
    std::cout << "删除李四: " << (removed ? "成功" : "失败") << std::endl;
    std::cout << "删除后大小: " << sl.size() << std::endl;

    // 测试删除后的排名
    std::cout << "删除后的排名：" << std::endl;
    rank = sl.get_rank("张三");
    std::cout << "张三的排名: " << rank << std::endl;

    rank = sl.get_rank("王五");
    std::cout << "王五的排名: " << rank << std::endl;

    member = sl.get_by_rank(2);
    if (!member.empty()) std::cout << "排名第2的玩家: " << member << std::endl;
    else std::cout << "未找到排名第2的玩家" << std::endl;

    std::cout << "\n6. 测试更新操作" << std::endl;
    sl.insert(115.0, "张三"); // 更新张三的分数
    try {
        double score = sl.get_score("张三");
        std::cout << "更新后张三的分数: " << score << std::endl;
    } catch (const std::runtime_error& e) {
        std::cout << "错误: " << e.what() << std::endl;
    }

    // 测试更新后的排名
    rank = sl.get_rank("张三");
    std::cout << "更新后张三的排名: " << rank << std::endl;

    std::cout << "\n7. 测试分数范围查询" << std::endl;
    auto range_result = sl.range(95.0, 110.0);
    std::cout << "分数范围查询结果 (95-110): " << std::endl;
    for (const auto& pair : range_result) {
        std::cout << "  " << pair.second << " => " << pair.first << std::endl;
    }

    std::cout << "\n8. 测试排名范围查询" << std::endl;
    auto rank_range_result = sl.range(1, 3);
    std::cout << "排名范围查询结果 (1-3): " << std::endl;
    for (const auto& pair : rank_range_result) {
        std::cout << "  排名 " << sl.get_rank(pair.second) << ": " << pair.second << " (" << pair.first << ")" << std::endl;
    }

    std::cout << "\n9. 测试前10名查询" << std::endl;
    auto top10_result = sl.range(1, 10);
    std::cout << "前10名玩家: " << std::endl;
    for (size_t i = 0; i < top10_result.size(); i++) {
        std::cout << "  " << (i + 1) << ". " << top10_result[i].second << " (" << top10_result[i].first << ")" << std::endl;
    }

    std::cout <<"测试排名范围为1,1"<<std::endl;
    rank_range_result = sl.range(1, 1);
    for (const auto& pair : rank_range_result) {
        std::cout << "  排名 " << sl.get_rank(pair.second) << ": " << pair.second << " (" << pair.first << ")" << std::endl;
    }

     std::cout <<"测试排名范围为2,5"<<std::endl;
     rank_range_result = sl.range(2, 5);
    for (const auto& pair : rank_range_result) {
        std::cout << "  排名 " << sl.get_rank(pair.second) << ": " << pair.second << " (" << pair.first << ")" << std::endl;
    }

     std::cout <<"测试排名范围为1,3"<<std::endl;
     rank_range_result = sl.range(1, 3);
    for (const auto& pair : rank_range_result) {
        std::cout << "  排名 " << sl.get_rank(pair.second) << ": " << pair.second << " (" << pair.first << ")" << std::endl;
    }

    std::cout<<"根据分数排名查询"<<std::endl;

    std::cout << "分数范围查询结果 (95-110): " << std::endl;
    rank_range_result = sl.range(95.0, 110.0);
    for (const auto& pair : rank_range_result) {
        std::cout << "  排名 " << sl.get_rank(pair.second) << ": " << pair.second << " (" << pair.first << ")" << std::endl;
    }

    std::cout<<"测试异常分数范围为查询结果(94-120)"<<std::endl;
    rank_range_result = sl.range(94.0, 120.0);
    for (const auto& pair : rank_range_result) {
        std::cout << "  排名 " << sl.get_rank(pair.second) << ": " << pair.second << " (" << pair.first << ")" << std::endl;
    }

    std::cout << "异常分数范围查询结果 (0-94): " << std::endl;
    rank_range_result = sl.range(0.0, 94.0);
    for (const auto& pair : rank_range_result) {
        std::cout << "  排名 " << sl.get_rank(pair.second) << ": " << pair.second << " (" << pair.first << ")" << std::endl;
    }

    std::cout << "异常分数范围查询结果 (120-150): " << std::endl;
    rank_range_result = sl.range(120.0, 150.0);
    for (const auto& pair : rank_range_result) {
        std::cout << "  排名 " << sl.get_rank(pair.second) << ": " << pair.second << " (" << pair.first << ")" << std::endl;
    }

    std::cout << "\n=== 测试完成 ===" << std::endl;
}

// 大数据测试用例
void test_skiplist_large_data() {
    std::cout << "\n=== 大数据测试 ===" << std::endl;

    SkipList sl;
    const int TEST_SIZE = 10000; // 测试数据量

    std::cout << "1. 测试插入 " << TEST_SIZE << " 个元素" << std::endl;
    auto start_time = std::chrono::high_resolution_clock::now();

    // 插入大量数据
    for (int i = 0; i < TEST_SIZE; i++) {
        double score = rand() % 10000; // 随机分数
        std::string member = "player_" + std::to_string(i);
        sl.insert(score, member);
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    std::cout << "插入完成，当前大小: " << sl.size() << std::endl;
    std::cout << "当前最大层数: " << sl.get_level() << std::endl;
    std::cout << "插入耗时: " << duration.count() << " 毫秒" << std::endl;

    std::cout << "\n2. 测试查询性能" << std::endl;
    start_time = std::chrono::high_resolution_clock::now();

    // 随机查询1000个成员
    int found_count = 0;
    for (int i = 0; i < 1000; i++) {
        int index = rand() % TEST_SIZE;
        std::string member = "player_" + std::to_string(index);
        try {
            double score = sl.get_score(member);
            found_count++;
        } catch (const std::runtime_error& e) {
            // 忽略未找到的情况
        }
    }

    end_time = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    std::cout << "查询完成，找到 " << found_count << " 个成员" << std::endl;
    std::cout << "查询耗时: " << duration.count() << " 毫秒" << std::endl;

    std::cout << "\n3. 测试排名计算性能" << std::endl;
    start_time = std::chrono::high_resolution_clock::now();

    // 随机计算1000个成员的排名
    int rank_count = 0;
    for (int i = 0; i < 1000; i++) {
        int index = rand() % TEST_SIZE;
        std::string member = "player_" + std::to_string(index);
        int rank = sl.get_rank(member);
        if (rank > 0) {
            rank_count++;
        }
    }

    end_time = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    std::cout << "排名计算完成，成功计算 " << rank_count << " 个成员的排名" << std::endl;
    std::cout << "排名计算耗时: " << duration.count() << " 毫秒" << std::endl;

    std::cout << "\n4. 测试删除操作" << std::endl;
    start_time = std::chrono::high_resolution_clock::now();

    // 删除1000个成员
    int remove_count = 0;
    for (int i = 0; i < 1000; i++) {
        int index = rand() % TEST_SIZE;
        std::string member = "player_" + std::to_string(index);
        if (sl.remove(member)) {
            remove_count++;
        }
    }

    end_time = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    std::cout << "删除完成，成功删除 " << remove_count << " 个成员" << std::endl;
    std::cout << "删除后大小: " << sl.size() << std::endl;
    std::cout << "删除耗时: " << duration.count() << " 毫秒" << std::endl;

    std::cout << "\n5. 测试范围查询" << std::endl;
    start_time = std::chrono::high_resolution_clock::now();

    // 测试分数范围查询
    auto range_result = sl.range(4000.0, 6000.0);

    end_time = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    std::cout << "范围查询完成，找到 " << range_result.size() << " 个成员" << std::endl;
    std::cout << "范围查询耗时: " << duration.count() << " 毫秒" << std::endl;

    std::cout << "\n6. 测试前100名查询" << std::endl;
    start_time = std::chrono::high_resolution_clock::now();

    // 测试前100名查询
    auto top100_result = sl.range(1, 100);

    end_time = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    std::cout << "前100名查询完成,找到 " << top100_result.size() << " 个成员" << std::endl;
    std::cout << "前100名查询耗时: " << duration.count() << " 毫秒" << std::endl;

    std::cout << "\n=== 大数据测试完成 ===" << std::endl;
}


*/
int main()
{
    // test_skiplist();
    // test_skiplist_large_data();

    return 0;
}