# Debian/Linux 平台特定源文件
# Qt Bluetooth 在 Linux 上通过 BlueZ D-Bus 后端工作
# 如需添加 Linux 专用代码（如 D-Bus 直接交互），在此添加源文件：
#
# list(APPEND PLATFORM_SOURCES
#     Debian/src/linuxble.cpp
#     Debian/header/linuxble.h
# )
# include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Debian/header)

# Linux 可能需要额外的 D-Bus 依赖
# find_package(Qt${QT_VERSION_MAJOR} COMPONENTS DBus)
# target_link_libraries(NearLink_Mesh PRIVATE Qt${QT_VERSION_MAJOR}::DBus)
