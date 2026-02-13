cmake_minimum_required(VERSION 3.16)

project(NearLink_Mesh VERSION 1.0.0 LANGUAGES CXX)

# ─── 统一版本管理 ────────────────────────────────────
# 修改版本只需改 project(VERSION x.y.z)，自动传播到所有平台
set(APP_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(APP_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(APP_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(APP_VERSION_STR   "${PROJECT_VERSION}")
# Android versionCode: MAJOR*10000 + MINOR*100 + PATCH
math(EXPR APP_VERSION_CODE "${APP_VERSION_MAJOR} * 10000 + ${APP_VERSION_MINOR} * 100 + ${APP_VERSION_PATCH}")

message(STATUS "NearLink Mesh v${APP_VERSION_STR} (code: ${APP_VERSION_CODE})")

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ─── Qt 依赖 ────────────────────────────────────────
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Bluetooth)

# ─── 头文件搜索路径 ──────────────────────────────────────
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/Qt/header/core
    ${CMAKE_CURRENT_SOURCE_DIR}/Qt/header/ble
    ${CMAKE_CURRENT_SOURCE_DIR}/Qt/header/widget
    ${CMAKE_CURRENT_SOURCE_DIR}/Qt/header/utils
)

# ─── 翻译文件编译 (.ts → .qm) ────────────────────────────
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS LinguistTools QUIET)
set(TS_FILE ${CMAKE_CURRENT_SOURCE_DIR}/Qt/resource/translation/NearLink_Mesh_zh_CN.ts)
set(QM_FILE ${CMAKE_CURRENT_SOURCE_DIR}/Qt/resource/translation/NearLink_Mesh_zh_CN.qm)

if(Qt${QT_VERSION_MAJOR}LinguistTools_FOUND)
    # 使用 Qt 自带的 lrelease 编译翻译文件
    add_custom_command(
        OUTPUT  ${QM_FILE}
        COMMAND Qt${QT_VERSION_MAJOR}::lrelease ${TS_FILE} -qm ${QM_FILE}
        DEPENDS ${TS_FILE}
        COMMENT "Compiling translation: ${TS_FILE} -> .qm"
        VERBATIM
    )
    add_custom_target(translations ALL DEPENDS ${QM_FILE})
else()
    message(WARNING "Qt LinguistTools not found, skipping .ts -> .qm compilation. Pre-compiled .qm required.")
endif()

# ─── 资源文件 ────────────────────────────────────────────
set(RESOURCE_FILES
    Qt/resource/resources.qrc
)

# ─── 共享源文件 (跨平台) ──────────────────────────────────
set(SHARED_HEADERS
    Qt/header/core/meshprotocol.h
    Qt/header/ble/blemanager.h
    Qt/header/widget/mainwindow.h
    Qt/header/widget/scanpage.h
    Qt/header/widget/connectedpage.h
    Qt/header/widget/senddialog.h
    Qt/header/utils/stylemanager.h
)

set(SHARED_SOURCES
    main.cpp
    Qt/src/core/meshprotocol.cpp
    Qt/src/ble/blemanager.cpp
    Qt/src/widget/mainwindow.cpp
    Qt/src/widget/scanpage.cpp
    Qt/src/widget/connectedpage.cpp
    Qt/src/widget/senddialog.cpp
    Qt/src/utils/stylemanager.cpp
)

# ─── 平台特定源文件 ──────────────────────────────────────
set(PLATFORM_SOURCES "")

if(WIN32)
    # Windows 平台特定代码
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Windows/CMakeLists.txt")
        include(Windows/CMakeLists.txt)
    endif()
elseif(APPLE)
    if(IOS)
        # iOS 平台特定代码
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/IOS/CMakeLists.txt")
            include(IOS/CMakeLists.txt)
        endif()
    else()
        # macOS 平台特定代码
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/MacOS/CMakeLists.txt")
            include(MacOS/CMakeLists.txt)
        endif()
    endif()
elseif(ANDROID)
    # Android 平台特定代码
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Android/CMakeLists.txt")
        include(Android/CMakeLists.txt)
    endif()
elseif(OHOS OR CMAKE_SYSTEM_NAME STREQUAL "OHOS")
    # OpenHarmony (鸿蒙) 平台特定代码
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Harmony/CMakeLists.txt")
        include(Harmony/CMakeLists.txt)
    endif()
elseif(UNIX)
    # Debian/Linux 平台特定代码
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Debian/CMakeLists.txt")
        include(Debian/CMakeLists.txt)
    endif()
endif()

# ─── 合并所有源文件 ──────────────────────────────────────
set(PROJECT_SOURCES
    ${SHARED_HEADERS}
    ${SHARED_SOURCES}
    ${PLATFORM_SOURCES}
    ${RESOURCE_FILES}
)

