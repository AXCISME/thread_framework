# 线程框架 (Thread Framework)

一个基于C++17的现代多态线程管理框架，提供统一的接口来创建和管理不同类型的线程。

## 特性

- ✅ **多态设计**: 统一的接口支持不同类型的线程工作者
- ✅ **类型安全**: 编译时类型检查，避免运行时错误
- ✅ **线程安全**: 内置线程安全的状态管理
- ✅ **易于扩展**: 简单的接口，易于创建自定义工作者
- ✅ **资源管理**: 自动管理线程生命周期，防止资源泄漏
- ✅ **灵活控制**: 支持启动、暂停、恢复、停止等操作
- ✅ **状态监控**: 实时监控线程状态和性能

## 快速开始

### 系统要求

- C++17 或更高版本的编译器
- pthread 支持
- Make 构建工具

### 编译和运行

```bash
# 克隆或下载项目
cd thread_framework

# 编译所有示例
make

# 运行基础示例
make run-basic

# 运行自定义工作者示例
make run-custom

# 查看所有可用命令
make help
```

## 基本使用

```cpp
#include "thread_framework/ThreadManager.h"
#include "thread_framework/BaseWorkers.h"

using namespace thread_framework;

int main() {
    // 创建线程管理器
    ThreadManager manager;

    // 创建监控线程（持续运行）
    auto monitor = std::make_unique<MonitorWorker>(
        std::chrono::seconds(1),
        []() { std::cout << "Monitoring..." << std::endl; }
    );
    size_t monitorId = manager.createThreadWithWorker(std::move(monitor), "Monitor");

    // 创建任务线程（执行一次）
    auto task = std::make_unique<TaskWorker>(
        []() { std::cout << "Task completed!" << std::endl; },
        "My Task"
    );
    manager.createThreadWithWorker(std::move(task), "Task");

    // 等待所有线程完成
    manager.waitForAll();

    return 0;
}
```

## 工作者类型

### 内置工作者

1. **MonitorWorker** - 持续运行的监控任务
2. **TaskWorker** - 一次性执行的任务
3. **TimerWorker** - 定时触发回调
4. **LoopWorker** - 执行固定次数的循环

### 自定义工作者

```cpp
class MyWorker : public IThreadWorker {
public:
    void run() override {
        setState(ThreadState::RUNNING);

        while (shouldContinue()) {
            // 你的业务逻辑
            doWork();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        setState(ThreadState::FINISHED);
    }

    std::string getType() const override {
        return "MyWorker";
    }

private:
    void doWork() {
        // 具体工作逻辑
    }
};
```

## 线程管理

```cpp
ThreadManager manager;

// 创建线程
size_t threadId = manager.createThreadWithWorker(std::move(worker), "ThreadName");

// 线程控制
manager.pauseThread(threadId);
manager.resumeThread(threadId);
manager.stopThread(threadId);

// 状态监控
size_t activeCount = manager.getActiveThreadCount();
auto statuses = manager.getAllThreadStatus();
```

## 项目结构

```
thread_framework/
├── include/thread_framework/
│   ├── IThreadWorker.h      # 线程工作者接口
│   ├── ThreadManager.h      # 线程管理器
│   └── BaseWorkers.h        # 基础工作者实现
├── examples/
│   ├── basic_usage.cpp      # 基础使用示例
│   ├── custom_worker.cpp    # 自定义工作者示例
│   └── README.md           # 使用指南
├── tests/                   # 测试文件
├── Makefile                # 构建配置
└── README.md              # 项目说明
```

## API 文档

### IThreadWorker 接口

所有工作者必须实现的核心接口：

```cpp
class IThreadWorker {
public:
    virtual void run() = 0;                    // 纯虚函数，主要执行逻辑
    virtual std::string getType() const = 0;   // 纯虚函数，工作者类型
    virtual std::string getDescription() const; // 虚函数，工作者描述
    virtual void onInitialize();               // 虚函数，初始化回调
    virtual void onStart();                    // 虚函数，启动回调
    virtual void onStop();                     // 虚函数，停止回调
    virtual void onPause();                    // 虚函数，暂停回调
    virtual void onResume();                   // 虚函数，恢复回调
    virtual void onError(const std::string& error); // 虚函数，错误回调

    // 状态查询
    virtual ThreadState getState() const;
    virtual bool isRunning() const;
    virtual bool isPaused() const;
    virtual bool isStopped() const;
    virtual bool isFinished() const;

protected:
    virtual bool shouldContinue();  // 检查是否应该继续执行
    virtual void setState(ThreadState newState); // 设置线程状态
};
```

### ThreadManager 类

线程管理器的主要方法：

```cpp
class ThreadManager {
public:
    // 线程创建
    size_t createThreadWithWorker(std::unique_ptr<IThreadWorker> worker,
                                 const std::string& name = "");

    // 线程控制
    bool stopThread(size_t threadId);
    bool pauseThread(size_t threadId);
    bool resumeThread(size_t threadId);
    void stopAll();
    void waitForAll();

    // 状态查询
    size_t getActiveThreadCount() const;
    size_t getTotalThreadCount() const;
    std::string getThreadStatus(size_t threadId) const;
    std::vector<std::string> getAllThreadStatus() const;

    // 资源管理
    void cleanupFinishedThreads();
    void setMaxThreads(size_t maxThreads);
};
```

## 使用场景

- **系统监控**: 使用 MonitorWorker 持续监控系统状态
- **异步任务**: 使用 TaskWorker 执行耗时的后台操作
- **定时任务**: 使用 TimerWorker 定期执行维护任务
- **数据处理**: 使用 LoopWorker 处理批量数据
- **网络检查**: 自定义工作者定期检查网络连接状态
- **文件监控**: 自定义工作者监控文件变化

## 性能特性

- **低开销**: 虚函数调用开销极小
- **内存效率**: 使用智能指针自动管理内存
- **线程安全**: 原子操作和互斥锁保证线程安全
- **可扩展**: 支持大量线程并发执行

## 构建选项

```bash
# 发布版本
make

# 调试版本
make debug

# 自定义编译选项
make CXXFLAGS="-O3 -march=native"

# 安装到系统
make install

# 创建发布包
make package
```

## 许可证

本项目采用 MIT 许可证，详见 LICENSE 文件。

## 贡献

欢迎提交 Issue 和 Pull Request 来改进这个项目。

## 更新日志

### v1.0.0
- 初始版本发布
- 支持基本的线程管理功能
- 提供多种内置工作者类型
- 完整的文档和示例