# OpenHarmony (鸿蒙) 平台特定源文件
# Qt 尚未官方支持 OpenHarmony，此目录用于存放鸿蒙专用适配代码
# 星闪 (NearLink) SLE 通信在鸿蒙系统上可使用原生 NearLink API
#
# 当鸿蒙 Qt 适配可用时，在此添加源文件：
# list(APPEND PLATFORM_SOURCES
#     Harmony/src/harmonyble.cpp
#     Harmony/header/harmonyble.h
# )
# include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Harmony/header)
#
# 注意: 鸿蒙平台的星闪近场通信 (NearLink SLE) 可直接调用，
#       无需通过 BLE 桥接，性能和功耗更优
