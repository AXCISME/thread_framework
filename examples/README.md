# 线程框架使用指南

这个目录包含了线程框架的使用示例，展示了如何在实际项目中使用这个多态线程管理系统。

## 文件结构

```
thread_framework/
├── include/thread_framework/
│   ├── IThreadWorker.h      # 线程工作者接口定义
│   ├── ThreadManager.h      # 线程管理器
│   └── BaseWorkers.h        # 基础工作者实现
├── examples/
│   ├── basic_usage.cpp      # 基础使用示例
│   ├── custom_worker.cpp    # 自定义工作者示例
│   └── README.md           # 本文件
└── tests/                   # 测试文件（待添加）
```

## 快速开始

### 1. 包含头文件

```cpp
#include "thread_framework/ThreadManager.h"
#include "thread_framework/BaseWorkers.h"
```

### 2. 创建线程管理器

```cpp
thread_framework::ThreadManager manager;
```

### 3. 创建并启动线程

```cpp
// 方式1: 使用预定义的工作者
auto monitor = std::make_unique<thread_framework::MonitorWorker>(
    std::chrono::seconds(1),  // 监控间隔
    []() { std::cout << "Monitoring..." << std::endl; }  // 监控回调
);

size_t threadId = manager.createThreadWithWorker(std::move(monitor), "MyMonitor");

// 方式2: 使用任务工作者
auto task = std::make_unique<thread_framework::TaskWorker>(
    []() { std::cout << "Executing task..." << std::endl; },
    "My Task Description"
);

manager.createThreadWithWorker(std::move(task), "MyTask");
```

## 工作者类型

### MonitorWorker（监控工作者）
- **用途**: 持续运行的监控任务
- **特点**: 定期执行回调函数，适合系统监控、日志记录等
- **生命周期**: 持续运行，直到手动停止

```cpp
auto monitor = std::make_unique<MonitorWorker>(
    std::chrono::milliseconds(1000),  // 检查间隔
    []() { /* 监控逻辑 */ }
);
```

### TaskWorker（任务工作者）
- **用途**: 一次性执行的任务
- **特点**: 执行一次任务后自动结束，适合异步操作、批处理等
- **生命周期**: 执行完自动结束

```cpp
auto task = std::make_unique<TaskWorker>(
    []() { /* 任务逻辑 */ },
    "任务描述"
);
```

### TimerWorker（定时器工作者）
- **用途**: 定时触发回调
- **特点**: 按固定间隔触发，可设置最大触发次数
- **生命周期**: 达到最大次数后自动结束

```cpp
auto timer = std::make_unique<TimerWorker>(
    std::chrono::milliseconds(500),  // 触发间隔
    []() { /* 定时逻辑 */ },
    10  // 最大触发次数，-1表示无限次
);
```

### LoopWorker（循环工作者）
- **用途**: 执行固定次数的循环
- **特点**: 适合需要精确控制循环次数的任务
- **生命周期**: 完成指定次数后自动结束

```cpp
auto loop = std::make_unique<LoopWorker>(
    5,  // 循环次数
    [](int current) { std::cout << "Loop " << current << std::endl; }
);
```

## 自定义工作者

创建自定义工作者非常简单，只需要继承 `IThreadWorker` 接口：

```cpp
class MyCustomWorker : public thread_framework::IThreadWorker {
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
        return "MyCustomWorker";
    }

private:
    void doWork() {
        // 具体工作逻辑
    }
};
```

## 线程管理

### 基本操作

```cpp
// 创建线程
size_t threadId = manager.createThreadWithWorker(std::move(worker), "ThreadName");

// 暂停线程
manager.pauseThread(threadId);

// 恢复线程
manager.resumeThread(threadId);

// 停止线程
manager.stopThread(threadId);

// 等待所有线程完成
manager.waitForAll();
```

### 状态监控

```cpp
// 获取活跃线程数
size_t activeCount = manager.getActiveThreadCount();

// 获取所有线程状态
auto statuses = manager.getAllThreadStatus();
for (const auto& status : statuses) {
    std::cout << status << std::endl;
}

// 获取特定线程状态
std::string status = manager.getThreadStatus(threadId);
```

## 编译示例

```bash
# 基础使用示例
g++ -std=c++17 -pthread examples/basic_usage.cpp -o basic_usage

# 自定义工作者示例
g++ -std=c++17 -pthread examples/custom_worker.cpp -o custom_worker
```

## 注意事项

1. **线程安全**: 框架内部使用 `std::atomic` 和互斥锁确保线程安全
2. **资源管理**: 线程管理器会自动清理资源，确保没有内存泄漏
3. **异常处理**: 在 `run()` 方法中添加异常处理，避免线程崩溃
4. **回调函数**: 回调函数应该尽量简洁，避免长时间阻塞

## 扩展性

- 添加新的工作者类型只需要继承 `IThreadWorker` 接口
- 可以通过工厂模式动态创建不同类型的线程
- 支持自定义线程状态和生命周期管理
- 可以集成到现有的项目中，无需修改现有代码