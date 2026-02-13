# ─── Windows 平台配置 ────────────────────────────────────
# Qt Bluetooth 在 Windows 上依赖 WinRT 后端，仅 MSVC 支持
if(MSVC)
    # MSVC 编译选项
    add_compile_options(
        /utf-8          # 源文件与执行字符集均为 UTF-8
        /W3             # 警告级别 3
        /permissive-    # 严格标准一致性
    )
    # 抑制 C4996 (deprecated) 等常见噪音警告
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
else()
    message(WARNING
        "Windows builds require MSVC for Qt Bluetooth (WinRT backend).\n"
        "MinGW does NOT ship Qt6::Bluetooth. Please switch to an MSVC kit.")
endif()

# Windows 平台宏
add_compile_definitions(WIN32_LEAN_AND_MEAN NOMINMAX)

# ── App Icon（.ico 由 CI 从 icon.png 生成）────────────────
set(WIN_ICON_RC "${CMAKE_CURRENT_SOURCE_DIR}/Windows/app.rc")
set(WIN_ICON_ICO "${CMAKE_CURRENT_SOURCE_DIR}/Windows/icon.ico")
if(EXISTS "${WIN_ICON_ICO}")
    list(APPEND PLATFORM_SOURCES "${WIN_ICON_RC}")
endif()

# 如需添加 Windows 特定源文件，在此追加：
# list(APPEND PLATFORM_SOURCES
#     Windows/src/winble.cpp
# )
# include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Windows/header)
