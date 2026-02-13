#include "mainwindow.h"
#include "stylemanager.h"

#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QProgressBar>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle(qApp->applicationName());
    resize(420, 700);
    setMinimumSize(360, 560);

    // 创建 BLE 管理器
    m_ble = new BleManager(this);

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

    // 连接后自动查询拓扑（延迟 800ms 等 CCCD 订阅完成）
    m_topoQueryTimer.setSingleShot(true);
    m_topoQueryTimer.setInterval(800);
    connect(&m_topoQueryTimer, &QTimer::timeout, this, [this]() {
        m_ble->queryTopology();
    });
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
    m_scanPage = new ScanPage(m_ble, this);
    m_stack->addWidget(m_scanPage);

    // 页面 1: 连接中
    m_connectingPage = new QWidget(this);
    auto *connectingLayout = new QVBoxLayout(m_connectingPage);
    connectingLayout->setAlignment(Qt::AlignCenter);
    auto *spinner = new QProgressBar(m_connectingPage);
    spinner->setRange(0, 0); // indeterminate
    spinner->setFixedWidth(120);
    spinner->setTextVisible(false);
    m_connectingLabel = new QLabel(tr("Connecting..."), m_connectingPage);
    m_connectingLabel->setObjectName("connectingLabel");
    m_connectingLabel->setAlignment(Qt::AlignCenter);
    connectingLayout->addWidget(spinner, 0, Qt::AlignCenter);
    connectingLayout->addWidget(m_connectingLabel);
    m_stack->addWidget(m_connectingPage);

    // 页面 2: 已连接页
    m_connectedPage = new ConnectedPage(m_ble, this);
    m_stack->addWidget(m_connectedPage);

    m_stack->setCurrentIndex(0);
}

void MainWindow::onConnStateChanged(BleManager::ConnState state)
{
    updateStatusDot(state);

    switch (state) {
    case BleManager::Disconnected:
    case BleManager::Scanning:
        m_stack->setCurrentIndex(0);
        m_topoQueryTimer.stop();
        break;
    case BleManager::Connecting:
        m_connectingLabel->setText(tr("Connecting to %1 ...").arg(m_ble->deviceName()));
        m_stack->setCurrentIndex(1);
        break;
    case BleManager::Connected:
        m_stack->setCurrentIndex(2);
        m_topoQueryTimer.start();
        break;
    }
}

void MainWindow::onDeviceSelected(int index)
{
    m_ble->connectToDevice(index);
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
