# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Common Development Commands

### Building and Running
- `make` - Build all examples (default target)
- `make examples` - Build all example programs
- `make debug` - Build debug version with debug symbols
- `make run-basic` - Build and run the basic usage example
- `make run-custom` - Build and run the custom worker example
- `make run-examples` - Build and run all examples
- `make clean` - Remove all build artifacts (bin/, build/ directories)

### Testing and Verification
- `make tests` - Build test programs (if any exist in tests/)
- `make run-tests` - Build and run all tests
- `make verify` - Quick verification that headers compile and linking works
- `make check-deps` - Check for required dependencies (g++, make, pthread)

### Utilities
- `make help` - Show all available targets and usage
- `make info` - Display framework information
- `make package` - Create distribution tar.gz package
- `make install` - Install headers to /usr/local/include/thread_framework/
- `make uninstall` - Remove framework from system

### Build Configuration
- Compiler: g++ with C++17 standard
- Flags: `-Wall -Wextra -Wpedantic -O2` (release), `-g -DDEBUG` (debug)
- Includes: `-Iinclude`
- Libraries: `-pthread`
- Build directories: `bin/` for executables, `build/` for intermediate files

## Architecture Overview

This is a modern C++17 polymorphic thread management framework with three core components:

### Core Interface (`IThreadWorker.h`)
- **Abstract base class**: `IThreadWorker` - defines the contract for all thread workers
- **Thread states**: STOPPED, RUNNING, PAUSED, FINISHED (enum class `ThreadState`)
- **Key methods**: `run()` (pure virtual), `getType()` (pure virtual), `shouldContinue()`, `setState()`
- **Factory interface**: `IThreadWorkerFactory` for creating workers via factory pattern
- **Thread-safe**: Uses `std::atomic` for state management

### Thread Management (`ThreadManager.h`)
- **Central manager**: `ThreadManager` class handles multiple thread lifecycles
- **Thread info**: `ThreadInfo` struct contains thread object, worker, name, start time
- **Operations**: create, start, stop, pause, resume threads by ID
- **Monitoring**: Get active thread count, status information, cleanup finished threads
- **Thread limits**: Configurable maximum thread count (0 = unlimited)
- **Thread-safe**: Uses mutexes for thread map operations, condition variable for waiting

### Built-in Workers (`BaseWorkers.h`)
- **MonitorWorker**: Continuous monitoring with configurable intervals
- **TaskWorker**: One-time task execution with completion tracking
- **TimerWorker**: Periodic callback triggering with max trigger limits
- **LoopWorker**: Fixed-count iteration loops with progress tracking

## Key Design Patterns

### Polymorphic Worker System
All workers inherit from `IThreadWorker` and implement `run()` and `getType()` methods. The framework manages workers through their interface, allowing runtime substitution of different worker types.

### RAII Resource Management
The `ThreadManager` destructor automatically stops all threads and waits for completion. Smart pointers (`std::unique_ptr`) manage worker and thread lifetimes.

### Factory Pattern Support
The framework supports both direct worker creation (`createThreadWithWorker`) and factory-based creation (`addFactory` + `createThread`).

### Thread-safe State Management
- Atomic variables for thread states and control flags
- Mutex protection for shared data structures
- Condition variables for thread synchronization

## Usage Patterns

### Creating Custom Workers
```cpp
class MyWorker : public IThreadWorker {
public:
    void run() override {
        setState(ThreadState::RUNNING);
        while (shouldContinue()) {
            // Your logic here
            doWork();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        setState(ThreadState::FINISHED);
    }

    std::string getType() const override {
        return "MyWorker";
    }
};
```

### Common Thread Control Flow
1. Create ThreadManager instance
2. Create worker instances (built-in or custom)
3. Use `createThreadWithWorker()` to start threads
4. Monitor with `getAllThreadStatus()` or `getActiveThreadCount()`
5. Control individual threads with `pauseThread()`, `resumeThread()`, `stopThread()`
6. Wait for completion with `waitForAll()`

## Project Structure

- **Headers**: `include/thread_framework/` - Public API headers
- **Examples**: `examples/` - Usage examples with Chinese comments
- **Tests**: `tests/` - Test files (currently empty in repo)
- **Build**: Root `Makefile` handles all build operations
- **Documentation**: README.md (Chinese) and examples/README.md (Chinese)

## Important Notes

### Thread Safety
- All framework components are thread-safe
- Custom workers must use `shouldContinue()` regularly to respond to pause/stop requests
- Use atomic operations or mutexes in custom workers for shared data

### Error Handling
- Workers should catch exceptions in their `run()` methods and call `onError()`
- Thread creation failures return `SIZE_MAX` as thread ID
- Framework logs thread lifecycle events to stdout

### Memory Management
- Framework uses RAII - no manual memory management needed
- Workers are managed via `std::unique_ptr`
- Automatic cleanup when ThreadManager is destroyed

### Performance Considerations
- Virtual function overhead is minimal compared to thread management costs
- Each worker runs in its own std::thread
- Consider thread pool pattern for large numbers of short-lived tasks