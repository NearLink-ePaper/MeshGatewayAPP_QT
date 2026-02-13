#include "stylemanager.h"
#include <QFile>
#include <QTextStream>
#include <QGuiApplication>
#include <QtMath>
#include <QPainter>
#include <QPainterPath>

QString StyleManager::loadStyleSheet()
{
    QFile file(":/style/style.qss");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream stream(&file);
        return stream.readAll();
    }
    // 如果资源文件加载失败，使用内置样式
    return darkThemeStyleSheet();
}

QPixmap StyleManager::loadSvgIcon(const QString &path, int logicalSize)
{
    qreal dpr = qApp ? qApp->devicePixelRatio() : 1.0;
    int phys = qCeil(logicalSize * dpr);
    QPixmap pix(path);
    pix = pix.scaled(phys, phys, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    pix.setDevicePixelRatio(dpr);
    return pix;
}

int StyleManager::dp(int logicalPx)
{
    qreal dpr = qApp ? qApp->devicePixelRatio() : 1.0;
    return qCeil(logicalPx * dpr);
}

// ─── AAWidget ────────────────────────────────────────────
void AAWidget::paintEvent(QPaintEvent *)
{
    if (m_radius <= 0 && m_bg == Qt::transparent && m_border == Qt::transparent)
        return;                       // 无需自定义绘制

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    const qreal hw = m_borderW / 2.0; // 边框半宽，向内收缩避免裁切
    QRectF r = QRectF(rect()).adjusted(hw, hw, -hw, -hw);

    QPainterPath path;
    path.addRoundedRect(r, m_radius, m_radius);

    if (m_bg.alpha() > 0) {
        p.fillPath(path, m_bg);
    }
    if (m_borderW > 0 && m_border.alpha() > 0) {
        p.setPen(QPen(m_border, m_borderW));
        p.drawPath(path);
    }
}

// ─── AAButton ────────────────────────────────────────────
void AAButton::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    // 根据按钮状态微调背景色
    QColor bg = m_bg;
    if (!isEnabled()) {
        bg.setAlphaF(bg.alphaF() * 0.3);
    } else if (isDown() && bg.alpha() > 0) {
        bg = bg.darker(130);
    } else if (underMouse() && bg.alpha() > 0) {
        bg = bg.lighter(115);
    }

    const qreal hw = m_borderW / 2.0;
    QRectF r = QRectF(rect()).adjusted(hw, hw, -hw, -hw);

    QPainterPath path;
    path.addRoundedRect(r, m_radius, m_radius);

    if (bg.alpha() > 0)
        p.fillPath(path, bg);

    if (m_borderW > 0 && m_border.alpha() > 0) {
        p.setPen(QPen(m_border, m_borderW));
        p.drawPath(path);
    }

    // 绘制按钮文字（居中）
    p.setPen(palette().color(isEnabled() ? QPalette::ButtonText : QPalette::Mid));
    p.drawText(rect(), Qt::AlignCenter, text());
}

QString StyleManager::darkThemeStyleSheet()
{
    return QStringLiteral(R"(
QWidget { background-color:#0D1117; color:#E6EDF3; font-size:13px; }
QMainWindow { background-color:#0D1117; }
QWidget#topBar { background-color:#161B22; border-bottom:1px solid rgba(240,246,252,0.1); }
QLabel#appTitle { color:#FFF; font-size:17px; font-weight:600; }
QLabel#statusDot { min-width:10px; max-width:10px; min-height:10px; max-height:10px; border-radius:5px; }
QLabel#debugLabel { background-color:#161B22; color:#D29922; font-family:monospace; font-size:11px; padding:10px 12px; border:1px solid rgba(210,153,34,0.15); border-radius:10px; }
AAButton#scanButton { background-color:transparent; border:none; color:#FFF; font-size:14px; font-weight:600; padding:12px 24px; qproperty-bgColor:#238636; qproperty-borderColor:#663FB950; qproperty-borderRadius:10; qproperty-borderWidth:1; }
AAButton#scanButton:disabled { color:#484F58; }
QLabel#hintLabel { color:#484F58; font-size:13px; padding:48px 24px; }
QListWidget#deviceList { background-color:transparent; border:none; }
QListWidget#deviceList::item { background-color:#161B22; border:1px solid rgba(240,246,252,0.06); border-radius:12px; margin:3px 0; }
AAWidget#deviceInfoCard { background-color:transparent; border:none; qproperty-bgColor:#161B22; qproperty-borderColor:#333FB950; qproperty-borderRadius:12; qproperty-borderWidth:1; }
AAButton#disconnectBtn { background-color:transparent; border:none; color:#F85149; padding:6px 16px; qproperty-bgColor:#1AF85149; qproperty-borderColor:#40F85149; qproperty-borderRadius:8; qproperty-borderWidth:1; }
AAButton#queryTopoBtn { background-color:transparent; border:none; color:#58A6FF; padding:10px; font-weight:600; qproperty-bgColor:#21262D; qproperty-borderColor:#4058A6FF; qproperty-borderRadius:10; qproperty-borderWidth:1; }
AAWidget#nodeCard { background-color:transparent; border:none; qproperty-bgColor:#161B22; qproperty-borderColor:#0FF0F6FC; qproperty-borderRadius:10; qproperty-borderWidth:1; }
AAWidget#broadcastCard { background-color:transparent; border:none; qproperty-bgColor:#161B22; qproperty-borderColor:#0FF0F6FC; qproperty-borderRadius:10; qproperty-borderWidth:1; }
QLineEdit#broadcastInput,QLineEdit#sendDialogInput { background-color:#0D1117; color:#E6EDF3; border:1px solid rgba(240,246,252,0.1); border-radius:8px; padding:9px 12px; }
AAButton#broadcastBtn,AAButton#sendDialogSend { background-color:transparent; border:none; color:#FFF; padding:9px 20px; font-weight:600; qproperty-bgColor:#238636; qproperty-borderColor:#663FB950; qproperty-borderRadius:8; qproperty-borderWidth:1; }
AAWidget#logContainer { background-color:transparent; border:none; qproperty-bgColor:#0D1117; qproperty-borderColor:#0FF0F6FC; qproperty-borderRadius:10; qproperty-borderWidth:1; }
QListWidget#logList { background-color:transparent; border:none; font-family:monospace; font-size:11px; }
QScrollBar:vertical { background:transparent; width:6px; }
QScrollBar::handle:vertical { background:rgba(139,148,158,0.25); border-radius:3px; min-height:40px; }
QScrollBar::add-line:vertical,QScrollBar::sub-line:vertical { height:0; }
QScrollBar:horizontal { height:0; }
QProgressBar { background-color:#21262D; border:none; border-radius:4px; height:6px; }
QProgressBar::chunk { background-color:#58A6FF; border-radius:3px; }
)");
}
