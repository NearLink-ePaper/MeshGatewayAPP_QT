# ─── iOS 平台配置 ─────────────────────────────────────────
# Qt Bluetooth 在 iOS 上通过 CoreBluetooth 后端工作

# ── 从模板生成 Info.plist（统一版本管理）──────────────────
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/IOS/Info.plist.in
    ${CMAKE_CURRENT_SOURCE_DIR}/IOS/Info.plist
    @ONLY
)

# 保存路径，在 target 创建后由根 CMakeLists.txt 应用
set(IOS_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/IOS/Info.plist")

# ── LaunchScreen（灵动岛 / 全面屏适配必需）────────────────
set(IOS_LAUNCH_STORYBOARD "${CMAKE_CURRENT_SOURCE_DIR}/IOS/LaunchScreen.storyboard")

# ── AppIcon（Asset Catalog）──────────────────────────────
set(IOS_ASSET_CATALOG "${CMAKE_CURRENT_SOURCE_DIR}/IOS/Assets.xcassets")

# ── App 名称本地化（英文 / 简体中文）─────────────────────
set(IOS_LPROJ_EN "${CMAKE_CURRENT_SOURCE_DIR}/IOS/en.lproj/InfoPlist.strings")
set(IOS_LPROJ_ZH "${CMAKE_CURRENT_SOURCE_DIR}/IOS/zh-Hans.lproj/InfoPlist.strings")

# 如需添加 iOS 专用代码（如 Objective-C 桥接），在此添加源文件：
# list(APPEND PLATFORM_SOURCES
#     IOS/src/iosble.mm
# )
# include_directories(${CMAKE_CURRENT_SOURCE_DIR}/IOS/header)
