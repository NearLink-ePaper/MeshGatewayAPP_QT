# ─── Android 平台配置 ────────────────────────────────────

# ── 自动检测 Qt Android Activity 类路径 ──────────────────
# Qt 6.7+ 部分 kit 将 Java 类从 org.qtproject.qt.android.bindings
# 迁移到 org.qtproject.qt.android，但并非所有架构都已迁移。
# 通过检查 Qt 安装目录中的 Java 源文件自动判断。

get_filename_component(_qt_prefix "${Qt6_DIR}/../../.." ABSOLUTE)
set(_qt_java_base "${_qt_prefix}/src/android/java/src/org/qtproject/qt/android")

if(EXISTS "${_qt_java_base}/QtActivity.java")
    set(QT_ANDROID_ACTIVITY_CLASS "org.qtproject.qt.android.QtActivity")
    set(QT_ANDROID_APPLICATION_CLASS "org.qtproject.qt.android.QtApplication")
elseif(EXISTS "${_qt_java_base}/bindings/QtActivity.java")
    set(QT_ANDROID_ACTIVITY_CLASS "org.qtproject.qt.android.bindings.QtActivity")
    set(QT_ANDROID_APPLICATION_CLASS "org.qtproject.qt.android.bindings.QtApplication")
else()
    message(WARNING "Cannot find QtActivity.java in ${_qt_java_base}, defaulting to bindings path")
    set(QT_ANDROID_ACTIVITY_CLASS "org.qtproject.qt.android.bindings.QtActivity")
    set(QT_ANDROID_APPLICATION_CLASS "org.qtproject.qt.android.bindings.QtApplication")
endif()

message(STATUS "Qt Android Activity class: ${QT_ANDROID_ACTIVITY_CLASS}")

# ── 从模板生成 AndroidManifest.xml ───────────────────────
# 直接生成到源码 Android/ 目录，确保 Qt Creator 能找到
set(ANDROID_PACKAGE_OUTPUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Android")
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/Android/AndroidManifest.xml.in
    ${ANDROID_PACKAGE_OUTPUT_DIR}/AndroidManifest.xml
    @ONLY
)

# 如需添加 Android 专用代码（如 JNI 桥接），在此添加源文件：
# list(APPEND PLATFORM_SOURCES
#     Android/src/androidble.cpp
# )
# include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Android/header)
