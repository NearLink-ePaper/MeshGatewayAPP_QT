# ─── OpenHarmony (鸿蒙) 平台配置 ─────────────────────────────
# 参考: https://wiki.qt.io/Qt_for_OpenHarmony/zh
#
# 构建要求:
#   - DevEco Studio (HarmonyOS NEXT, API 12+)
#   - OHOS SDK: NATIVE_OHOS_SDK 环境变量指向 native 目录
#   - Qt for OpenHarmony: Qt 5.12.12 / 5.15.16（向下兼容 5.12.12）
#   - 交叉编译工具链: ohos-clang (arm64-v8a)
#
# 环境变量:
#   OHOS_SDK_ROOT    — OpenHarmony SDK 根目录
#   NATIVE_OHOS_SDK  — ${OHOS_SDK_ROOT}/<api_ver>/native
#   LLVM_INSTALL_DIR — ${NATIVE_OHOS_SDK}/llvm
#   QT_OHOS_DIR      — Qt for OHOS 安装路径 (如 /opt/Qt/5.12.12/ohos-arm64-v8a-clang)
#
# 鸿蒙平台优势:
#   星闪 (NearLink) SLE 通信可直接调用原生 API，
#   无需通过 BLE 桥接，性能和功耗更优
# ─────────────────────────────────────────────────────────────

message(STATUS "Configuring for OpenHarmony (OHOS) platform")

# ── C++ 标准兼容 ─────────────────────────────────────────────
# Qt 5.12 最低要求 C++11，项目使用 C++17；ohos-clang 支持 C++17
# 如遇编译问题可降为 C++14:
# set(CMAKE_CXX_STANDARD 14 PARENT_SCOPE)

# ── OHOS SDK 检查 ────────────────────────────────────────────
if(DEFINED ENV{NATIVE_OHOS_SDK})
    set(NATIVE_OHOS_SDK $ENV{NATIVE_OHOS_SDK})
    message(STATUS "NATIVE_OHOS_SDK: ${NATIVE_OHOS_SDK}")
elseif(DEFINED ENV{OHOS_SDK_ROOT})
    # 自动定位最新 API 版本
    file(GLOB _api_dirs "$ENV{OHOS_SDK_ROOT}/*/native")
    list(SORT _api_dirs ORDER DESCENDING)
    list(GET _api_dirs 0 NATIVE_OHOS_SDK)
    message(STATUS "Auto-detected NATIVE_OHOS_SDK: ${NATIVE_OHOS_SDK}")
else()
    message(WARNING "OHOS SDK not found. Set NATIVE_OHOS_SDK or OHOS_SDK_ROOT environment variable.")
endif()

# ── 编译选项 ─────────────────────────────────────────────────
if(DEFINED NATIVE_OHOS_SDK)
    # OHOS sysroot
    if(NOT DEFINED CMAKE_SYSROOT)
        set(CMAKE_SYSROOT "${NATIVE_OHOS_SDK}/sysroot" PARENT_SCOPE)
    endif()

    # 链接 OHOS 系统库路径
    link_directories("${NATIVE_OHOS_SDK}/sysroot/usr/lib/aarch64-linux-ohos")
endif()

# OHOS 平台宏
add_compile_definitions(OHOS_PLATFORM)

# ── OHOS 平台源文件 ──────────────────────────────────────────
# NearLink SLE 原生通信（鸿蒙专用，替代 BLE）
# 取消注释以启用：
# list(APPEND PLATFORM_SOURCES
#     Harmony/src/sle_manager.cpp
# )
# include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Harmony/header)

# ── OHOS 权限与配置 ──────────────────────────────────────────
# HarmonyOS 应用权限在 module.json5 中声明：
#   "requestPermissions": [
#     { "name": "ohos.permission.ACCESS_BLUETOOTH" },
#     { "name": "ohos.permission.ACCESS_NEARLINK" }
#   ]
#
# DevEco 打包流程:
#   1. CMake 交叉编译产出 libNearLink_Mesh.so
#   2. 复制 .so 到 <deveco-project>/entry/libs/arm64-v8a/
#   3. 复制 Qt 依赖库到同一目录
#   4. DevEco Studio 构建签名并生成 .hap
#
# 构建命令示例:
#   cmake -B build \
#     -DCMAKE_TOOLCHAIN_FILE=${NATIVE_OHOS_SDK}/build/cmake/ohos.toolchain.cmake \
#     -DCMAKE_PREFIX_PATH=${QT_OHOS_DIR} \
#     -DOHOS_ARCH=arm64-v8a \
#     -DCMAKE_BUILD_TYPE=Release
#   cmake --build build --parallel

# ── 统一版本 (复用根 CMakeLists.txt 变量) ─────────────────────
# APP_VERSION_STR, APP_VERSION_CODE 已在根 CMakeLists.txt 定义
# 可在 OHOS module.json5 或 app.json5 中引用
