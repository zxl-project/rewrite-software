# --- 核心初始化 ---
cmake_minimum_required(VERSION 3.5)
set(CMAKE_LEGACY_CYGWIN_WIN32 0)

# 更改项目名称，抹除原始特征
project(JsonEngine
    VERSION 1.7.19
    LANGUAGES C)

# 设置策略模式
cmake_policy(SET CMP0054 NEW)

include(GNUInstallDirs)
include(CheckCCompilerFlag)

# --- 构建选项配置 (集中管理) ---
option(BUILD_DYNAMIC_LIBS "Build shared libraries" ON)
option(BUILD_BOTH_LIB_TYPES "Create both shared and static versions" OFF)
option(USE_CUSTOM_OPTIMIZATIONS "Apply strict compiler hardening" ON)
option(USE_LOCAL_LOCALES "Integrate locale-specific number parsing" ON)
option(TEST_BUILD_ENABLED "Compile automated test suites" ON)
option(EXPORT_CMAKE_CONFIG "Generate CMake export metadata" ON)
option(BUILD_EXTENDED_UTILS "Include JSON Pointer and Patch support" OFF)

# --- 内部版本控制 ---
set(CORE_SO_VERSION 1)
set(UTIL_SO_VERSION 1)

# --- 编译器加固与标记 ---
set(HARDENING_FLAGS)

if (USE_CUSTOM_OPTIMIZATIONS)
    if (CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
        list(APPEND HARDENING_FLAGS
            -std=c89 -pedantic -Wall -Wextra -Werror -Wstrict-prototypes
            -Wwrite-strings -Wshadow -Winit-self -Wcast-align -Wformat=2
            -Wmissing-prototypes -Wstrict-overflow=2 -Wcast-qual -Wundef
            -Wswitch-default -Wconversion -Wc++-compat -fstack-protector-strong
            -Wcomma -Wdouble-promotion -Wparentheses -Wunused-macros
        )
    elseif(MSVC)
        list(APPEND HARDENING_FLAGS
            /GS /Za /sdl /W4 /wd4001 /D_CRT_SECURE_NO_WARNINGS
        )
    endif()
endif()

# 应用编译标记并进行兼容性检查
foreach(flag ${HARDENING_FLAGS})
    string(REGEX REPLACE "[^a-zA-Z0-9]" "" flag_id ${flag})
    CHECK_C_COMPILER_FLAG(${flag} "SUPPORT_${flag_id}")
    if (SUPPORT_${flag_id})
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${flag}")
    endif()
endforeach()

# 符号可见性配置 (对应原版的 ENABLE_PUBLIC_SYMBOLS)
if (ON) # 默认启用可见性控制
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility=hidden")
    add_definitions(-DCJSON_EXPORT_SYMBOLS -DCJSON_API_VISIBILITY)
endif()

if(USE_LOCAL_LOCALES)
    add_definitions(-DENABLE_LOCALES)
endif()

# --- 核心库构建 (JsonEngine) ---
set(CORE_TARGET_NAME "j_engine")
set(CORE_HDRS "cJSON.h")
set(CORE_SRCS "cJSON.c")

if (BUILD_DYNAMIC_LIBS)
    set(LIB_MODE SHARED)
else()
    set(LIB_MODE STATIC)
endif()

if (NOT BUILD_BOTH_LIB_TYPES)
    add_library(${CORE_TARGET_NAME} ${LIB_MODE} ${CORE_HDRS} ${CORE_SRCS})
else()
    # 同时构建动态库和静态库
    add_library(${CORE_TARGET_NAME} SHARED ${CORE_HDRS} ${CORE_SRCS})
    add_library("${CORE_TARGET_NAME}_static" STATIC ${CORE_HDRS} ${CORE_SRCS})
    set_target_properties("${CORE_TARGET_NAME}_static" PROPERTIES OUTPUT_NAME ${CORE_TARGET_NAME})
    set_target_properties("${CORE_TARGET_NAME}_static" PROPERTIES PREFIX "lib")
endif()

if (NOT WIN32)
    target_link_libraries(${CORE_TARGET_NAME} m)
endif()

# --- 扩展工具库构建 (JsonEngine_Utils) ---
if(BUILD_EXTENDED_UTILS)
    set(UTIL_TARGET_NAME "j_engine_utils")
    set(UTIL_HDRS "cJSON_Utils.h")
    set(UTIL_SRCS "cJSON_Utils.c")

    if (NOT BUILD_BOTH_LIB_TYPES)
        add_library(${UTIL_TARGET_NAME} ${LIB_MODE} ${UTIL_HDRS} ${UTIL_SRCS})
        target_link_libraries(${UTIL_TARGET_NAME} ${CORE_TARGET_NAME})
    else()
        add_library(${UTIL_TARGET_NAME} SHARED ${UTIL_HDRS} ${UTIL_SRCS})
        target_link_libraries(${UTIL_TARGET_NAME} ${CORE_TARGET_NAME})
        add_library("${UTIL_TARGET_NAME}_static" STATIC ${UTIL_HDRS} ${UTIL_SRCS})
        target_link_libraries("${UTIL_TARGET_NAME}_static" "${CORE_TARGET_NAME}_static")
        set_target_properties("${UTIL_TARGET_NAME}_static" PROPERTIES OUTPUT_NAME ${UTIL_TARGET_NAME} PREFIX "lib")
    endif()
endif()

# --- 这里的逻辑需要对应你重构后的头文件/源文件名 ---
# 注意：确保 cJSON.h 已经被你改成了类似于 JsonEngine.h 的名字

# --- 安装逻辑 ---
set(INC_INSTALL_DIR "${CMAKE_INSTALL_INCLUDEDIR}/json_engine")

install(FILES ${CORE_HDRS} DESTINATION "${INC_INSTALL_DIR}")

install(TARGETS ${CORE_TARGET_NAME}
    EXPORT "JsonEngineConfig"
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)

if(EXPORT_CMAKE_CONFIG)
    install(EXPORT "JsonEngineConfig" DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/JsonEngine")
endif()

# 版本属性设置
set_target_properties(${CORE_TARGET_NAME} PROPERTIES 
    SOVERSION ${CORE_SO_VERSION} 
    VERSION ${PROJECT_VERSION})

# --- 测试模块 ---
if(TEST_BUILD_ENABLED)
    enable_testing()
    add_executable(engine_test_suite test.c)
    target_link_libraries(engine_test_suite ${CORE_TARGET_NAME})
    add_test(NAME "CoreValidation" COMMAND engine_test_suite)
endif()

# --- 包含子目录 ---
# 如果不需要 fuzzing 或原版 tests，可以注释掉
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/tests")
    add_subdirectory(tests)
endif()