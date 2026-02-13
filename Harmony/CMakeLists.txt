# ─── OpenHarmony (鸿蒙) 平台配置 ─────────────────────────────
# 参考: https://wiki.qt.io/Qt_for_HarmonyOS/zh
#
# ═══════════════════════════════════════════════════════════════
#  前置条件 (在 Windows 上交叉编译)
# ═══════════════════════════════════════════════════════════════
#
# 1. 安装 DevEco Studio 并下载 HarmonyOS SDK (API 15+)
#    https://developer.huawei.com/consumer/cn/deveco-studio/
#
# 2. 从源码编译或下载预编译的 Qt 5.12.12 for OHOS
#    安装路径示例: C:\Qt\qt-5.12.12-ohos
#    (qmake 位于 C:\Qt\qt-5.12.12-ohos\bin\qmake)
#
# 3. 下载 llvm-mingw 工具链 (提供 mingw32-make)
#    https://github.com/mstorsjo/llvm-mingw/releases
#    解压到例如 D:\llvm-mingw-20230919-ucrt-x86_64
#
# 4. 安装 Strawberry Perl (构建 Qt 源码时需要)
#    https://strawberryperl.com/
#
# ═══════════════════════════════════════════════════════════════
#  必需的环境变量
# ═══════════════════════════════════════════════════════════════
#
#   NATIVE_OHOS_SDK    — HarmonyOS SDK native 目录
#     示例: D:\DevEcoStudio\sdk\HarmonyOS-NEXT-DB6\openharmony\native
#
#   OHOS_SDK_SYSROOT   — %NATIVE_OHOS_SDK%\sysroot
#
#   LLVM_INSTALL_DIR   — %NATIVE_OHOS_SDK%\llvm
#     (OHOS 自带的 clang 交叉编译器，用于 arm64-v8a)
#
#   QT_OHOS_DIR        — Qt 5.12.12 for OHOS 安装路径
#     示例: C:\Qt\qt-5.12.12-ohos
#
#   MINGW_ROOT         — llvm-mingw 工具链路径
#     示例: D:\llvm-mingw-20230919-ucrt-x86_64
#     (需将 %MINGW_ROOT%\bin 加入 PATH，提供 mingw32-make)
#
# ═══════════════════════════════════════════════════════════════
#  CMake 构建命令 (Windows CMD)
# ═══════════════════════════════════════════════════════════════
#
#   SET NATIVE_OHOS_SDK=D:\DevEcoStudio\sdk\HarmonyOS-NEXT-DB6\openharmony\native
#   SET OHOS_SDK_SYSROOT=%NATIVE_OHOS_SDK%\sysroot
#   SET LLVM_INSTALL_DIR=%NATIVE_OHOS_SDK%\llvm
#
#   cmake -B build -G "MinGW Makefiles" ^
#     -DCMAKE_C_COMPILER=%LLVM_INSTALL_DIR%\bin\clang.exe ^
#     -DCMAKE_CXX_COMPILER=%LLVM_INSTALL_DIR%\bin\clang++.exe ^
#     -DCMAKE_SYSROOT=%OHOS_SDK_SYSROOT% ^
#     -DCMAKE_PREFIX_PATH=C:\Qt\qt-5.12.12-ohos ^
#     -DCMAKE_FIND_ROOT_PATH=C:\Qt\qt-5.12.12-ohos ^
#     -DCMAKE_BUILD_TYPE=Release ^
#     -DOHOS=ON ^
#     -DOHOS_ARCH=arm64-v8a
#
#   mingw32-make -j%NUMBER_OF_PROCESSORS%
#
# ═══════════════════════════════════════════════════════════════
#  部署流程
# ═══════════════════════════════════════════════════════════════
#
#   1. 编译产出 libNearLink_Mesh.so
#   2. 复制 libNearLink_Mesh.so → <deveco-project>\entry\libs\arm64-v8a\
#   3. 复制 Qt 依赖库到同一目录:
#        copy %QT_OHOS_DIR%\lib\libQt5Core.so     → libs\arm64-v8a\
#        copy %QT_OHOS_DIR%\lib\libQt5Gui.so      → libs\arm64-v8a\
#        copy %QT_OHOS_DIR%\lib\libQt5Widgets.so   → libs\arm64-v8a\
#        copy %QT_OHOS_DIR%\lib\libQt5Bluetooth.so → libs\arm64-v8a\
#        (及其他依赖的 Qt 库)
#   4. 修改 <deveco-project>\entry\src\main\ets\common\QtAppConstants.ets
#        将 APP_LIBRARY_NAME 改为 "NearLink_Mesh"
#   5. DevEco Studio 签名并构建 .hap
#
# ─────────────────────────────────────────────────────────────

