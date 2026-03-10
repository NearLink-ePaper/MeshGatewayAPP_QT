#include "mainwindow.h"
#include "stylemanager.h"
#include "imageutils.h"

#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QProgressBar>
#include <QFileDialog>
#include <QImageReader>
#if defined(Q_OS_IOS)
#include "iosutil.h"
#endif

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle(qApp->applicationName());
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    // 移动端全屏显示
    showMaximized();
#else
    resize(420, 700);
    setMinimumSize(360, 560);
#endif

    // 创建 BLE 管理器
    m_ble = new BleManager(this);

    // 创建 WiFi Socket 传输
    m_wifi = new SocketTransport(this);

    // 中心 widget
    auto *central = new QWidget(this);
    central->setObjectName("centralWidget");
    setCentralWidget(central);

    auto *rootLayout = new QVBoxLayout(central);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    setupTopBar();
    rootLayout->addWidget(findChild<QWidget*>("topBar"));

    setupPages();
    rootLayout->addWidget(m_stack, 1);

    // 信号连接
    connect(m_ble, &BleManager::connStateChanged, this, &MainWindow::onConnStateChanged);
    connect(m_scanPage, &ScanPage::deviceSelected, this, &MainWindow::onDeviceSelected);
    connect(m_scanPage, &ScanPage::wifiDeviceSelected, this, &MainWindow::onWifiDeviceSelected);

    // 连接后自动查询拓扑（延迟 800ms 等 CCCD 订阅完成）
    m_topoQueryTimer.setSingleShot(true);
    m_topoQueryTimer.setInterval(800);
    connect(&m_topoQueryTimer, &QTimer::timeout, this, [this]() {
        m_ble->queryTopology();
    });

    // 30s 自动刷新拓扑
    m_autoTopoTimer.setInterval(30000);
    connect(&m_autoTopoTimer, &QTimer::timeout, this, [this]() {
        if (m_ble->connState() == BleManager::Connected && !m_ble->isImageBusy())
            m_ble->queryTopology();
    });

    // 图片传输完成后自动刷新拓扑
    connect(m_ble, &BleManager::imageSendStateChanged, this, &MainWindow::onImageSendDone);

    // WiFi 传输进度 / 状态 / 结果
    connect(m_wifi, &SocketTransport::progressChanged,
            m_connectedPage, &ConnectedPage::updateWifiProgress);
    connect(m_wifi, &SocketTransport::stateChanged, this,
            [this](SocketTransport::State s) {
        m_connectedPage->onWifiStateChanged(s);
    });
    connect(m_wifi, &SocketTransport::finished, this, [this](bool ok, const QString &msg) {
        m_connectedPage->onWifiStateChanged(
            ok ? SocketTransport::Done : SocketTransport::Error, msg);
        m_connectedPage->addLog(ok ? tr("[WiFi] %1").arg(msg)
                                   : tr("[WiFi] 失败: %1").arg(msg));
    });

    // 加载持久化的节点图片
    m_nodeImages = NodeImageStore::loadAll();
}

MainWindow::~MainWindow()
{
    m_ble->disconnectDevice();
}

void MainWindow::setupTopBar()
{
    auto *topBar = new AAWidget(this);
    topBar->setObjectName("topBar");
    topBar->setFixedHeight(52);

    auto *layout = new QHBoxLayout(topBar);
    layout->setContentsMargins(16, 0, 16, 0);

    auto *title = new QLabel(qApp->applicationName(), topBar);
    title->setObjectName("appTitle");
    layout->addWidget(title);

    layout->addStretch();

    m_statusDot = new QLabel(topBar);
    m_statusDot->setObjectName("statusDot");
    m_statusDot->setFixedSize(12, 12);
    updateStatusDot(BleManager::Disconnected);
    layout->addWidget(m_statusDot);
}

void MainWindow::setupPages()
{
    m_stack = new QStackedWidget(this);

    // 页面 0: 扫描页
    m_scanPage = new ScanPage(m_ble, m_wifi, this);
    m_stack->addWidget(m_scanPage);

    // 页面 1: 连接中
    m_connectingPage = new QWidget(this);
    auto *connectingLayout = new QVBoxLayout(m_connectingPage);
    connectingLayout->setAlignment(Qt::AlignCenter);
    auto *spinner = new QProgressBar(m_connectingPage);
    spinner->setRange(0, 0); // indeterminate
    spinner->setFixedWidth(120);
    spinner->setTextVisible(false);
    m_connectingLabel = new QLabel(tr("连接中..."), m_connectingPage);
    m_connectingLabel->setObjectName("connectingLabel");
    m_connectingLabel->setAlignment(Qt::AlignCenter);
    connectingLayout->addWidget(spinner, 0, Qt::AlignCenter);
    connectingLayout->addWidget(m_connectingLabel);
    m_stack->addWidget(m_connectingPage);

    // 页面 2: 已连接页
    m_connectedPage = new ConnectedPage(m_ble, this);
    connect(m_connectedPage, &ConnectedPage::nodeClicked, this, &MainWindow::onNodeClicked);
    connect(m_connectedPage, &ConnectedPage::multicastImageRequested, this, &MainWindow::onMulticastImageRequested);
    // WiFi 断开：取消传输，恢复扫描页
    connect(m_connectedPage, &ConnectedPage::wifiDisconnectRequested, this, [this]() {
        m_wifi->cancel();
        m_connectedPage->resetMode();
        m_currentTransport = ImagePreviewDialog::BleTransport;
        m_stack->setCurrentIndex(0);
        updateStatusDot(BleManager::Disconnected);
    });
    // WiFi 取消传输
    connect(m_connectedPage, &ConnectedPage::wifiCancelRequested, this, [this]() {
        m_wifi->cancel();
    });
    m_stack->addWidget(m_connectedPage);

    m_stack->setCurrentIndex(0);
}

