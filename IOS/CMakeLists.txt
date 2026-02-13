# ─── iOS 平台配置 ─────────────────────────────────────────
# Qt Bluetooth 在 iOS 上通过 CoreBluetooth 后端工作

# ── 从模板生成 Info.plist（统一版本管理）──────────────────
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/IOS/Info.plist.in
    ${CMAKE_CURRENT_SOURCE_DIR}/IOS/Info.plist
    @ONLY
)

# 保存 plist 路径，在 target 创建后由根 CMakeLists.txt 应用
set(IOS_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/IOS/Info.plist")

# 如需添加 iOS 专用代码（如 Objective-C 桥接），在此添加源文件：
# list(APPEND PLATFORM_SOURCES
#     IOS/src/iosble.mm
# )
# include_directories(${CMAKE_CURRENT_SOURCE_DIR}/IOS/header)
