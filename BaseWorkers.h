#ifndef BASE_WORKERS_H
#define BASE_WORKERS_H

#include "IThreadWorker.h"
#include <iostream>
#include <functional>
#include <chrono>
#include <atomic>

namespace thread_framework {

/**
 * @brief 监控工作者 - 持续运行的监控线程
 *
 * 适用于系统监控、日志记录、心跳检测等需要持续运行的任务。
 */
class MonitorWorker : public IThreadWorker {
private:
    std::chrono::milliseconds interval_;
    std::function<void()> callback_;
    std::atomic<bool> enabled_{true};
    std::atomic<int> iterationCount_{0};

public:
    /**
     * @brief 构造函数
     *
     * @param interval 监控间隔，默认1000毫秒
     * @param callback 监控回调函数，可选
     */
    explicit MonitorWorker(std::chrono::milliseconds interval = std::chrono::milliseconds(1000),
                          std::function<void()> callback = nullptr)
        : interval_(interval), callback_(callback) {}

    /**
     * @brief 执行监控逻辑
     */
    void run() override {
        setState(ThreadState::RUNNING);

        while (shouldContinue() && enabled_.load()) {
            iterationCount_++;

            // 执行监控逻辑
            if (callback_) {
                callback_();
            } else {
                // 默认监控逻辑
                defaultMonitorLogic();
            }

            // 等待下一次监控
            std::this_thread::sleep_for(interval_);
        }

        setState(ThreadState::FINISHED);
    }

    /**
     * @brief 获取工作者类型
     */
    std::string getType() const override {
        return "MonitorWorker";
    }

    /**
     * @brief 获取工作者描述
     */
    std::string getDescription() const override {
        return "Monitor worker with " + std::to_string(interval_.count()) + "ms interval";
    }

    /**
     * @brief 获取迭代次数
     */
    int getIterationCount() const {
        return iterationCount_.load();
    }

    /**
     * @brief 启用/禁用监控
     */
    void setEnabled(bool enabled) {
        enabled_.store(enabled);
    }

    /**
     * @brief 检查是否启用
     */
    bool isEnabled() const {
        return enabled_.load();
    }

    /**
     * @brief 设置监控间隔
     */
    void setInterval(std::chrono::milliseconds interval) {
        interval_ = interval;
    }

    /**
     * @brief 获取监控间隔
     */
    std::chrono::milliseconds getInterval() const {
        return interval_;
    }

protected:
    /**
     * @brief 默认监控逻辑
     */
    virtual void defaultMonitorLogic() {
        // 默认的监控实现，子类可以重写
        std::cout << "[" << getType() << "] Monitoring check #" << getIterationCount() << std::endl;
    }

    void onStop() override {
        enabled_.store(false);
        std::cout << "[" << getType() << "] Monitoring stopped after " << getIterationCount() << " checks" << std::endl;
    }
};

/**
 * @brief 任务工作者 - 执行一次性任务
 *
 * 适用于异步操作、批处理、文件处理等执行一次就结束的任务。
 */
class TaskWorker : public IThreadWorker {
private:
    std::function<void()> task_;
    std::string description_;
    std::atomic<bool> completed_{false};

public:
    /**
     * @brief 构造函数
     *
     * @param task 要执行的任务函数
     * @param description 任务描述
     */
    TaskWorker(std::function<void()> task, const std::string& description = "")
        : task_(task), description_(description.empty() ? "Unnamed Task" : description) {}

    /**
     * @brief 执行任务
     */
    void run() override {
        setState(ThreadState::RUNNING);

        if (task_) {
            try {
                task_();
                completed_.store(true);
            } catch (const std::exception& e) {
                onError(std::string("Task execution failed: ") + e.what());
            }
        }

        setState(ThreadState::FINISHED);
    }

    /**
     * @brief 获取工作者类型
     */
    std::string getType() const override {
        return "TaskWorker";
    }

    /**
     * @brief 获取工作者描述
     */
    std::string getDescription() const override {
        return description_;
    }

    /**
     * @brief 检查任务是否完成
     */
    bool isCompleted() const {
        return completed_.load();
    }

    void onStart() override {
        std::cout << "[" << getType() << "] Starting task: " << description_ << std::endl;
    }

