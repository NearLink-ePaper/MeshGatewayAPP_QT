#ifndef STYLEMANAGER_H
#define STYLEMANAGER_H

#include <QString>
#include <QPixmap>

/**
 * 全局样式管理 — 加载 iOS 风格 QSS 样式表
 */
class StyleManager
{
public:
    static QString loadStyleSheet();
    static QString darkThemeStyleSheet();

    /// 加载 SVG 图标并按 devicePixelRatio 缩放，避免高 DPI 下模糊
    static QPixmap loadSvgIcon(const QString &path, int logicalSize);

    /// 密度无关像素: 将逻辑像素乘以 devicePixelRatio 后取整
    static int dp(int logicalPx);
};

#endif // STYLEMANAGER_H
