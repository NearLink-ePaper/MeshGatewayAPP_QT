# macOS 平台特定源文件
# 当前使用 Qt Bluetooth 共享实现，暂无 macOS 专用代码
# 如需添加 CoreBluetooth 原生集成，在此添加源文件：
#
# list(APPEND PLATFORM_SOURCES
#     MacOS/src/macble.mm
#     MacOS/header/macble.h
# )
# include_directories(${CMAKE_CURRENT_SOURCE_DIR}/MacOS/header)
# target_link_libraries(NearLink_Mesh PRIVATE "-framework CoreBluetooth")