void MainWindow::onConnStateChanged(BleManager::ConnState state)
{
    updateStatusDot(state);

    switch (state) {
    case BleManager::Disconnected:
    case BleManager::Scanning:
        if (m_currentTransport == ImagePreviewDialog::WifiTransport) break;
        m_stack->setCurrentIndex(0);
        m_topoQueryTimer.stop();
        m_autoTopoTimer.stop();
        break;
    case BleManager::Connecting:
        m_connectingLabel->setText(tr("正在连接 %1 ...").arg(m_ble->deviceName()));
        m_stack->setCurrentIndex(1);
        break;
    case BleManager::Connected:
        m_stack->setCurrentIndex(2);
        m_topoQueryTimer.start();
        m_autoTopoTimer.start();
        break;
    }
}

void MainWindow::onDeviceSelected(int index)
{
    m_currentTransport = ImagePreviewDialog::BleTransport;
    m_ble->connectToDevice(index);
}

void MainWindow::onWifiDeviceSelected(const WifiDevice &device)
{
    m_currentTransport = ImagePreviewDialog::WifiTransport;
    m_wifi->setHost(device.host, device.port);

    // 填充 ConnectedPage 的设备信息和节点列表
    m_connectedPage->setWifiMode(device);
    m_connectedPage->addLog(tr("[WiFi] 已连接 %1 (%2:%3)")
                                .arg(device.name).arg(device.host).arg(device.port));
    m_stack->setCurrentIndex(2);
    m_statusDot->setStyleSheet("background-color: #4CAF50; border-radius: 5px;");
}

void MainWindow::updateStatusDot(BleManager::ConnState state)
{
    QString color;
    switch (state) {
    case BleManager::Connected:    color = "#3FB950"; break;
    case BleManager::Connecting:   color = "#D29922"; break;
    case BleManager::Scanning:     color = "#58A6FF"; break;
    default:                       color = "#484F58"; break;
    }
    m_statusDot->setStyleSheet(
        QString("background-color: %1; border-radius: 5px;").arg(color));
}

// ═══════════════════════════════════════════════════════
//  节点操作流程
// ═══════════════════════════════════════════════════════

void MainWindow::onNodeClicked(const MeshNode &node)
{
    if (m_ble->isImageBusy()) return;

    QImage lastBitmap = m_nodeImages.value(node.addr);
    auto *dlg = new NodeActionDialog(node, lastBitmap, this);
    connect(dlg, &NodeActionDialog::sendTextRequested, this, &MainWindow::onSendTextToNode);
    connect(dlg, &NodeActionDialog::sendImageRequested, this, &MainWindow::onSendImageToNode);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->open();
}

void MainWindow::onSendTextToNode(const MeshNode &node)
{
    auto *dlg = new SendDialog(node, this);
    if (dlg->exec() == QDialog::Accepted) {
        QString text = dlg->text();
        if (!text.isEmpty()) {
            m_ble->sendToNode(node.addr, text.toUtf8());
            m_connectedPage->addLog(QString::fromUtf8("\xe2\x86\x92 [0x%1] %2")
                                        .arg(MeshProtocol::addrToHex4(node.addr), text));
        }
    }
    dlg->deleteLater();
}

void MainWindow::onSendImageToNode(const MeshNode &node)
{
    m_currentNode = node;
    m_currentMcastTargets.clear();
    openImagePicker(node);
}

void MainWindow::onMulticastImageRequested(const QList<quint16> &targets)
{
    if (targets.isEmpty()) return;
    if (m_ble->isImageBusy()) return;
    m_currentNode = MeshNode(0xFFFF, 0);
    m_currentMcastTargets = targets;
    openImagePicker(m_currentNode, targets);
}

