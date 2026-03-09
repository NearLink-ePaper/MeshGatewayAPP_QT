# ─── macOS 平台配置 ───────────────────────────────────────
# Qt Bluetooth 在 macOS 上通过 CoreBluetooth 后端工作

# ── App Icon（.icns 由 CI 从 icon.png 生成）──────────────
set(MACOS_ICON_FILE "${CMAKE_CURRENT_SOURCE_DIR}/MacOS/icon.icns")
if(EXISTS "${MACOS_ICON_FILE}")
    set(MACOS_ICON "${MACOS_ICON_FILE}" PARENT_SCOPE)
endif()

# 如需添加 CoreBluetooth 原生集成，在此添加源文件：
# list(APPEND PLATFORM_SOURCES
#     MacOS/src/macble.mm
#     MacOS/header/macble.h
# )
# include_directories(${CMAKE_CURRENT_SOURCE_DIR}/MacOS/header)
# target_link_libraries(NearLink_Mesh PRIVATE "-framework CoreBluetooth")
