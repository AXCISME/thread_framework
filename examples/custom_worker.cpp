/**
 * @file custom_worker.cpp
 * @brief 自定义工作者示例
 *
 * 这个文件展示了如何继承IThreadWorker接口来创建自定义的工作者类。
 */

#include "../include/thread_framework/IThreadWorker.h"
#include "../include/thread_framework/ThreadManager.h"
#include <iostream>
#include <fstream>
#include <random>
#include <chrono>

using namespace thread_framework;

/**
 * @brief 文件监控工作者 - 自定义工作者示例
 *
 * 继承IThreadWorker，实现文件监控功能
 */
class FileMonitorWorker : public IThreadWorker {
private:
    std::string filePath_;
    std::chrono::milliseconds checkInterval_;
    std::atomic<size_t> lastFileSize_{0};
    std::atomic<int> changeCount_{0};

public:
    /**
     * @brief 构造函数
     *
     * @param filePath 要监控的文件路径
     * @param interval 检查间隔
     */
    FileMonitorWorker(const std::string& filePath, std::chrono::milliseconds interval = std::chrono::seconds(5))
        : filePath_(filePath), checkInterval_(interval) {}

    /**
     * @brief 执行文件监控逻辑
     */
    void run() override {
        setState(ThreadState::RUNNING);

        std::cout << "[" << getType() << "] 开始监控文件: " << filePath_ << std::endl;

        while (shouldContinue()) {
            checkFile();
            std::this_thread::sleep_for(checkInterval_);
        }

        setState(ThreadState::FINISHED);
    }

    /**
     * @brief 获取工作者类型
     */
    std::string getType() const override {
        return "FileMonitorWorker";
    }

    /**
     * @brief 获取工作者描述
     */
    std::string getDescription() const override {
        return "File monitor for: " + filePath_;
    }

    /**
     * @brief 获取文件变化次数
     */
    int getChangeCount() const {
        return changeCount_.load();
    }

protected:
    /**
     * @brief 检查文件变化
     */
    void checkFile() {
        std::ifstream file(filePath_, std::ios::ate | std::ios::binary);
        if (file.is_open()) {
            size_t currentSize = file.tellg();

            if (lastFileSize_.load() != currentSize) {
                changeCount_++;
                std::cout << "[" << getType() << "] 检测到文件变化 #" << changeCount_.load()
                          << " - 大小: " << currentSize << " bytes" << std::endl;
                lastFileSize_.store(currentSize);
            } else {
                std::cout << "[" << getType() << "] 文件未变化" << std::endl;
            }
        } else {
            std::cout << "[" << getType() << "] 无法打开文件: " << filePath_ << std::endl;
        }
    }

    void onStop() override {
        std::cout << "[" << getType() << "] 文件监控停止，共检测到 " << getChangeCount() << " 次变化" << std::endl;
    }
};

/**
 * @brief 数据处理工作者 - 批处理示例
 *
 * 继承IThreadWorker，实现数据处理功能
 */
class DataProcessorWorker : public IThreadWorker {
private:
    std::vector<int> data_;
    std::function<int(const std::vector<int>&)> processor_;
    std::atomic<int> result_{0};

public:
    /**
     * @brief 构造函数
     *
     * @param data 要处理的数据
     * @param processor 数据处理函数
     */
    DataProcessorWorker(const std::vector<int>& data, std::function<int(const std::vector<int>&)> processor)
        : data_(data), processor_(processor) {}

    /**
     * @brief 执行数据处理
     */
    void run() override {
        setState(ThreadState::RUNNING);

        std::cout << "[" << getType() << "] 开始处理 " << data_.size() << " 个数据项" << std::endl;

        if (processor_) {
            try {
                result_.store(processor_(data_));
                std::cout << "[" << getType() << "] 数据处理完成，结果: " << result_.load() << std::endl;
            } catch (const std::exception& e) {
                onError(std::string("数据处理失败: ") + e.what());
            }
        }

        setState(ThreadState::FINISHED);
    }

    /**
     * @brief 获取工作者类型
     */
    std::string getType() const override {
        return "DataProcessorWorker";
    }

    /**
     * @brief 获取处理结果
     */
    int getResult() const {
        return result_.load();
    }

    void onStart() override {
        std::cout << "[" << getType() << "] 初始化数据处理器..." << std::endl;
    }
};

/**
 * @brief 网络检查工作者 - 模拟网络连接检查
 */
class NetworkCheckerWorker : public IThreadWorker {
private:
    std::vector<std::string> endpoints_;
    std::chrono::milliseconds checkInterval_;
    std::atomic<int> successCount_{0};
    std::atomic<int> failCount_{0};

public:
    /**
     * @brief 构造函数
     *
     * @param endpoints 要检查的网络端点
     * @param interval 检查间隔
     */
    NetworkCheckerWorker(const std::vector<std::string>& endpoints, std::chrono::milliseconds interval = std::chrono::seconds(3))
        : endpoints_(endpoints), checkInterval_(interval) {}

