#ifndef ITHREAD_WORKER_H
#define ITHREAD_WORKER_H

#include <string>
#include <iostream>
#include <memory>
#include <functional>
#include <atomic>
#include <chrono>
#include <thread>

/**
 * @file IThreadWorker.h
 * @brief 线程工作者接口定义
 *
 * 这个文件定义了线程工作者的基础接口，所有具体的工作者类都应该继承这个接口
 *
 * @author Thread Framework Team
 * @version 1.0.0
 * @date 2024
 */

namespace thread_framework {

/**
 * @brief 线程状态枚举
 */
enum class ThreadState {
    STOPPED,    ///< 线程已停止
    RUNNING,    ///< 线程正在运行
    PAUSED,     ///< 线程已暂停
    FINISHED    ///< 线程已完成
};

/**
 * @brief 线程工作者基类接口
 *
 * 这是所有线程工作者必须实现的接口。定义了线程的基本行为和生命周期管理。
 * 派生类需要实现纯虚函数来定义具体的工作逻辑。
 */
class IThreadWorker {
public:
    virtual ~IThreadWorker() = default;

    /**
     * @brief �线程执行的主要逻辑
     *
     * 这是线程的入口点，派生类必须实现这个方法来定义具体的工作逻辑。
     * 线程管理器会调用这个方法来启动线程的执行。
     */
    virtual void run() = 0;

    /**
     * @brief 获取工作者的类型名称
     *
     * @return std::string 工作者类型的名称，用于日志和调试
     */
    virtual std::string getType() const = 0;

    /**
     * @brief 获取工作者的描述信息
     *
     * @return std::string 工作者的详细描述
     */
    virtual std::string getDescription() const {
        return getType() + " worker";
    }

    // 线程控制方法 - 提供默认实现，派生类可以重写

    /**
     * @brief 线程启动前的初始化
     *
     * 在线程开始执行前调用，派生类可以重写这个方法来执行初始化逻辑。
     * 默认实现为空。
     */
    virtual void onInitialize() {}

    /**
     * @brief 线程启动时的回调
     *
     * 在线程开始执行时调用，派生类可以重写这个方法来执行启动逻辑。
     */
    virtual void onStart() {}

    /**
     * @brief 线程停止时的回调
     *
     * 在线程停止时调用，派生类可以重写这个方法来执行清理逻辑。
     */
    virtual void onStop() {}

    /**
     * @brief 线程暂停时的回调
     *
     * 在线程暂停时调用，派生类可以重写这个方法来执行暂停逻辑。
     */
    virtual void onPause() {}

    /**
     * @brief 线程恢复时的回调
     *
     * 在线程从暂停状态恢复时调用，派生类可以重写这个方法来执行恢复逻辑。
     */
    virtual void onResume() {}

    /**
     * @brief 错误处理回调
     *
     * 当线程执行过程中发生错误时调用。
     *
     * @param error 错误信息
     */
    virtual void onError(const std::string& error) {}

    /**
     * @brief 获取线程状态
     *
     * @return ThreadState 当前线程状态
     */
    virtual ThreadState getState() const { return state.load(); }

    /**
     * @brief 检查线程是否正在运行
     *
     * @return true 线程正在运行
     * @return false 线程未运行
     */
    virtual bool isRunning() const { return state.load() == ThreadState::RUNNING; }

    /**
     * @brief 检查线程是否已暂停
     *
     * @return true 线程已暂停
     * @return false 线程未暂停
     */
    virtual bool isPaused() const { return state.load() == ThreadState::PAUSED; }

    /**
     * @brief 检查线程是否已停止
     *
     * @return true 线程已停止
     * @return false 线程未停止
     */
    virtual bool isStopped() const { return state.load() == ThreadState::STOPPED; }

    /**
     * @brief 检查线程是否已完成
     *
     * @return true 线程已完成
     * @return false 线程未完成
     */
    virtual bool isFinished() const { return state.load() == ThreadState::FINISHED; }

protected:
    std::atomic<ThreadState> state{ThreadState::STOPPED};
    std::atomic<bool> shouldStop{false};
    std::atomic<bool> shouldPause{false};

    /**
     * @brief 检查线程是否应该继续执行
     *
     * 这个方法应该在派生类的run()方法中定期调用，以响应暂停和停止请求。
     *
     * @return true 线程应该继续执行
     * @return false 线程应该停止执行
     */
    virtual bool shouldContinue() {
        // 处理暂停状态
        while (shouldPause.load() && !shouldStop.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        return !shouldStop.load();
    }

    /**
     * @brief 设置线程状态
     *
     * @param newState 新的线程状态
     */
    virtual void setState(ThreadState newState) {
        state.store(newState);
    }
};

/**
 * @brief 线程工作者工厂接口
 *
 * 用于创建线程工作者的工厂接口，支持依赖注入和配置化创建。
 */
class IThreadWorkerFactory {
public:
    virtual ~IThreadWorkerFactory() = default;

    /**
     * @brief 创建线程工作者实例
     *
     * @return std::unique_ptr<IThreadWorker> 线程工作者的智能指针
     */
    virtual std::unique_ptr<IThreadWorker> createWorker() = 0;

    /**
     * @brief 获取工厂类型名称
     *
     * @return std::string 工厂类型的名称
     */
    virtual std::string getFactoryType() const = 0;

    /**
     * @brief 检查工厂是否支持指定的配置
     *
     * @param config 配置参数
     * @return true 支持该配置
     * @return false 不支持该配置
     */
    virtual bool supportsConfig(const std::string& config) const {
        return true; // 默认支持所有配置
    }
};

} // namespace thread_framework

#endif // ITHREAD_WORKER_H