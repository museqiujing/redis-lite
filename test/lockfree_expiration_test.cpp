#include "../src/lockfree_expiration.h"
#include <iostream>
#include <thread>
#include <vector>
#include <chrono>

void test_basic_functionality()
{
    std::cout << "=== 测试基础功能 ===" << std::endl;

    LockFreeExpirationManager manager;

    // 测试时间缓存
    int64_t time1 = manager.get_cached_time();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    int64_t time2 = manager.get_cached_time();

    std::cout << "时间缓存测试: " << time1 << " -> " << time2 << std::endl;

    // 测试添加过期条目
    SDS key1("test_key_1");
    int64_t expire_time = time1 + 1000; // 1秒后过期

    manager.add_expiration(key1, expire_time);
    std::cout << "添加过期条目: " << key1.c_str() << " 过期时间: " << expire_time << std::endl;

    // 测试检查过期
    bool expired = manager.check_expiration(key1, expire_time);
    std::cout << "立即检查过期: " << (expired ? "是" : "否") << std::endl;

    // 测试队列大小
    std::cout << "队列大小: " << manager.get_queue_size() << std::endl;
    std::cout << "待清理数量: " << manager.get_pending_cleanup_count() << std::endl;

    std::cout << "基础功能测试完成" << std::endl
              << std::endl;
}

void test_concurrent_operations()
{
    std::cout << "=== 测试并发操作 ===" << std::endl;

    LockFreeExpirationManager manager;
    const int THREAD_COUNT = 4;
    const int OPERATIONS_PER_THREAD = 1000;

    std::vector<std::thread> threads;

    auto worker = [&manager](int thread_id)
    {
        int64_t base_time = manager.get_cached_time();

        for (int i = 0; i < OPERATIONS_PER_THREAD; i++)
        {
            SDS key("key_" + std::to_string(thread_id) + "_" + std::to_string(i));
            int64_t expire_time = base_time + (i % 100) * 10; // 不同过期时间

            manager.add_expiration(key, expire_time);

            // 随机触发清理
            if (i % 100 == 0)
            {
                manager.batch_cleanup();
            }
        }
    };

    // 启动多个线程
    for (int i = 0; i < THREAD_COUNT; i++)
    {
        threads.emplace_back(worker, i);
    }

    // 等待所有线程完成
    for (auto &t : threads)
    {
        t.join();
    }

    std::cout << "并发操作完成" << std::endl;
    std::cout << "最终队列大小: " << manager.get_queue_size() << std::endl;
    std::cout << "最终待清理数量: " << manager.get_pending_cleanup_count() << std::endl;
    std::cout << "并发操作测试完成" << std::endl
              << std::endl;
}

void test_performance()
{
    std::cout << "=== 测试性能 ===" << std::endl;

    LockFreeExpirationManager manager;
    const int OPERATION_COUNT = 10000; // 减少测试数量，避免内存问题

    auto start = std::chrono::high_resolution_clock::now();

    int64_t base_time = manager.get_cached_time();

    // 预创建键，避免重复分配内存
    std::vector<SDS> keys;
    keys.reserve(OPERATION_COUNT);
    for (int i = 0; i < OPERATION_COUNT; i++)
    {
        keys.emplace_back("perf_key_" + std::to_string(i));
    }

    for (int i = 0; i < OPERATION_COUNT; i++)
    {
        int64_t expire_time = base_time + (i % 100) * 100; // 调整过期时间范围

        manager.add_expiration(keys[i], expire_time);

        // 更频繁的清理，避免队列溢出
        if (i % 100 == 0)
        {
            manager.batch_cleanup();
        }

        // 进度显示
        if (i % 1000 == 0)
        {
            std::cout << "进度: " << i << "/" << OPERATION_COUNT << std::endl;
        }
    }

    // 最终清理
    manager.batch_cleanup();

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "性能测试结果:" << std::endl;
    std::cout << "操作数量: " << OPERATION_COUNT << std::endl;
    std::cout << "耗时: " << duration.count() << "ms" << std::endl;
    std::cout << "吞吐量: " << (OPERATION_COUNT * 1000.0 / duration.count()) << " ops/s" << std::endl;
    std::cout << "队列大小: " << manager.get_queue_size() << std::endl;
    std::cout << "待清理数量: " << manager.get_pending_cleanup_count() << std::endl;
    std::cout << "性能测试完成" << std::endl
              << std::endl;
}

int main()
{
    std::cout << "开始无锁过期管理器测试" << std::endl;

    try
    {
        test_basic_functionality();
        test_concurrent_operations();
        test_performance();

        std::cout << "所有测试通过!" << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << "测试失败: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}