    /**
     * @brief 执行网络检查
     */
    void run() override {
        setState(ThreadState::RUNNING);

        std::cout << "[" << getType() << "] 开始网络检查，监控 " << endpoints_.size() << " 个端点" << std::endl;

        while (shouldContinue()) {
            checkEndpoints();
            std::this_thread::sleep_for(checkInterval_);
        }

        setState(ThreadState::FINISHED);
    }

    /**
     * @brief 获取工作者类型
     */
    std::string getType() const override {
        return "NetworkCheckerWorker";
    }

    /**
     * @brief 获取成功次数
     */
    int getSuccessCount() const {
        return successCount_.load();
    }

    /**
     * @brief 获取失败次数
     */
    int getFailCount() const {
        return failCount_.load();
    }

protected:
    /**
     * @brief 检查所有端点
     */
    void checkEndpoints() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(100, 500);

        for (const auto& endpoint : endpoints_) {
            // 模拟网络检查延迟
            std::this_thread::sleep_for(std::chrono::milliseconds(dis(gen)));

            // 模拟网络检查结果（70%成功率）
            bool success = (dis(gen) % 100) < 70;

            if (success) {
                successCount_++;
                std::cout << "[" << getType() << "] ✓ " << endpoint << " - 连接正常" << std::endl;
            } else {
                failCount_++;
                std::cout << "[" << getType() << "] ✗ " << endpoint << " - 连接失败" << std::endl;
            }
        }

        std::cout << "[" << getType() << "] 检查完成 - 成功: " << successCount_.load()
                  << ", 失败: " << failCount_.load() << std::endl;
    }

    void onStop() override {
        std::cout << "[" << getType() << "] 网络检查停止 - 总计成功: " << getSuccessCount()
                  << ", 失败: " << getFailCount() << std::endl;
    }
};

int main() {
    std::cout << "=== 自定义工作者示例 ===" << std::endl;

    ThreadManager manager;

    // 示例1: 文件监控工作者
    std::cout << "\n1. 文件监控工作者" << std::endl;

    // 创建一个测试文件
    std::ofstream testFile("test_monitor.txt");
    testFile << "Initial content" << std::endl;
    testFile.close();

    auto fileMonitor = std::make_unique<FileMonitorWorker>("test_monitor.txt", std::chrono::seconds(2));
    size_t fileMonitorId = manager.createThreadWithWorker(std::move(fileMonitor), "FileMonitor");

    // 示例2: 数据处理工作者
    std::cout << "\n2. 数据处理工作者" << std::endl;

    std::vector<int> numbers = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    auto dataProcessor = std::make_unique<DataProcessorWorker>(numbers, [](const std::vector<int>& data) {
        int sum = 0;
        for (int num : data) {
            sum += num;
            std::this_thread::sleep_for(std::chrono::milliseconds(200)); // 模拟处理时间
        }
        return sum;
    });

    size_t dataProcessorId = manager.createThreadWithWorker(std::move(dataProcessor), "DataProcessor");

    // 示例3: 网络检查工作者
    std::cout << "\n3. 网络检查工作者" << std::endl;

    std::vector<std::string> endpoints = {
        "api.example.com",
        "cdn.example.com",
        "db.example.com"
    };

    auto networkChecker = std::make_unique<NetworkCheckerWorker>(endpoints, std::chrono::seconds(3));
    size_t networkCheckerId = manager.createThreadWithWorker(std::move(networkChecker), "NetworkChecker");

    // 运行一段时间并监控状态
    std::cout << "\n4. 运行监控" << std::endl;
    for (int i = 0; i < 10; ++i) {
        std::this_thread::sleep_for(std::chrono::seconds(1));

        std::cout << "\n--- 状态检查 #" << (i+1) << " ---" << std::endl;
        auto statuses = manager.getAllThreadStatus();
        for (const auto& status : statuses) {
            std::cout << "  " << status << std::endl;
        }

        // 在第5秒时修改测试文件，触发文件监控
        if (i == 4) {
            std::ofstream file("test_monitor.txt", std::ios::app);
            file << "Additional content at " << std::time(nullptr) << std::endl;
            file.close();
            std::cout << "  [主线程] 已修改测试文件" << std::endl;
        }
    }

    // 停止所有线程
    std::cout << "\n5. 停止所有线程..." << std::endl;
    manager.stopAll();
    manager.waitForAll();

    // 清理测试文件
    std::remove("test_monitor.txt");

    std::cout << "\n=== 自定义工作者示例完成 ===" << std::endl;

    return 0;
}