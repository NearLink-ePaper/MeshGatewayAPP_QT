#include "scanpage.h"
#include "stylemanager.h"
#include <QHBoxLayout>
#include <QFont>
#include <QFrame>

ScanPage::ScanPage(BleManager *ble, SocketTransport *wifi, QWidget *parent)
    : QWidget(parent), m_ble(ble), m_wifi(wifi)
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(12);

    // 调试信息卡片（AAWidget 容器提供抗锯齿圆角）
    auto *debugCard = new AAWidget(this);
    debugCard->setObjectName("debugCard");
    auto *debugCardLayout = new QVBoxLayout(debugCard);
    debugCardLayout->setContentsMargins(12, 10, 12, 10);
    m_debugLabel = new QLabel(m_ble->debugInfo(), debugCard);
    m_debugLabel->setObjectName("debugLabel");
    m_debugLabel->setWordWrap(true);
    debugCardLayout->addWidget(m_debugLabel);
    mainLayout->addWidget(debugCard);

    // 设备列表（初始隐藏，扫描到设备后显示）
    m_deviceList = new QListWidget(this);
    m_deviceList->setObjectName("deviceList");
    m_deviceList->setFrameShape(QFrame::NoFrame);
    m_deviceList->setSpacing(4);
    m_deviceList->setSelectionMode(QAbstractItemView::NoSelection);
    m_deviceList->setVisible(false);
    m_deviceList->setItemDelegate(new AAItemDelegate(
        12, QColor(0x16, 0x1B, 0x22), QColor(240, 246, 252, 15), 1, m_deviceList));
    connect(m_deviceList, &QListWidget::itemClicked, this, &ScanPage::onItemClicked);
    mainLayout->addWidget(m_deviceList, 1);

    // 弹性空间（设备列表隐藏时将按钮推到底部）
    mainLayout->addStretch(1);

    // 扫描按钮
    m_scanBtn = new AAButton(tr("Scan Mesh Gateway"), this);
    m_scanBtn->setObjectName("scanButton");
    m_scanBtn->setMinimumHeight(48);
    m_scanBtn->setCursor(Qt::PointingHandCursor);
    connect(m_scanBtn, &QPushButton::clicked, this, &ScanPage::onScanClicked);
    mainLayout->addWidget(m_scanBtn);

    // 提示文本
    m_hintLabel = new QLabel(tr("Click the button above to scan\nAuto-discover SLE_GW_XXXX devices"), this);
    m_hintLabel->setObjectName("hintLabel");
    m_hintLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(m_hintLabel);

    // 倒计时定时器
    m_countdownTimer.setInterval(1000);
    connect(&m_countdownTimer, &QTimer::timeout, this, &ScanPage::onCountdownTick);

    // RSSI 刷新定时器 (500ms)
    m_rssiTimer.setInterval(500);
    connect(&m_rssiTimer, &QTimer::timeout, this, &ScanPage::refreshDeviceList);

    // 信号连接
    connect(m_ble, &BleManager::connStateChanged, this, &ScanPage::onConnStateChanged);
    connect(m_ble, &BleManager::scannedDevicesChanged, this, &ScanPage::onDevicesChanged);
    connect(m_ble, &BleManager::debugInfoChanged, this, &ScanPage::onDebugInfoChanged);
    if (m_wifi) {
        connect(m_wifi, &SocketTransport::wifiDeviceFound, this, &ScanPage::onWifiDeviceFound);
    }
}

void ScanPage::onScanClicked()
{
    m_wifiDevices.clear();
    m_ble->startScan();
    // 同时探测 WiFi 设备
    if (m_wifi)
        m_wifi->startProbe();
}

void ScanPage::onConnStateChanged(BleManager::ConnState state)
{
    bool scanning = (state == BleManager::Scanning);
    m_scanBtn->setEnabled(!scanning);
    m_scanBtn->setText(scanning ? tr("Scanning...") : tr("Scan Mesh Gateway"));

    if (scanning) {
        m_remainingSec = 12;
        m_debugLabel->setText(tr("Scanning... (%1s)").arg(m_remainingSec));
        m_countdownTimer.start();
        m_rssiTimer.start();
    } else {
        m_countdownTimer.stop();
        m_rssiTimer.stop();
    }
}

void ScanPage::onCountdownTick()
{
    if (--m_remainingSec > 0) {
        m_debugLabel->setText(tr("Scanning... (%1s)").arg(m_remainingSec));
    } else {
        m_countdownTimer.stop();
    }
}

void ScanPage::onDevicesChanged()
{
    refreshDeviceList();
}

void ScanPage::onDebugInfoChanged(const QString &info)
{
    m_debugLabel->setText(info);
}

