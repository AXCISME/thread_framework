#ifndef THREAD_MANAGER_H
#define THREAD_MANAGER_H

#include "IThreadWorker.h"
#include <thread>
#include <vector>
#include <memory>
#include <unordered_map>
#include <mutex>
#include <condition_variable>

namespace thread_framework {

/**
 * @brief 线程信息结构
 */
struct ThreadInfo {
    std::unique_ptr<std::thread> thread;       ///< 线程对象
    std::unique_ptr<IThreadWorker> worker;     ///< 工作者对象
    std::atomic<bool> running{false};          ///< 运行状态
    std::string name;                          ///< 线程名称
    std::chrono::steady_clock::time_point startTime; ///< 启动时间

    ThreadInfo(std::string n = "") : name(std::move(n)) {}
};

/**
 * @brief 线程管理器
 *
 * 负责管理多个线程的生命周期，提供统一的线程创建、启动、停止、监控接口。
 * 支持通过工厂模式创建线程，支持线程池和资源管理。
 */
class ThreadManager {
public:
    /**
     * @brief 构造函数
     *
     * @param maxThreads 最大线程数限制，0表示无限制
     */
    explicit ThreadManager(size_t maxThreads = 0) : maxThreads_(maxThreads) {}

    /**
     * @brief 析构函数
     *
     * 自动停止所有线程并清理资源
     */
    ~ThreadManager() {
        stopAll();
        waitForAll();
    }

    /**
     * @brief 添加线程工作者工厂
     *
     * @param factory 工厂对象的智能指针
     * @param type 工厂类型名称，用于后续创建线程
     * @return true 添加成功
     * @return false 添加失败（类型名称已存在）
     */
    bool addFactory(std::unique_ptr<IThreadWorkerFactory> factory, const std::string& type) {
        std::lock_guard<std::mutex> lock(factoriesMutex_);

        if (factories_.find(type) != factories_.end()) {
            return false; // 类型已存在
        }

        factories_[type] = std::move(factory);
        return true;
    }

    /**
     * @brief 创建并启动线程
     *
     * @param type 工厂类型名称
     * @param name 线程名称，如果为空则自动生成
     * @return size_t 线程ID，如果创建失败返回 SIZE_MAX
     */
    size_t createThread(const std::string& type, const std::string& name = "") {
        std::lock_guard<std::mutex> lock(factoriesMutex_);

        auto it = factories_.find(type);
        if (it == factories_.end()) {
            return SIZE_MAX; // 工厂类型不存在
        }

        // 检查线程数限制
        if (maxThreads_ > 0 && threads_.size() >= maxThreads_) {
            return SIZE_MAX; // 达到最大线程数限制
        }

        auto worker = it->second->createWorker();
        if (!worker) {
            return SIZE_MAX; // 创建工作者失败
        }

        return startWorker(std::move(worker), name.empty() ? type + "_" + std::to_string(nextId_++) : name);
    }

    /**
     * @brief 直接创建并启动线程（使用指定的工作者）
     *
     * @param worker 工作者对象的智能指针
     * @param name 线程名称
     * @return size_t 线程ID，如果创建失败返回 SIZE_MAX
     */
    size_t createThreadWithWorker(std::unique_ptr<IThreadWorker> worker, const std::string& name = "") {
        if (!worker) {
            return SIZE_MAX;
        }

        // 检查线程数限制
        if (maxThreads_ > 0 && threads_.size() >= maxThreads_) {
            return SIZE_MAX;
        }

        return startWorker(std::move(worker), name.empty() ? worker->getType() + "_" + std::to_string(nextId_++) : name);
    }

    /**
     * @brief 停止指定线程
     *
     * @param threadId 线程ID
     * @return true 停止成功
     * @return false 停止失败（线程不存在或已停止）
     */
    bool stopThread(size_t threadId) {
        std::lock_guard<std::mutex> lock(threadsMutex_);

        auto it = threads_.find(threadId);
        if (it == threads_.end()) {
            return false;
        }

        auto& info = it->second;
        info.worker->onStop();

        if (info.thread && info.thread->joinable()) {
            info.thread->join();
        }

        return true;
    }

    /**
     * @brief 暂停指定线程
     *
     * @param threadId 线程ID
     * @return true 暂停成功
     * @return false 暂停失败
     */
    bool pauseThread(size_t threadId) {
        std::lock_guard<std::mutex> lock(threadsMutex_);

        auto it = threads_.find(threadId);
        if (it == threads_.end()) {
            return false;
        }

        auto& info = it->second;
        if (info.worker->isRunning()) {
            info.worker->onPause();
            return true;
        }

        return false;
    }

    /**
     * @brief 恢复指定线程
     *
     * @param threadId 线程ID
     * @return true 恢复成功
     * @return false 恢复失败
     */
    bool resumeThread(size_t threadId) {
        std::lock_guard<std::mutex> lock(threadsMutex_);

        auto it = threads_.find(threadId);
        if (it == threads_.end()) {
            return false;
        }

        auto& info = it->second;
        if (info.worker->isPaused()) {
            info.worker->onResume();
            return true;
        }

        return false;
    }

    /**
     * @brief 停止所有线程
     */
    void stopAll() {
        std::lock_guard<std::mutex> lock(threadsMutex_);

        for (auto& pair : threads_) {
            auto& info = pair.second;
            info.worker->onStop();
        }
    }