void MainWindow::openImagePicker(const MeshNode &node, const QList<quint16> &multicastTargets)
{
#if defined(Q_OS_IOS)
    // iOS: PHPickerViewController 直接打开原生相册（ObjC++ 桥接）
    iosOpenPhotoLibrary([this, node, multicastTargets](QImage img) {
        if (img.isNull()) return;
        openCropDialog(img, node, multicastTargets);
    });
#elif defined(Q_OS_ANDROID)
    // Android: getOpenFileContent 触发 ACTION_GET_CONTENT image/*
    QFileDialog::getOpenFileContent(
        QStringLiteral("Images (*.jpg *.jpeg *.png *.bmp *.gif *.webp)"),
        [this, node, multicastTargets](const QString &fileName, const QByteArray &fileContent) {
            if (fileName.isEmpty() || fileContent.isEmpty()) return;
            QImage img;
            if (!img.loadFromData(fileContent)) return;
            openCropDialog(img, node, multicastTargets);
        });
#else
    QStringList filters;
    for (const auto &fmt : QImageReader::supportedImageFormats())
        filters << QStringLiteral("*.%1").arg(QString::fromLatin1(fmt));
    QString filter = tr("图片 (%1)").arg(filters.join(' '));

    QString path = QFileDialog::getOpenFileName(this, tr("选择图片"), QString(), filter);
    if (path.isEmpty()) return;

    QImage img(path);
    if (img.isNull()) return;

    openCropDialog(img, node, multicastTargets);
#endif
}

void MainWindow::openCropDialog(const QImage &image, const MeshNode &node,
                                 const QList<quint16> &multicastTargets)
{
    auto *dlg = new CropImageDialog(image, node, multicastTargets.size(), this);
    ImagePreviewDialog::TransportMode transport = m_currentTransport;
    connect(dlg, &CropImageDialog::cropConfirmed, this,
            [this, node, multicastTargets, transport](const QImage &cropped, const ImageResolution &res) {
        openPreviewDialog(cropped, res, node, multicastTargets, transport);
    });
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->open();
}

void MainWindow::openPreviewDialog(const QImage &cropped, const ImageResolution &res,
                                    const MeshNode &node, const QList<quint16> &multicastTargets,
                                    ImagePreviewDialog::TransportMode transport)
{
    auto *dlg = new ImagePreviewDialog(cropped, res, node, multicastTargets, transport, this);
    connect(dlg, &ImagePreviewDialog::sendRequested, this,
            [this, node, multicastTargets](const QByteArray &imageData, int w, int h,
                                           BleManager::ImageSendMode mode, quint8 imageMode,
                                           const QImage &previewBitmap) {
        if (!multicastTargets.isEmpty()) {
            // 组播发送
            m_ble->sendImageMulticast(multicastTargets, imageData, w, h, imageMode);
            m_connectedPage->addLog(tr("[BLE] 组播图片 %1x%2 → %3 节点")
                                        .arg(w).arg(h).arg(multicastTargets.size()));
            // 保存每个目标节点的图片
            for (quint16 addr : multicastTargets) {
                m_nodeImages[addr] = previewBitmap;
                NodeImageStore::save(addr, previewBitmap);
            }
        } else {
            // 单播发送
            m_ble->sendImage(node.addr, imageData, w, h, mode, imageMode);
            m_connectedPage->addLog(tr("[BLE] 发送图片 0x%1 %2x%3 (%4 B)")
                                        .arg(MeshProtocol::addrToHex4(node.addr))
                                        .arg(w).arg(h).arg(imageData.size()));
            // 保存节点图片
            m_nodeImages[node.addr] = previewBitmap;
            NodeImageStore::save(node.addr, previewBitmap);
        }
    });

    // WiFi 直传
    connect(dlg, &ImagePreviewDialog::wifiSendRequested, this,
            [this, node, multicastTargets](const QByteArray &imageData, int w, int h,
                                           quint8 imageMode, const QImage &previewBitmap) {
        m_connectedPage->addLog(tr("[WiFi] 发送图片 %1x%2 (%3 B)...")
                                    .arg(w).arg(h).arg(imageData.size()));
        m_wifi->sendImage(imageData, w, h, imageMode);

        // 保存节点图片
        if (!multicastTargets.isEmpty()) {
            for (quint16 addr : multicastTargets) {
                m_nodeImages[addr] = previewBitmap;
                NodeImageStore::save(addr, previewBitmap);
            }
        } else {
            m_nodeImages[node.addr] = previewBitmap;
            NodeImageStore::save(node.addr, previewBitmap);
        }
    });

    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->open();
}

void MainWindow::onImageSendDone(const BleManager::ImageSendState &state)
{
    // 传输完成后延迟 2s 刷新拓扑
    if (state.type == BleManager::ImgDone || state.type == BleManager::ImgMulticastDone) {
        QTimer::singleShot(2000, this, [this]() {
            if (m_ble->connState() == BleManager::Connected && !m_ble->isImageBusy())
                m_ble->queryTopology();
        });
    }
}