void ScanPage::onItemClicked(QListWidgetItem *item)
{
    int index = item->data(Qt::UserRole).toInt();
    if (index >= 0) {
        emit deviceSelected(index);
    } else {
        int wifiIdx = -(index + 1);
        if (wifiIdx >= 0 && wifiIdx < m_wifiDevices.size())
            emit wifiDeviceSelected(m_wifiDevices.at(wifiIdx));
    }
}

void ScanPage::onWifiDeviceFound(const WifiDevice &device)
{
    m_wifiDevices.append(device);
    refreshDeviceList();
}

// item data encoding:
//   UserRole int >= 0        → BLE device index
//   UserRole int < 0         → WiFi device: -(wifiIndex+1)

void ScanPage::refreshDeviceList()
{
    m_deviceList->clear();
    const auto &bleDevices = m_ble->scannedDevices();

    bool anyDevice = !bleDevices.isEmpty() || !m_wifiDevices.isEmpty();
    m_hintLabel->setVisible(!anyDevice);
    m_deviceList->setVisible(anyDevice);

    // ─── BLE 设备 ─────────────────────────────────────────
    for (int i = 0; i < bleDevices.size(); ++i) {
        const auto &dev = bleDevices.at(i);

        auto *widget = new AAWidget();
        widget->setObjectName("deviceCard");
        auto *hLayout = new QHBoxLayout(widget);
        hLayout->setContentsMargins(14, 12, 14, 12);
        hLayout->setSpacing(12);

        auto *iconLabel = new QLabel(widget);
        iconLabel->setPixmap(StyleManager::loadSvgIcon(":/img/bluetooth.svg", 22));
        iconLabel->setFixedSize(36, 36);
        iconLabel->setAlignment(Qt::AlignCenter);
        iconLabel->setObjectName("deviceIcon");
        hLayout->addWidget(iconLabel, 0, Qt::AlignVCenter);

        auto *infoLayout = new QVBoxLayout();
        infoLayout->setSpacing(4);
        auto *nameLabel = new QLabel(dev.name, widget);
        nameLabel->setObjectName("deviceName");
        nameLabel->setMinimumHeight(22);
        auto *addrLabel = new QLabel(dev.address, widget);
        addrLabel->setObjectName("deviceAddr");
        infoLayout->addWidget(nameLabel);
        infoLayout->addWidget(addrLabel);
        hLayout->addLayout(infoLayout, 1);

        auto *rssiLabel = new QLabel(QString("%1 dBm").arg(dev.rssi), widget);
        rssiLabel->setObjectName(dev.rssi > -65 ? "rssiGood" : "rssiWeak");
        hLayout->addWidget(rssiLabel, 0, Qt::AlignVCenter);

        auto *item = new QListWidgetItem(m_deviceList);
        item->setData(Qt::UserRole, i);         // BLE: index >= 0
        QSize hint = widget->sizeHint();
        hint.setHeight(hint.height() + 8);
        item->setSizeHint(hint);
        m_deviceList->setItemWidget(item, widget);
    }

    // ─── WiFi 设备 ────────────────────────────────────────
    for (int i = 0; i < m_wifiDevices.size(); ++i) {
        const auto &dev = m_wifiDevices.at(i);

        auto *widget = new AAWidget();
        widget->setObjectName("deviceCard");
        auto *hLayout = new QHBoxLayout(widget);
        hLayout->setContentsMargins(14, 12, 14, 12);
        hLayout->setSpacing(12);

        auto *iconLabel = new QLabel(widget);
        iconLabel->setPixmap(StyleManager::loadSvgIcon(":/img/wireless.svg", 22));
        iconLabel->setFixedSize(36, 36);
        iconLabel->setAlignment(Qt::AlignCenter);
        iconLabel->setObjectName("deviceIcon");
        hLayout->addWidget(iconLabel, 0, Qt::AlignVCenter);

        auto *infoLayout = new QVBoxLayout();
        infoLayout->setSpacing(4);
        auto *nameLabel = new QLabel(dev.name, widget);
        nameLabel->setObjectName("deviceName");
        nameLabel->setMinimumHeight(22);
        auto *addrLabel = new QLabel(QStringLiteral("%1:%2  [WiFi]").arg(dev.host).arg(dev.port), widget);
        addrLabel->setObjectName("deviceAddr");
        infoLayout->addWidget(nameLabel);
        infoLayout->addWidget(addrLabel);
        hLayout->addLayout(infoLayout, 1);

        auto *tagLabel = new QLabel(tr("TCP"), widget);
        tagLabel->setObjectName("rssiGood");
        hLayout->addWidget(tagLabel, 0, Qt::AlignVCenter);

        auto *item = new QListWidgetItem(m_deviceList);
        item->setData(Qt::UserRole, -(i + 1));  // WiFi: 负数编码 -(index+1)
        QSize hint = widget->sizeHint();
        hint.setHeight(hint.height() + 8);
        item->setSizeHint(hint);
        m_deviceList->setItemWidget(item, widget);
    }
}
