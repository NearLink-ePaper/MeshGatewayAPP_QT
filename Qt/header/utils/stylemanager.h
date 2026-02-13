#ifndef STYLEMANAGER_H
#define STYLEMANAGER_H

#include <QString>
#include <QPixmap>
#include <QWidget>
#include <QPushButton>

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

/// 抗锯齿 QWidget: 绘制 QSS 背景/边框时启用 QPainter::Antialiasing
class AAWidget : public QWidget {
public:
    using QWidget::QWidget;
protected:
    void paintEvent(QPaintEvent *) override;
};

/// 抗锯齿 QPushButton: 绘制按钮时启用 QPainter::Antialiasing
class AAButton : public QPushButton {
public:
    using QPushButton::QPushButton;
protected:
    void paintEvent(QPaintEvent *) override;
};

#endif // STYLEMANAGER_H
