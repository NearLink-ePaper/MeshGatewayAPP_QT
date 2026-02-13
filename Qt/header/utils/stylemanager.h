#ifndef STYLEMANAGER_H
#define STYLEMANAGER_H

#include <QString>
#include <QPixmap>
#include <QWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QStyledItemDelegate>
#include <QColor>

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

/**
 * 抗锯齿 QWidget — 手动绘制圆角背景/边框，完全绕过 QSS border-radius
 * 在 QSS 中使用 qproperty-* 设置视觉属性:
 *   qproperty-bgColor, qproperty-borderColor,
 *   qproperty-borderRadius, qproperty-borderWidth
 */
class AAWidget : public QWidget {
    Q_OBJECT
    Q_PROPERTY(QColor bgColor      READ bgColor      WRITE setBgColor)
    Q_PROPERTY(QColor borderColor   READ borderColor   WRITE setBorderColor)
    Q_PROPERTY(int    borderRadius  READ borderRadius  WRITE setBorderRadius)
    Q_PROPERTY(int    borderWidth   READ borderWidth   WRITE setBorderWidth)
public:
    using QWidget::QWidget;

    QColor bgColor()      const { return m_bg; }
    QColor borderColor()  const { return m_border; }
    int    borderRadius() const { return m_radius; }
    int    borderWidth()  const { return m_borderW; }

    void setBgColor(const QColor &c)     { m_bg = c; update(); }
    void setBorderColor(const QColor &c) { m_border = c; update(); }
    void setBorderRadius(int r)          { m_radius = r; update(); }
    void setBorderWidth(int w)           { m_borderW = w; update(); }

protected:
    void paintEvent(QPaintEvent *) override;

private:
    QColor m_bg      = Qt::transparent;
    QColor m_border  = Qt::transparent;
    int    m_radius  = 0;
    int    m_borderW = 0;
};

/**
 * 抗锯齿 QPushButton — 手动绘制圆角背景/边框
 * 在 QSS 中使用 qproperty-* 设置视觉属性
 */
class AAButton : public QPushButton {
    Q_OBJECT
    Q_PROPERTY(QColor bgColor      READ bgColor      WRITE setBgColor)
    Q_PROPERTY(QColor borderColor   READ borderColor   WRITE setBorderColor)
    Q_PROPERTY(int    borderRadius  READ borderRadius  WRITE setBorderRadius)
    Q_PROPERTY(int    borderWidth   READ borderWidth   WRITE setBorderWidth)
public:
    using QPushButton::QPushButton;

    QColor bgColor()      const { return m_bg; }
    QColor borderColor()  const { return m_border; }
    int    borderRadius() const { return m_radius; }
    int    borderWidth()  const { return m_borderW; }

    void setBgColor(const QColor &c)     { m_bg = c; update(); }
    void setBorderColor(const QColor &c) { m_border = c; update(); }
    void setBorderRadius(int r)          { m_radius = r; update(); }
    void setBorderWidth(int w)           { m_borderW = w; update(); }

protected:
    void paintEvent(QPaintEvent *) override;

private:
    QColor m_bg      = Qt::transparent;
    QColor m_border  = Qt::transparent;
    int    m_radius  = 0;
    int    m_borderW = 0;
};

/**
 * 抗锯齿 QLineEdit — 手动绘制圆角背景/边框，支持 focus 状态边框色
 */
class AALineEdit : public QLineEdit {
    Q_OBJECT
    Q_PROPERTY(QColor bgColor          READ bgColor          WRITE setBgColor)
    Q_PROPERTY(QColor borderColor       READ borderColor       WRITE setBorderColor)
    Q_PROPERTY(QColor focusBorderColor  READ focusBorderColor  WRITE setFocusBorderColor)
    Q_PROPERTY(int    borderRadius      READ borderRadius      WRITE setBorderRadius)
    Q_PROPERTY(int    borderWidth       READ borderWidth       WRITE setBorderWidth)
public:
    using QLineEdit::QLineEdit;

    QColor bgColor()         const { return m_bg; }
    QColor borderColor()     const { return m_border; }
    QColor focusBorderColor() const { return m_focusBorder; }
    int    borderRadius()    const { return m_radius; }
    int    borderWidth()     const { return m_borderW; }

    void setBgColor(const QColor &c)         { m_bg = c; update(); }
    void setBorderColor(const QColor &c)     { m_border = c; update(); }
    void setFocusBorderColor(const QColor &c) { m_focusBorder = c; update(); }
    void setBorderRadius(int r)              { m_radius = r; update(); }
    void setBorderWidth(int w)               { m_borderW = w; update(); }

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QColor m_bg          = Qt::transparent;
    QColor m_border      = Qt::transparent;
    QColor m_focusBorder = Qt::transparent;
    int    m_radius      = 0;
    int    m_borderW     = 0;
};

/**
 * 抗锯齿列表项代理 — 手动绘制圆角背景，替代 QSS ::item border-radius
 */
class AAItemDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    explicit AAItemDelegate(int radius, const QColor &bg, const QColor &border,
                            int borderWidth = 1, QObject *parent = nullptr);
    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;

private:
    int    m_radius;
    QColor m_bg;
    QColor m_border;
    int    m_borderW;
};

#endif // STYLEMANAGER_H
