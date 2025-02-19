# 编译器和标志
CXX = g++
CXXFLAGS = -std=c++11 -O2 -Wall -g -I./src -I./third_party/leveldb/include
LDFLAGS = -lpthread -L./third_party/leveldb/build -lleveldb

# 目录设置
SRCDIR = src
TESTDIR = testnew
BUILDDIR = build
BINDIR = bin

# 源代码文件
SRC_FILES = $(wildcard $(SRCDIR)/**/*.cpp)  # 查找 src 目录下所有的.cpp 文件
TEST_SRCS = $(TESTDIR)/redisServer.cpp $(TESTDIR)/redisClient.cpp  # 新增 redisClient.cpp
OBJS = $(patsubst $(SRCDIR)/%.cpp, $(BUILDDIR)/%.o, $(SRC_FILES) ) # 将所有.cpp 文件转换为.o 文件
TEST_OBJS = $(patsubst $(TESTDIR)/%.cpp, $(BUILDDIR)/%.o, $(TEST_SRCS) ) # 编译测试文件的.o 文件
TARGETS = $(BINDIR)/redisServer $(BINDIR)/redisClient  # 新增 redisClient 可执行文件目标


# 创建目录
$(shell mkdir -p $(BUILDDIR))
$(shell mkdir -p $(BINDIR))
$(shell mkdir -p $(BUILDDIR)/Network)  # 确保 Network 子目录存在
$(shell mkdir -p $(BUILDDIR)/Poller)  # 确保 Poller 子目录存在
$(shell mkdir -p $(BUILDDIR)/Redis)  # 确保 Redis 子目录存在
$(shell mkdir -p $(BUILDDIR)/Thread)  # 确保 Thread 子目录存在
$(shell mkdir -p $(BUILDDIR)/Util)  # 确保 Util 子目录存在


# 默认目标
all: $(TARGETS)

# 编译并链接测试文件和源码
$(BINDIR)/redisServer: $(BUILDDIR)/redisServer.o $(OBJS)
	$(CXX) $^ -o $@ $(LDFLAGS)

# 新增：编译并链接 redisClient 可执行文件
$(BINDIR)/redisClient: $(BUILDDIR)/redisClient.o $(OBJS)
	$(CXX) $^ -o $@ $(LDFLAGS)


# 编译源代码文件
$(BUILDDIR)/%.o: $(SRCDIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# 编译测试文件
$(BUILDDIR)/%.o: $(TESTDIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@


# 清理构建文件
clean:
	find $(BUILDDIR) -name "*.o" -type f -exec rm -f {} +
	rm -rf $(BINDIR)


# 为 src 目录下的每个.cpp 文件添加头文件依赖
$(BUILDDIR)/%.o: $(SRCDIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@ -MD -MP -MF $(@:.o=.d)


# 包含生成的.d 文件
-include $(OBJS:.o=.d)


# 伪目标
.PHONY: all clean