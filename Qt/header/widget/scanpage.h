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
#include "sockettransport.h"
#include "stylemanager.h"

/**
 * 扫描页面 — 扫描按钮 + 网关设备列表
 */
class ScanPage : public QWidget
{
    Q_OBJECT

public:
    explicit ScanPage(BleManager *ble, SocketTransport *wifi, QWidget *parent = nullptr);

signals:
    void deviceSelected(int index);
    void wifiDeviceSelected(const WifiDevice &device);

private slots:
    void onScanClicked();
    void onConnStateChanged(BleManager::ConnState state);
    void onDevicesChanged();
    void onDebugInfoChanged(const QString &info);
    void onItemClicked(QListWidgetItem *item);
    void onCountdownTick();
    void onWifiDeviceFound(const WifiDevice &device);

private:
    void refreshDeviceList();

    BleManager      *m_ble;
    SocketTransport *m_wifi;
    QLabel          *m_debugLabel;
    AAButton        *m_scanBtn;
    QListWidget     *m_deviceList;
    QLabel          *m_hintLabel;
    QTimer           m_countdownTimer;
    QTimer           m_rssiTimer;
    int              m_remainingSec = 0;

    QList<WifiDevice> m_wifiDevices;
};

#endif // SCANPAGE_H
