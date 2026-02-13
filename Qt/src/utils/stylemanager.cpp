#include "stylemanager.h"
#include <QFile>
#include <QTextStream>
#include <QGuiApplication>
#include <QtMath>

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

QString StyleManager::darkThemeStyleSheet()
{
    return QStringLiteral(R"(
QWidget { background-color:#0D1117; color:#E6EDF3; font-family:"Segoe UI","SF Pro Display","PingFang SC",sans-serif; font-size:13px; }
QMainWindow { background-color:#0D1117; }
QWidget#topBar { background-color:#161B22; border-bottom:1px solid rgba(240,246,252,0.1); }
QLabel#appTitle { color:#FFF; font-size:17px; font-weight:600; }
QLabel#statusDot { min-width:10px; max-width:10px; min-height:10px; max-height:10px; border-radius:5px; }
QLabel#debugLabel { background-color:#161B22; color:#D29922; font-family:"Cascadia Code","Consolas",monospace; font-size:11px; padding:10px 12px; border:1px solid rgba(210,153,34,0.15); border-radius:10px; }
QPushButton#scanButton { background-color:#238636; color:#FFF; font-size:14px; font-weight:600; border:1px solid rgba(63,185,80,0.4); border-radius:10px; padding:12px 24px; }
QPushButton#scanButton:disabled { background-color:#21262D; color:#484F58; }
QLabel#hintLabel { color:#484F58; font-size:13px; padding:48px 24px; }
QListWidget#deviceList { background-color:transparent; border:none; }
QListWidget#deviceList::item { background-color:#161B22; border:1px solid rgba(240,246,252,0.06); border-radius:12px; margin:3px 0; }
QWidget#deviceInfoCard { background-color:#161B22; border:1px solid rgba(63,185,80,0.2); border-radius:12px; }
QPushButton#disconnectBtn { background-color:rgba(248,81,73,0.1); color:#F85149; border:1px solid rgba(248,81,73,0.25); border-radius:8px; padding:6px 16px; }
QPushButton#queryTopoBtn { background-color:#21262D; color:#58A6FF; border:1px solid rgba(88,166,255,0.25); border-radius:10px; padding:10px; font-weight:600; }
QWidget#nodeCard { background-color:#161B22; border:1px solid rgba(240,246,252,0.06); border-radius:10px; }
QWidget#broadcastCard { background-color:#161B22; border:1px solid rgba(240,246,252,0.06); border-radius:10px; }
QLineEdit#broadcastInput,QLineEdit#sendDialogInput { background-color:#0D1117; color:#E6EDF3; border:1px solid rgba(240,246,252,0.1); border-radius:8px; padding:9px 12px; }
QPushButton#broadcastBtn,QPushButton#sendDialogSend { background-color:#238636; color:#FFF; border:1px solid rgba(63,185,80,0.4); border-radius:8px; padding:9px 20px; font-weight:600; }
QWidget#logContainer { background-color:#0D1117; border:1px solid rgba(240,246,252,0.06); border-radius:10px; }
QListWidget#logList { background-color:transparent; border:none; font-family:"Cascadia Code","Consolas",monospace; font-size:11px; }
QScrollBar:vertical { background:transparent; width:6px; }
QScrollBar::handle:vertical { background:rgba(139,148,158,0.25); border-radius:3px; min-height:40px; }
QScrollBar::add-line:vertical,QScrollBar::sub-line:vertical { height:0; }
QScrollBar:horizontal { height:0; }
QProgressBar { background-color:#21262D; border:none; border-radius:4px; height:6px; }
QProgressBar::chunk { background-color:#58A6FF; border-radius:3px; }
)");
}