    /**
     * @brief 等待所有线程完成
     */
    void waitForAll() {
        std::unique_lock<std::mutex> lock(threadsMutex_);

        condition_.wait(lock, [this]() {
            return allThreadsFinished();
        });

        // 清理已完成的线程
        cleanupFinishedThreadsUnsafe();
    }

    /**
     * @brief 获取活跃线程数量
     *
     * @return size_t 活跃线程数量
     */
    size_t getActiveThreadCount() const {
        std::lock_guard<std::mutex> lock(threadsMutex_);

        size_t count = 0;
        for (const auto& pair : threads_) {
            if (pair.second.worker->isRunning() || pair.second.worker->isPaused()) {
                count++;
            }
        }
        return count;
    }

    /**
     * @brief 获取总线程数量
     *
     * @return size_t 总线程数量
     */
    size_t getTotalThreadCount() const {
        std::lock_guard<std::mutex> lock(threadsMutex_);
        return threads_.size();
    }

    /**
     * @brief 获取线程状态信息
     *
     * @param threadId 线程ID
     * @return std::string 状态信息，如果线程不存在返回空字符串
     */
    std::string getThreadStatus(size_t threadId) const {
        std::lock_guard<std::mutex> lock(threadsMutex_);

        auto it = threads_.find(threadId);
        if (it == threads_.end()) {
            return "";
        }

        const auto& info = it->second;
        std::string stateStr;

        switch (info.worker->getState()) {
            case ThreadState::RUNNING: stateStr = "RUNNING"; break;
            case ThreadState::STOPPED: stateStr = "STOPPED"; break;
            case ThreadState::PAUSED: stateStr = "PAUSED"; break;
            case ThreadState::FINISHED: stateStr = "FINISHED"; break;
        }

        return info.name + " [" + info.worker->getType() + "]: " + stateStr;
    }

    /**
     * @brief 获取所有线程的状态信息
     *
     * @return std::vector<std::string> 所有线程的状态信息
     */
    std::vector<std::string> getAllThreadStatus() const {
        std::lock_guard<std::mutex> lock(threadsMutex_);

        std::vector<std::string> status;
        for (const auto& pair : threads_) {
            status.push_back(getThreadStatus(pair.first));
        }
        return status;
    }

    /**
     * @brief 清理已完成的线程
     */
    void cleanupFinishedThreads() {
        std::lock_guard<std::mutex> lock(threadsMutex_);
        cleanupFinishedThreadsUnsafe();
    }

    /**
     * @brief 设置最大线程数
     *
     * @param maxThreads 最大线程数，0表示无限制
     */
    void setMaxThreads(size_t maxThreads) {
        maxThreads_ = maxThreads;
    }

    /**
     * @brief 获取最大线程数
     *
     * @return size_t 最大线程数
     */
    size_t getMaxThreads() const {
        return maxThreads_;
    }

private:
    std::unordered_map<std::string, std::unique_ptr<IThreadWorkerFactory>> factories_;
    std::unordered_map<size_t, ThreadInfo> threads_;
    mutable std::mutex factoriesMutex_;
    mutable std::mutex threadsMutex_;
    std::condition_variable condition_;
    size_t maxThreads_;
    std::atomic<size_t> nextId_{1};

    /**
     * @brief 启动工作者
     */
    size_t startWorker(std::unique_ptr<IThreadWorker> worker, const std::string& name) {
        size_t threadId = nextId_++;

        auto info = std::make_unique<ThreadInfo>(name);
        info->worker = std::move(worker);
        info->startTime = std::chrono::steady_clock::now();

        // 初始化工作者
        info->worker->onInitialize();

        // 创建并启动线程
        std::thread newThread([this, threadId]() {
            auto it = threads_.find(threadId);
            if (it != threads_.end()) {
                auto& info = it->second;
                info.running.store(true);
                info.worker->onStart();

                try {
                    info.worker->run();
                } catch (const std::exception& e) {
                    info.worker->onError(e.what());
                }

                // setState is protected, but the worker should set its own state
                // We'll let the worker's run() method handle this
                info.running.store(false);
                condition_.notify_all();
            }
        });

        // 存储工作者信息
        ThreadInfo& threadInfo = threads_[threadId];
        threadInfo.worker = std::move(info->worker);
        threadInfo.startTime = info->startTime;

        // 设置线程
        threadInfo.thread = std::make_unique<std::thread>(std::move(newThread));
        return threadId;
    }

    /**
     * @brief 检查所有线程是否已完成
     */
    bool allThreadsFinished() const {
        for (const auto& pair : threads_) {
            if (pair.second.running.load()) {
                return false;
            }
        }
        return true;
    }

    /**
     * @brief 清理已完成的线程（不使用锁）
     */
    void cleanupFinishedThreadsUnsafe() {
        auto it = threads_.begin();
        while (it != threads_.end()) {
            auto& info = it->second;
            if (!info.running.load() && info.worker->isFinished()) {
                if (info.thread && info.thread->joinable()) {
                    info.thread->join();
                }
                it = threads_.erase(it);
            } else {
                ++it;
            }
        }
    }
};

} // namespace thread_framework

#endif // THREAD_MANAGER_H