# ─── 构建目标 ────────────────────────────────────────────
if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(NearLink_Mesh
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
else()
    if(ANDROID)
        add_library(NearLink_Mesh SHARED
            ${PROJECT_SOURCES}
        )
    else()
        add_executable(NearLink_Mesh
            ${PROJECT_SOURCES}
        )
    endif()

endif()

# ─── 翻译依赖（确保 .qm 在 rcc 之前生成）─────────────────
if(TARGET translations)
    add_dependencies(NearLink_Mesh translations)
endif()

# ─── 链接库 ──────────────────────────────────────────────
target_link_libraries(NearLink_Mesh PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Bluetooth
)

# ─── 版本宏注入 C++ ──────────────────────────────────────
target_compile_definitions(NearLink_Mesh PRIVATE
    APP_VERSION_MAJOR=${APP_VERSION_MAJOR}
    APP_VERSION_MINOR=${APP_VERSION_MINOR}
    APP_VERSION_PATCH=${APP_VERSION_PATCH}
    APP_VERSION_STR="${APP_VERSION_STR}"
    APP_VERSION_CODE=${APP_VERSION_CODE}
)

# ─── Android 特定属性 ────────────────────────────────────
if(ANDROID)
    set_property(TARGET NearLink_Mesh APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
                 ${CMAKE_CURRENT_SOURCE_DIR}/Android)
endif()

# ─── iOS 特定属性 ─────────────────────────────────────────
if(IOS AND DEFINED IOS_INFO_PLIST)
    # LaunchScreen — 使用 Qt 6 的 QT_IOS_LAUNCH_SCREEN 属性
    # 不能手动添加为 target_sources，否则会与 Qt 自动生成的 LaunchScreen 冲突
    if(DEFINED IOS_LAUNCH_STORYBOARD)
        set_target_properties(NearLink_Mesh PROPERTIES
            QT_IOS_LAUNCH_SCREEN "${IOS_LAUNCH_STORYBOARD}")
    endif()

    # AppIcon — Asset Catalog
    if(DEFINED IOS_ASSET_CATALOG)
        target_sources(NearLink_Mesh PRIVATE "${IOS_ASSET_CATALOG}")
        set_source_files_properties("${IOS_ASSET_CATALOG}" PROPERTIES
            MACOSX_PACKAGE_LOCATION Resources)
    endif()

    # App 名称本地化（英文 / 简体中文）
    if(DEFINED IOS_LPROJ_EN)
        target_sources(NearLink_Mesh PRIVATE "${IOS_LPROJ_EN}")
        set_source_files_properties("${IOS_LPROJ_EN}" PROPERTIES
            MACOSX_PACKAGE_LOCATION "Resources/en.lproj")
    endif()
    if(DEFINED IOS_LPROJ_ZH)
        target_sources(NearLink_Mesh PRIVATE "${IOS_LPROJ_ZH}")
        set_source_files_properties("${IOS_LPROJ_ZH}" PROPERTIES
            MACOSX_PACKAGE_LOCATION "Resources/zh-Hans.lproj")
    endif()

    set_target_properties(NearLink_Mesh PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST "${IOS_INFO_PLIST}"
        XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "wellwhz"
        XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "Apple Development"
        XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "cn.wellwhz.nearlinkmesh"
        # AppIcon from Assets.xcassets
        XCODE_ATTRIBUTE_ASSETCATALOG_COMPILER_APPICON_NAME "AppIcon"
        # iPhone(1) + iPad(2)
        XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2"
        # 灵动岛需要 iOS 16+；设为 15.0 确保兼容更多设备
        XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET "15.0"
        # 通过 Xcode INFOPLIST_KEY_ 确保蓝牙权限嵌入 Info.plist
        XCODE_ATTRIBUTE_INFOPLIST_KEY_NSBluetoothAlwaysUsageDescription
            "This app uses Bluetooth to connect to NearLink Mesh gateway devices for network topology viewing and data communication."
        XCODE_ATTRIBUTE_INFOPLIST_KEY_NSBluetoothPeripheralUsageDescription
            "This app uses Bluetooth to connect to NearLink Mesh gateway devices."
        XCODE_ATTRIBUTE_INFOPLIST_KEY_UIBackgroundModes "bluetooth-central"
    )
endif()

# ─── macOS 图标 ──────────────────────────────────────────
if(APPLE AND NOT IOS AND DEFINED MACOS_ICON)
    target_sources(NearLink_Mesh PRIVATE "${MACOS_ICON}")
    set_source_files_properties("${MACOS_ICON}" PROPERTIES
        MACOSX_PACKAGE_LOCATION Resources)
    get_filename_component(_icon_name "${MACOS_ICON}" NAME)
    set(MACOSX_ICON_OPTION MACOSX_BUNDLE_ICON_FILE "${_icon_name}")
endif()

# ─── 目标属性 ────────────────────────────────────────────
set_target_properties(NearLink_Mesh PROPERTIES
    ${MACOSX_ICON_OPTION}
    MACOSX_BUNDLE_GUI_IDENTIFIER cn.wellwhz.nearlinkmesh
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE_COPYRIGHT "Copyright © 2025 wellwhz. All rights reserved."
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

# ─── 安装规则 ────────────────────────────────────────────
include(GNUInstallDirs)
install(TARGETS NearLink_Mesh
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(NearLink_Mesh)
endif()
