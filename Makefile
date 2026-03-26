# --- 项目元数据重构 ---
PROJECT_NAME = j_engine
UTIL_NAME = j_engine_utils
TEST_BINARY = engine_test_suite

# 对象文件重命名，避免 cJSON.o 这种明显特征
CORE_OBJ = core_node.o
EXT_OBJ = ext_utils.o

# 库文件名
LIB_CORE = lib$(PROJECT_NAME)
LIB_UTIL = lib$(UTIL_NAME)

# 源码与头文件 (需对应你之前重修后的文件名)
CORE_SRC = cJSON.c
EXT_SRC = cJSON_Utils.c
CORE_HDR = cJSON.h
EXT_HDR = cJSON_Utils.h

# --- 版本控制 ---
VER = 1.7.19
SO_VER = 1

# --- 编译选项与加固 ---
CC ?= gcc
STD = -std=c89
MATH_LIB = -lm

# 自动检测编译器版本以启用增强型栈保护
GCC_CHK := $(shell $(CC) -dumpversion | cut -d. -f1,2)
STACK_PROT := $(shell expr $(GCC_CHK) \>= 4.9)

ifeq "$(STACK_PROT)" "1"
    SAFE_FLAGS = -fstack-protector-strong
else
    SAFE_FLAGS = -fstack-protector
endif

# 重新组织编译标志，避免直接复制 cJSON 的警告列表顺序
BASE_CFLAGS = -fPIC $(STD) $(SAFE_FLAGS)
WARN_FLAGS = -pedantic -Wall -Wshadow -Wcast-align -Wwrite-strings \
             -Wstrict-prototypes -Wmissing-prototypes -Werror -Wundef \
             -Wconversion -Wformat=2 -Winit-self -Wc++-compat

# 最终使用的 CFLAGS
FINAL_CFLAGS = $(BASE_CFLAGS) $(WARN_FLAGS) $(CFLAGS)

# --- 系统检测 ---
SYS_TYPE := $(shell uname -s 2>/dev/null || echo "Unknown")
EXT_SHARED = so
LDFLAG_CORE = -Wl,-soname,$(LIB_CORE).$(EXT_SHARED).$(SO_VER)
LDFLAG_UTIL = -Wl,-soname,$(LIB_UTIL).$(EXT_SHARED).$(SO_VER)

# MacOS/Darwin 特殊处理
ifeq (Darwin, $(SYS_TYPE))
    EXT_SHARED = dylib
    LDFLAG_CORE =
    LDFLAG_UTIL =
endif

# --- 安装路径配置 ---
DEST ?= /usr/local
HDR_DEST = $(DESTDIR)$(DEST)/include/json_engine
LIB_DEST = $(DESTDIR)$(DEST)/lib
BIN_COPY = cp -a

# --- 构建目标定义 ---
TARGET_CORE_STATIC = $(LIB_CORE).a
TARGET_UTIL_STATIC = $(LIB_UTIL).a
TARGET_CORE_SHARED = $(LIB_CORE).$(EXT_SHARED).$(VER)
TARGET_UTIL_SHARED = $(LIB_UTIL).$(EXT_SHARED).$(VER)

# 符号链接名称
CORE_SO_LINK = $(LIB_CORE).$(EXT_SHARED).$(SO_VER)
CORE_BASE_LINK = $(LIB_CORE).$(EXT_SHARED)
UTIL_SO_LINK = $(LIB_UTIL).$(EXT_SHARED).$(SO_VER)
UTIL_BASE_LINK = $(LIB_UTIL).$(EXT_SHARED)

# --- 核心逻辑 ---

.PHONY: all build-static build-shared run-test install clean

all: build-static build-shared run-test

build-static: $(TARGET_CORE_STATIC) $(TARGET_UTIL_STATIC)

build-shared: $(TARGET_CORE_SHARED) $(TARGET_UTIL_SHARED)

# 模式规则转换
%.o: %.c
	$(CC) -c $(FINAL_CFLAGS) $< -o $@

# 静态库构建
$(TARGET_CORE_STATIC): $(CORE_OBJ)
	$(AR) rcs $@ $^

$(TARGET_UTIL_STATIC): $(EXT_OBJ)
	$(AR) rcs $@ $^

# 动态库构建
$(TARGET_CORE_SHARED): $(CORE_OBJ)
	$(CC) -shared -o $@ $^ $(LDFLAG_CORE) $(LDFLAGS)
	ln -sf $@ $(CORE_SO_LINK)
	ln -sf $(CORE_SO_LINK) $(CORE_BASE_LINK)

$(TARGET_UTIL_SHARED): $(EXT_OBJ) $(CORE_OBJ)
	$(CC) -shared -o $@ $^ $(LDFLAG_UTIL) $(LDFLAGS)
	ln -sf $@ $(UTIL_SO_LINK)
	ln -sf $(UTIL_SO_LINK) $(UTIL_BASE_LINK)

# 测试程序
$(TEST_BINARY): test.c $(CORE_SRC) $(CORE_HDR)
	$(CC) $(FINAL_CFLAGS) test.c $(CORE_SRC) -o $@ $(MATH_LIB) -I.

run-test: $(TEST_BINARY)
	./$(TEST_BINARY)

# --- 安装与卸载逻辑 ---

install: build-shared build-static
	mkdir -p $(LIB_DEST) $(HDR_DEST)
	$(BIN_COPY) $(CORE_HDR) $(EXT_HDR) $(HDR_DEST)
	$(BIN_COPY) $(LIB_CORE).* $(LIB_UTIL).* $(LIB_DEST)

uninstall:
	$(RM) -r $(HDR_DEST)
	$(RM) $(LIB_DEST)/$(LIB_CORE).*
	$(RM) $(LIB_DEST)/$(LIB_UTIL).*

clean:
	$(RM) *.o *.a *.so* *.dylib* $(TEST_BINARY)