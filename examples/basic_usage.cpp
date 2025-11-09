/**
 * @file basic_usage.cpp
 * @brief 线程框架基础使用示例
 *
 * 这个文件展示了如何使用线程框架来创建和管理不同类型的线程。
 */

#include "../include/thread_framework/ThreadManager.h"
#include "../include/thread_framework/BaseWorkers.h"
#include <iostream>
#include <chrono>

using namespace thread_framework;

int main() {
    std::cout << "=== 线程框架基础使用示例 ===" << std::endl;

    // 创建线程管理器
    ThreadManager manager;

    // 示例1: 直接使用基础工作者类
    std::cout << "\n1. 创建监控线程（持续运行）" << std::endl;

    // 创建监控工作者 - 每1秒执行一次监控
    auto monitor = std::make_unique<MonitorWorker>(
        std::chrono::seconds(1),
        []() {
            static int checkCount = 0;
            checkCount++;
            std::cout << "  [监控] 系统状态检查 #" << checkCount << " - 一切正常" << std::endl;
        }
    );

    size_t monitorId = manager.createThreadWithWorker(std::move(monitor), "SystemMonitor");
    if (monitorId == SIZE_MAX) {
        std::cerr << "创建监控线程失败" << std::endl;
        return 1;
    }

    // 示例2: 创建任务线程（执行一次就结束）
    std::cout << "\n2. 创建异步任务线程" << std::endl;

    auto downloadTask = []() {
        std::cout << "  [任务] 开始下载文件..." << std::endl;
        for (int i = 0; i <= 100; i += 25) {
            if (i > 0) std::this_thread::sleep_for(std::chrono::milliseconds(500));
            std::cout << "  [任务] 下载进度: " << i << "%" << std::endl;
        }
        std::cout << "  [任务] 文件下载完成！" << std::endl;
    };

    auto taskWorker = std::make_unique<TaskWorker>(downloadTask, "文件下载任务");
    size_t taskId = manager.createThreadWithWorker(std::move(taskWorker), "FileDownloader");

    // 示例3: 创建定时器线程（触发5次后结束）
    std::cout << "\n3. 创建定时器线程" << std::endl;

    int timerCount = 0;
    auto timerWorker = std::make_unique<TimerWorker>(
        std::chrono::milliseconds(800),
        [&timerCount]() {
            timerCount++;
            std::cout << "  [定时器] 定时任务执行 #" << timerCount << std::endl;
        },
        5  // 只触发5次
    );

    size_t timerId = manager.createThreadWithWorker(std::move(timerWorker), "MaintenanceTimer");

    // 显示所有线程状态
    std::cout << "\n4. 线程状态监控" << std::endl;
    for (int i = 0; i < 8; ++i) {
        std::this_thread::sleep_for(std::chrono::seconds(1));

        std::cout << "\n--- 第 " << (i+1) << " 次状态检查 ---" << std::endl;
        auto statuses = manager.getAllThreadStatus();
        for (const auto& status : statuses) {
            std::cout << "  " << status << std::endl;
        }

        std::cout << "活跃线程数: " << manager.getActiveThreadCount() << std::endl;
    }

    // 示例4: 线程控制操作
    std::cout << "\n5. 线程控制操作" << std::endl;

    // 暂停监控线程
    std::cout << "暂停监控线程..." << std::endl;
    manager.pauseThread(monitorId);
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // 恢复监控线程
    std::cout << "恢复监控线程..." << std::endl;
    manager.resumeThread(monitorId);
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // 停止监控线程
    std::cout << "停止监控线程..." << std::endl;
    manager.stopThread(monitorId);

    // 等待所有线程完成
    std::cout << "\n6. 等待所有线程完成..." << std::endl;
    manager.waitForAll();

    std::cout << "\n=== 示例完成 ===" << std::endl;

    return 0;
}