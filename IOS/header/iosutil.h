#pragma once
#if defined(Q_OS_IOS)

#include <functional>
#include <QImage>

/**
 * 调起 iOS 原生相册 (PHPickerViewController)
 * 用户选择图片后在主线程回调 callback(QImage)
 * 取消选择时回调 callback(QImage()) —— isNull() == true
 */
void iosOpenPhotoLibrary(std::function<void(QImage)> callback);

#endif // Q_OS_IOS
