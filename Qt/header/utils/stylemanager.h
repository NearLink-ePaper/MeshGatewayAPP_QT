#ifndef STYLEMANAGER_H
#define STYLEMANAGER_H

#include <QString>

/**
 * 全局样式管理 — 加载 iOS 风格 QSS 样式表
 */
class StyleManager
{
public:
    static QString loadStyleSheet();
    static QString darkThemeStyleSheet();
};

#endif // STYLEMANAGER_H