message(STATUS "Configuring for OpenHarmony (OHOS) platform")

# ── C++ 标准 ─────────────────────────────────────────────────
# Qt 5.12.12 for OHOS 使用 -c++std c++14 配置
# 若项目使用 C++17 特性在 ohos-clang 上编译失败，可降为 C++14:
# set(CMAKE_CXX_STANDARD 14 PARENT_SCOPE)

# ── OHOS SDK 检查 ────────────────────────────────────────────
if(DEFINED ENV{NATIVE_OHOS_SDK})
    set(NATIVE_OHOS_SDK $ENV{NATIVE_OHOS_SDK})
elseif(DEFINED NATIVE_OHOS_SDK)
    # 允许通过 -DNATIVE_OHOS_SDK=... 传入
else()
    message(WARNING "NATIVE_OHOS_SDK not set. "
        "Set it to HarmonyOS SDK native directory, e.g.:\n"
        "  D:\\DevEcoStudio\\sdk\\HarmonyOS-NEXT-DB6\\openharmony\\native")
endif()

if(DEFINED NATIVE_OHOS_SDK)
    message(STATUS "NATIVE_OHOS_SDK: ${NATIVE_OHOS_SDK}")

    # 派生变量
    if(NOT DEFINED OHOS_SDK_SYSROOT)
        set(OHOS_SDK_SYSROOT "${NATIVE_OHOS_SDK}/sysroot")
    endif()
    if(NOT DEFINED LLVM_INSTALL_DIR)
        set(LLVM_INSTALL_DIR "${NATIVE_OHOS_SDK}/llvm")
    endif()

    message(STATUS "OHOS_SDK_SYSROOT: ${OHOS_SDK_SYSROOT}")
    message(STATUS "LLVM_INSTALL_DIR: ${LLVM_INSTALL_DIR}")

    # 链接 OHOS 系统库路径
    link_directories("${OHOS_SDK_SYSROOT}/usr/lib/aarch64-linux-ohos")
endif()

# OHOS 平台宏
add_compile_definitions(OHOS_PLATFORM)

# ── OHOS 平台源文件 ──────────────────────────────────────────
# 鸿蒙平台优势：星闪 (NearLink) SLE 通信可直接调用原生 API，
# 无需通过 BLE 桥接，性能和功耗更优
#
# NearLink SLE 原生通信（鸿蒙专用，替代 BLE）
# 取消注释以启用：
# list(APPEND PLATFORM_SOURCES
#     Harmony/src/sle_manager.cpp
# )
# include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Harmony/header)

# ── OHOS 权限声明 (module.json5) ────────────────────────────
# HarmonyOS 应用权限在 module.json5 中声明：
#   "requestPermissions": [
#     { "name": "ohos.permission.ACCESS_BLUETOOTH" },
#     { "name": "ohos.permission.ACCESS_NEARLINK" }
#   ]

# ── 统一版本 (复用根 CMakeLists.txt 变量) ─────────────────────
# APP_VERSION_STR, APP_VERSION_CODE 已在根 CMakeLists.txt 定义
# 可在 OHOS module.json5 或 app.json5 中引用
