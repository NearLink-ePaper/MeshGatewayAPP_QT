#ifndef SCANPAGE_H
#define SCANPAGE_H

#include <QWidget>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QListWidget>
#include <QProgressBar>
#include <QTimer>

#include "blemanager.h"

/**
 * 扫描页面 — 扫描按钮 + 网关设备列表
 */
class ScanPage : public QWidget
{
    Q_OBJECT

public:
    explicit ScanPage(BleManager *ble, QWidget *parent = nullptr);

signals:
    void deviceSelected(int index);

private slots:
    void onScanClicked();
    void onConnStateChanged(BleManager::ConnState state);
    void onDevicesChanged();
    void onDebugInfoChanged(const QString &info);
    void onItemClicked(QListWidgetItem *item);
    void onCountdownTick();

private:
    void refreshDeviceList();

    BleManager  *m_ble;
    QLabel      *m_debugLabel;
    QPushButton *m_scanBtn;
    QListWidget *m_deviceList;
    QLabel      *m_hintLabel;
    QTimer       m_countdownTimer;
    QTimer       m_rssiTimer;
    int          m_remainingSec = 0;
};

#endif // SCANPAGE_H
