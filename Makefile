# 线程框架 Makefile
# Author: Thread Framework Team
# Date: 2024

# 编译器和标志
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Wpedantic -O2
DEBUG_FLAGS = -std=c++17 -Wall -Wextra -Wpedantic -g -DDEBUG
INCLUDES = -Iinclude
LDFLAGS = -pthread

# 目录
SRC_DIR = src
INCLUDE_DIR = include
BIN_DIR = bin
EXAMPLE_DIR = examples
TEST_DIR = tests
BUILD_DIR = build

# 文件
EXAMPLE_SOURCES = $(wildcard $(EXAMPLE_DIR)/*.cpp)
TEST_SOURCES = $(wildcard $(TEST_DIR)/*.cpp)

# 示例目标
EXAMPLE_TARGETS = $(EXAMPLE_SOURCES:$(EXAMPLE_DIR)/%.cpp=$(BIN_DIR)/%)

# 默认目标
.PHONY: all examples tests clean debug help

all: directories examples

# 创建必要目录
directories:
	@mkdir -p $(BIN_DIR) $(BUILD_DIR) $(BUILD_DIR)/examples

# 构建所有示例
examples: directories $(EXAMPLE_TARGETS)

# 构建单个示例
$(BIN_DIR)/%: $(EXAMPLE_DIR)/%.cpp
	@echo "Building example: $@"
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< $(LDFLAGS) -o $@

# 构建调试版本
debug: CXXFLAGS = $(DEBUG_FLAGS)
debug: clean examples

# 运行基础示例
run-basic: examples
	@echo "Running basic usage example..."
	./$(BIN_DIR)/basic_usage

# 运行自定义工作者示例
run-custom: examples
	@echo "Running custom worker example..."
	./$(BIN_DIR)/custom_worker

# 运行所有示例
run-examples: examples
	@echo "Running all examples..."
	@for example in $(BIN_DIR)/*; do \
		if [ -f "$$example" ]; then \
			echo "Running $$example..."; \
			$$example; \
			echo "---"; \
		fi; \
	done

# 构建测试
tests: directories $(TEST_SOURCES)
	@echo "Building tests..."
	@for test_src in $(TEST_SOURCES); do \
		test_name=$$(basename $$test_src .cpp); \
		echo "Building test: $$test_name"; \
		$(CXX) $(CXXFLAGS) $(INCLUDES) $$test_src $(LDFLAGS) -o $(BIN_DIR)/$$test_name; \
	done

# 运行测试
run-tests: tests
	@echo "Running tests..."
	@for test_bin in $(BIN_DIR)/test_*; do \
		if [ -f "$$test_bin" ]; then \
			echo "Running $$test_bin..."; \
			$$test_bin; \
		fi; \
	done

# 清理构建文件
clean:
	@echo "Cleaning build artifacts..."
	rm -rf $(BUILD_DIR) $(BIN_DIR)

# 安装框架头文件到系统目录（可选）
install: all
	@echo "Installing thread framework..."
	@mkdir -p /usr/local/include/thread_framework
	@cp -r $(INCLUDE_DIR)/thread_framework/* /usr/local/include/thread_framework/
	@echo "Framework installed to /usr/local/include/thread_framework/"

# 卸载框架（可选）
uninstall:
	@echo "Uninstalling thread framework..."
	@rm -rf /usr/local/include/thread_framework
	@echo "Framework uninstalled."

# 创建发布包
package: clean
	@echo "Creating package..."
	@tar -czf thread_framework.tar.gz \
		--exclude='*.tar.gz' \
		--exclude='bin' \
		--exclude='build' \
		--exclude='.git' \
		.
	@echo "Package created: thread_framework.tar.gz"

# 显示帮助
help:
	@echo "Thread Framework Makefile"
	@echo "========================"
	@echo ""
	@echo "Available targets:"
	@echo "  all           - Build framework and examples (default)"
	@echo "  examples      - Build all examples"
	@echo "  tests         - Build tests"
	@echo "  run-basic     - Run basic usage example"
	@echo "  run-custom    - Run custom worker example"
	@echo "  run-examples  - Run all examples"
	@echo "  run-tests     - Run all tests"
	@echo "  debug         - Build debug version"
	@echo "  clean         - Remove build artifacts"
	@echo "  install       - Install framework to system"
	@echo "  uninstall     - Remove framework from system"
	@echo "  package       - Create distribution package"
	@echo "  help          - Show this help"
	@echo ""
	@echo "Variables:"
	@echo "  CXX           - C++ compiler ($(CXX))"
	@echo "  CXXFLAGS      - Compiler flags ($(CXXFLAGS))"
	@echo "  INCLUDES      - Include directories ($(INCLUDES))"
	@echo "  LDFLAGS       - Linker flags ($(LDFLAGS))"
	@echo ""
	@echo "Usage examples:"
	@echo "  make                    # Build everything"
	@echo "  make examples           # Build examples only"
	@echo "  make run-basic          # Build and run basic example"
	@echo "  make debug CXXFLAGS='-g -O0'  # Build with debug info"

# 显示框架信息
info:
	@echo "Thread Framework Information"
	@echo "==========================="
	@echo "Version: 1.0.0"
	@echo "C++ Standard: C++17"
	@echo "Required Libraries: pthread"
	@echo "Include Directory: $(INCLUDE_DIR)"
	@echo "Source Directory: $(SRC_DIR)"
	@echo "Example Directory: $(EXAMPLE_DIR)"
	@echo "Build Directory: $(BUILD_DIR)"
	@echo "Binary Directory: $(BIN_DIR)"

# 检查依赖
check-deps:
	@echo "Checking dependencies..."
	@which $(CXX) > /dev/null 2>&1 && echo "✓ $(CXX) found" || echo "✗ $(CXX) not found"
	@which make > /dev/null 2>&1 && echo "✓ make found" || echo "✗ make not found"
	@echo "Checking for pthread support..."
	@echo '#include <pthread.h>' | $(CXX) -x c++ - -c -o /dev/null 2>/dev/null && echo "✓ pthread supported" || echo "✗ pthread not supported"

# 快速验证安装
verify: examples
	@echo "Verifying framework installation..."
	@echo "Testing basic compilation..."
	@echo '#include "thread_framework/ThreadManager.h"' | $(CXX) -x c++ $(INCLUDES) -c -o /dev/null - && echo "✓ Headers compile correctly" || echo "✗ Header compilation failed"
	@echo "Running quick test..."
	@echo 'int main() { return 0; }' | $(CXX) -x c++ $(INCLUDES) $(LDFLAGS) -o $(BUILD_DIR)/test - && $(BUILD_DIR)/test && echo "✓ Linking works" || echo "✗ Linking failed"
	@rm -f $(BUILD_DIR)/test