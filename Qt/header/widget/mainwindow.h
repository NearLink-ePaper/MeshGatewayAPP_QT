#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStackedWidget>
#include <QLabel>
#include <QTimer>

#include "blemanager.h"
#include "scanpage.h"
#include "connectedpage.h"
#include "senddialog.h"

/**
 * 主窗口 — 管理扫描页/连接中/已连接页面切换
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void onConnStateChanged(BleManager::ConnState state);
    void onDeviceSelected(int index);

private:
    void setupTopBar();
    void setupPages();
    void updateStatusDot(BleManager::ConnState state);

    BleManager      *m_ble;
    QStackedWidget  *m_stack;
    ScanPage        *m_scanPage;
    ConnectedPage   *m_connectedPage;
    QWidget         *m_connectingPage;
    QLabel          *m_statusDot;
    QLabel          *m_connectingLabel;
    QTimer           m_topoQueryTimer;
};

#endif // MAINWINDOW_H
