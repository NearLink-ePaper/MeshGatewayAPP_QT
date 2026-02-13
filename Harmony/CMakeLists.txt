# ─── OpenHarmony (鸿蒙) 平台配置 ─────────────────────────────
# 参考: https://wiki.qt.io/Qt_for_OpenHarmony/zh
#
# 构建要求:
#   - DevEco Studio (HarmonyOS NEXT, API 12+)
#   - OHOS SDK: NATIVE_OHOS_SDK 环境变量指向 native 目录
#   - Qt for OpenHarmony (当前 Qt 5.12.12 / 5.15.16, 待 Qt 6 适配)
#   - 交叉编译平台: ohos-clang (arm64-v8a)
#
# 环境变量:
#   OHOS_SDK_ROOT    — OpenHarmony SDK 根目录
#   NATIVE_OHOS_SDK  — ${OHOS_SDK_ROOT}/<api_ver>/native
#   LLVM_INSTALL_DIR — ${NATIVE_OHOS_SDK}/llvm
#
# 鸿蒙平台优势:
#   星闪 (NearLink) SLE 通信可直接调用原生 API，
#   无需通过 BLE 桥接，性能和功耗更优

# ── OHOS 平台源文件 ──────────────────────────────────────────
# 当 OHOS 适配就绪后，取消注释以下内容：

# NearLink SLE 原生通信（鸿蒙专用，替代 BLE）
# list(APPEND PLATFORM_SOURCES
#     Harmony/src/ohosble.cpp
# )
# list(APPEND PLATFORM_HEADERS
#     Harmony/header/ohosble.h
# )
# include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Harmony/header)

# ── OHOS 权限与配置 ──────────────────────────────────────────
# HarmonyOS 应用权限在 module.json5 中声明：
#   "requestPermissions": [
#     { "name": "ohos.permission.ACCESS_BLUETOOTH" },
#     { "name": "ohos.permission.ACCESS_NEARLINK" }
#   ]
#
# DevEco 项目模板用于最终打包 .hap：
#   1. Qt 编译产出 .so 库
#   2. 复制 .so 到 <deveco-project>/entry/libs/arm64-v8a/
#   3. 复制 Qt 依赖库到同一目录
#   4. DevEco Studio 构建签名并生成 .hap

# ── 统一版本 (复用根 CMakeLists.txt 变量) ─────────────────────
# APP_VERSION_STR, APP_VERSION_CODE 已在根 CMakeLists.txt 定义
# 可在 OHOS module.json5 或 app.json5 中引用