    void onStop() override {
        std::cout << "[" << getType() << "] Task stopped: " << description_ << std::endl;
    }
};

/**
 * @brief 定时器工作者 - 周期性触发回调
 *
 * 适用于定时任务、周期性检查、定时器等场景。
 */
class TimerWorker : public IThreadWorker {
private:
    std::chrono::milliseconds interval_;
    std::function<void()> callback_;
    int maxTriggers_;
    std::atomic<int> triggerCount_{0};

public:
    /**
     * @brief 构造函数
     *
     * @param interval 触发间隔
     * @param callback 回调函数
     * @param maxTriggers 最大触发次数，-1表示无限次
     */
    TimerWorker(std::chrono::milliseconds interval,
                std::function<void()> callback,
                int maxTriggers = -1)
        : interval_(interval), callback_(callback), maxTriggers_(maxTriggers) {}

    /**
     * @brief 执行定时器逻辑
     */
    void run() override {
        setState(ThreadState::RUNNING);

        while (shouldContinue()) {
            // 检查是否达到最大触发次数
            if (maxTriggers_ > 0 && triggerCount_.load() >= maxTriggers_) {
                break;
            }

            // 等待触发间隔
            std::this_thread::sleep_for(interval_);

            if (!shouldContinue()) {
                break;
            }

            // 执行回调
            triggerCount_++;
            if (callback_) {
                try {
                    callback_();
                } catch (const std::exception& e) {
                    onError(std::string("Timer callback failed: ") + e.what());
                }
            }
        }

        setState(ThreadState::FINISHED);
    }

    /**
     * @brief 获取工作者类型
     */
    std::string getType() const override {
        return "TimerWorker";
    }

    /**
     * @brief 获取工作者描述
     */
    std::string getDescription() const override {
        return "Timer worker with " + std::to_string(interval_.count()) + "ms interval" +
               (maxTriggers_ > 0 ? " (max " + std::to_string(maxTriggers_) + " triggers)" : " (infinite)");
    }

    /**
     * @brief 获取触发次数
     */
    int getTriggerCount() const {
        return triggerCount_.load();
    }

    /**
     * @brief 设置最大触发次数
     */
    void setMaxTriggers(int maxTriggers) {
        maxTriggers_ = maxTriggers;
    }

    /**
     * @brief 获取最大触发次数
     */
    int getMaxTriggers() const {
        return maxTriggers_;
    }

    void onStart() override {
        std::cout << "[" << getType() << "] Timer started (" << interval_.count() << "ms interval)" << std::endl;
    }

    void onStop() override {
        std::cout << "[" << getType() << "] Timer stopped after " << getTriggerCount() << " triggers" << std::endl;
    }
};

/**
 * @brief 循环工作者 - 执行固定次数的循环任务
 *
 * 适用于需要执行固定次数循环的任务。
 */
class LoopWorker : public IThreadWorker {
private:
    int loopCount_;
    std::function<void(int)> loopCallback_;
    std::function<void()> startCallback_;
    std::function<void()> endCallback_;
    std::atomic<int> currentLoop_{0};

public:
    /**
     * @brief 构造函数
     *
     * @param loopCount 循环次数
     * @param loopCallback 循环回调函数，参数为当前循环次数
     * @param startCallback 开始回调函数，可选
     * @param endCallback 结束回调函数，可选
     */
    LoopWorker(int loopCount,
               std::function<void(int)> loopCallback,
               std::function<void()> startCallback = nullptr,
               std::function<void()> endCallback = nullptr)
        : loopCount_(loopCount), loopCallback_(loopCallback),
          startCallback_(startCallback), endCallback_(endCallback) {}

    /**
     * @brief 执行循环逻辑
     */
    void run() override {
        setState(ThreadState::RUNNING);

        if (startCallback_) {
            startCallback_();
        }

        for (int i = 1; i <= loopCount_ && shouldContinue(); ++i) {
            currentLoop_.store(i);
            if (loopCallback_) {
                try {
                    loopCallback_(i);
                } catch (const std::exception& e) {
                    onError(std::string("Loop callback failed: ") + e.what());
                }
            }
        }

        if (endCallback_) {
            endCallback_();
        }

        setState(ThreadState::FINISHED);
    }

    /**
     * @brief 获取工作者类型
     */
    std::string getType() const override {
        return "LoopWorker";
    }

    /**
     * @brief 获取工作者描述
     */
    std::string getDescription() const override {
        return "Loop worker with " + std::to_string(loopCount_) + " iterations";
    }

    /**
     * @brief 获取当前循环次数
     */
    int getCurrentLoop() const {
        return currentLoop_.load();
    }

    /**
     * @brief 获取总循环次数
     */
    int getLoopCount() const {
        return loopCount_;
    }

    /**
     * @brief 获取进度百分比
     */
    double getProgress() const {
        return static_cast<double>(getCurrentLoop()) / loopCount_ * 100.0;
    }
};

} // namespace thread_framework

#endif // BASE_WORKERS_H