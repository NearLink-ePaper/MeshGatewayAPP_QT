#include "connectedpage.h"
#include "stylemanager.h"
#include <QDateTime>
#include <QFrame>
#include <QMouseEvent>
#include <QPalette>
#include <QScrollBar>

// ─── NodeCardWidget ─────────────────────────────────────

NodeCardWidget::NodeCardWidget(const MeshNode &node, QWidget *parent)
    : AAWidget(parent), m_node(node)
{
    setObjectName("nodeCard");
    setCursor(Qt::PointingHandCursor);

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(12, 10, 12, 10);
    layout->setSpacing(10);

    // 图标
    auto *iconBg = new QWidget(this);
    iconBg->setFixedSize(36, 36);
    iconBg->setObjectName(node.hops == 0 ? "nodeIconGateway" : "nodeIconNormal");

    auto *iconLabel = new QLabel(iconBg);
    QString svgPath = node.hops == 0 ? ":/img/gateway.svg" : ":/img/chip.svg";
    iconLabel->setPixmap(StyleManager::loadSvgIcon(svgPath, 22));
    iconLabel->setAlignment(Qt::AlignCenter);
    iconLabel->setFixedSize(36, 36);

    layout->addWidget(iconBg);

    // 地址和跳数
    auto *textLayout = new QVBoxLayout();
    textLayout->setSpacing(1);
    auto *addrLabel = new QLabel(QString("0x%1").arg(MeshProtocol::addrToHex4(node.addr)), this);
    addrLabel->setObjectName("nodeAddr");

    QString hopText;
    QString hopObj;
    switch (node.hops) {
    case 0:  hopText = tr("网关"); hopObj = "hopGateway"; break;
    case 1:  hopText = tr("直连"); hopObj = "hopDirect";  break;
    default: hopText = tr("%1 跳").arg(node.hops); hopObj = "hopMulti"; break;
    }
    auto *hopLabel = new QLabel(hopText, this);
    hopLabel->setObjectName(hopObj);

    textLayout->addWidget(addrLabel);
    textLayout->addWidget(hopLabel);
    layout->addLayout(textLayout, 1);

    // 发送图标
    auto *sendIcon = new QLabel(QStringLiteral("\u27A4"), this); // ➤
    sendIcon->setObjectName("sendIcon");
    layout->addWidget(sendIcon);

    // 长按定时器
    m_longPressTimer.setSingleShot(true);
    m_longPressTimer.setInterval(600);
    connect(&m_longPressTimer, &QTimer::timeout, this, [this]() {
        m_longPressTriggered = true;
        emit longPressed(m_node);
    });
}

void NodeCardWidget::setMulticastSelected(bool sel)
{
    m_mcastSelected = sel;
    setObjectName(sel ? "nodeCardMulticast" : "nodeCard");
    style()->unpolish(this);
    style()->polish(this);
}

void NodeCardWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_longPressTriggered = false;
        m_longPressTimer.start();
    }
    AAWidget::mousePressEvent(event);
}

void NodeCardWidget::mouseReleaseEvent(QMouseEvent *event)
{
    m_longPressTimer.stop();
    if (event->button() == Qt::LeftButton && !m_longPressTriggered) {
        emit clicked(m_node);
    }
    QWidget::mouseReleaseEvent(event);
}

// ─── ConnectedPage ──────────────────────────────────────

ConnectedPage::ConnectedPage(BleManager *ble, QWidget *parent)
    : QWidget(parent), m_ble(ble)
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(14, 10, 14, 10);
    mainLayout->setSpacing(10);

    // 调试信息（AAWidget 容器提供抗锯齿圆角）
    auto *debugCard = new AAWidget(this);
    debugCard->setObjectName("debugCard");
    auto *debugCardLayout = new QVBoxLayout(debugCard);
    debugCardLayout->setContentsMargins(12, 10, 12, 10);
    m_debugLabel = new QLabel(m_ble->debugInfo(), debugCard);
    m_debugLabel->setObjectName("debugLabel");
    m_debugLabel->setWordWrap(true);
    debugCardLayout->addWidget(m_debugLabel);
    mainLayout->addWidget(debugCard);

    // 图片传输进度条 (默认隐藏)
    m_progressContainer = new QWidget(this);
    m_progressContainer->setVisible(false);
    auto *progLayout = new QHBoxLayout(m_progressContainer);
    progLayout->setContentsMargins(0, 0, 0, 0);
    progLayout->setSpacing(8);
    m_progressBar = new QProgressBar;
    m_progressBar->setObjectName("imgProgressBar");
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setTextVisible(false);
    m_progressBar->setFixedHeight(dp(6));
    progLayout->addWidget(m_progressBar, 1);
    m_progressLabel = new QLabel;
    m_progressLabel->setObjectName("imgProgressLabel");
    m_progressLabel->setStyleSheet(QStringLiteral("color: #B0BEC5; font-size: %1px;").arg(dp(11)));
    progLayout->addWidget(m_progressLabel);
    m_cancelImgBtn = new AAButton(tr("取消"), this);
    m_cancelImgBtn->setObjectName("cancelImgBtn");
    m_cancelImgBtn->setCursor(Qt::PointingHandCursor);
    m_cancelImgBtn->setStyleSheet(QStringLiteral(
        "AAButton { background: rgba(255,82,82,0.2); color: #FF5252; border-radius: %1px; "
        "padding: %2px %3px; font-size: %4px; }"
        "AAButton:hover { background: rgba(255,82,82,0.4); }")
        .arg(dp(4)).arg(dp(4)).arg(dp(10)).arg(dp(11)));
    connect(m_cancelImgBtn, &QPushButton::clicked, this, &ConnectedPage::onCancelImageClicked);
    progLayout->addWidget(m_cancelImgBtn);
    mainLayout->addWidget(m_progressContainer);

    // 设备信息卡片
    auto *deviceCard = new AAWidget(this);
    deviceCard->setObjectName("deviceInfoCard");
    auto *deviceLayout = new QHBoxLayout(deviceCard);
    deviceLayout->setContentsMargins(12, 10, 12, 10);

    auto *infoLayout = new QVBoxLayout();
    m_deviceNameLabel = new QLabel(m_ble->deviceName(), this);
    m_deviceNameLabel->setObjectName("connDeviceName");
    m_gwAddrLabel = new QLabel("", this);
    m_gwAddrLabel->setObjectName("connGwAddr");
    infoLayout->addWidget(m_deviceNameLabel);
    infoLayout->addWidget(m_gwAddrLabel);
    deviceLayout->addLayout(infoLayout, 1);

    m_disconnectBtn = new AAButton(tr("断开连接"), this);
    m_disconnectBtn->setObjectName("disconnectBtn");
    m_disconnectBtn->setCursor(Qt::PointingHandCursor);
    connect(m_disconnectBtn, &QPushButton::clicked, this, &ConnectedPage::onDisconnectClicked);
    deviceLayout->addWidget(m_disconnectBtn);

    mainLayout->addWidget(deviceCard);

    // 查询拓扑按钮
    m_queryTopoBtn = new AAButton(tr("查询节点"), this);
    m_queryTopoBtn->setObjectName("queryTopoBtn");
    m_queryTopoBtn->setMinimumHeight(40);
    m_queryTopoBtn->setCursor(Qt::PointingHandCursor);
    connect(m_queryTopoBtn, &QPushButton::clicked, this, &ConnectedPage::onQueryTopoClicked);
    mainLayout->addWidget(m_queryTopoBtn);

    // 节点列表区域（垂直滚动，支持触摸滑动选择）
    m_nodeScroll = new QScrollArea(this);
    m_nodeScroll->setWidgetResizable(true);
    m_nodeScroll->setFrameShape(QFrame::NoFrame);
    m_nodeScroll->setObjectName("nodeScrollArea");
    m_nodeScroll->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_nodeScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    m_nodeListContainer = new QWidget();
    m_nodeListContainer->setObjectName("nodeGridContainer");
    m_nodeListLayout = new QVBoxLayout(m_nodeListContainer);
    m_nodeListLayout->setSpacing(6);
    m_nodeListLayout->setContentsMargins(0, 0, 0, 0);
    m_nodeScroll->setWidget(m_nodeListContainer);

    // 移动端触摸滚动（使用 TouchGesture 避免拦截 QLineEdit 输入法事件）
    QScroller::grabGesture(m_nodeScroll->viewport(), QScroller::TouchGesture);

    mainLayout->addWidget(m_nodeScroll, 2);

    // 组播操作栏 (默认隐藏)
    m_multicastBar = new QWidget(this);
    m_multicastBar->setVisible(false);
    auto *mcastLayout = new QHBoxLayout(m_multicastBar);
    mcastLayout->setContentsMargins(0, 0, 0, 0);
    mcastLayout->setSpacing(8);
    m_multicastLabel = new QLabel;
    m_multicastLabel->setStyleSheet(QStringLiteral("color: #CE93D8; font-size: %1px;").arg(dp(12)));
    mcastLayout->addWidget(m_multicastLabel, 1);
    m_multicastSendBtn = new AAButton(tr("组播发送"), this);
    m_multicastSendBtn->setCursor(Qt::PointingHandCursor);
    m_multicastSendBtn->setStyleSheet(QStringLiteral(
        "AAButton { background: rgba(156,39,176,0.25); color: #CE93D8; font-weight: bold; "
        "border-radius: %1px; padding: %2px %3px; font-size: %4px; }"
        "AAButton:hover { background: rgba(156,39,176,0.45); }")
        .arg(dp(6)).arg(dp(6)).arg(dp(14)).arg(dp(12)));
    connect(m_multicastSendBtn, &QPushButton::clicked, this, &ConnectedPage::onMulticastSendClicked);
    mcastLayout->addWidget(m_multicastSendBtn);
    m_multicastClearBtn = new AAButton(tr("清除选择"), this);
    m_multicastClearBtn->setCursor(Qt::PointingHandCursor);
    m_multicastClearBtn->setStyleSheet(QStringLiteral("color: #888; font-size: %1px;").arg(dp(11)));
    connect(m_multicastClearBtn, &QPushButton::clicked, this, [this]() {
        clearMulticastSelection();
    });
    mcastLayout->addWidget(m_multicastClearBtn);
    mainLayout->addWidget(m_multicastBar);

    // 广播输入区域
    auto *broadcastCard = new AAWidget(this);
    broadcastCard->setObjectName("broadcastCard");
    auto *bcastLayout = new QHBoxLayout(broadcastCard);
    bcastLayout->setContentsMargins(8, 6, 8, 6);
    bcastLayout->setSpacing(8);

    m_broadcastInput = new AALineEdit(this);
    m_broadcastInput->setObjectName("broadcastInput");
    m_broadcastInput->setPlaceholderText(tr("输入广播数据..."));
    m_broadcastInput->setAttribute(Qt::WA_InputMethodEnabled, true);
    m_broadcastInput->setInputMethodHints(Qt::ImhNone);
    // IME 预编辑文字高亮色，避免深色背景下看不清
    QPalette inputPal = m_broadcastInput->palette();
    inputPal.setColor(QPalette::Highlight, QColor(88, 166, 255, 140));
    inputPal.setColor(QPalette::HighlightedText, Qt::white);
    m_broadcastInput->setPalette(inputPal);
    connect(m_broadcastInput, &QLineEdit::returnPressed, this, &ConnectedPage::onBroadcastClicked);
    bcastLayout->addWidget(m_broadcastInput, 1);

    m_broadcastBtn = new AAButton(tr("发送"), this);
    m_broadcastBtn->setObjectName("broadcastBtn");
    m_broadcastBtn->setCursor(Qt::PointingHandCursor);
    connect(m_broadcastBtn, &QPushButton::clicked, this, &ConnectedPage::onBroadcastClicked);
    bcastLayout->addWidget(m_broadcastBtn);

    mainLayout->addWidget(broadcastCard);

    // 通信日志标题行
    auto *logHeader = new QHBoxLayout();
    auto *logTitle = new QLabel(tr("通信日志"), this);
    logTitle->setObjectName("logTitle");
    logHeader->addWidget(logTitle);
    logHeader->addStretch();
    auto *clearLogBtn = new AAButton(tr("清除"), this);
    clearLogBtn->setObjectName("clearLogBtn");
    clearLogBtn->setCursor(Qt::PointingHandCursor);
    connect(clearLogBtn, &QPushButton::clicked, this, &ConnectedPage::onClearLogClicked);
    logHeader->addWidget(clearLogBtn);
    mainLayout->addLayout(logHeader);

    // 通信日志容器
    auto *logContainer = new AAWidget(this);
    logContainer->setObjectName("logContainer");
    auto *logContainerLayout = new QVBoxLayout(logContainer);
    logContainerLayout->setContentsMargins(6, 6, 6, 6);
    logContainerLayout->setSpacing(0);

    m_logList = new QListWidget(logContainer);
    m_logList->setObjectName("logList");
    m_logList->setSelectionMode(QAbstractItemView::NoSelection);
    m_logList->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    logContainerLayout->addWidget(m_logList);

    mainLayout->addWidget(logContainer, 1);

    // 信号连接
    connect(m_ble, &BleManager::debugInfoChanged, this, &ConnectedPage::onDebugInfoChanged);
    connect(m_ble, &BleManager::deviceNameChanged, this, &ConnectedPage::onDeviceNameChanged);
    connect(m_ble, &BleManager::messageReceived, this, &ConnectedPage::onMessageReceived);
    connect(m_ble, &BleManager::imageSendStateChanged, this, &ConnectedPage::onImageSendStateChanged);
}

void ConnectedPage::onClearLogClicked()
{
    m_logList->clear();
}

void ConnectedPage::addLog(const QString &text)
{
    QString time = QDateTime::currentDateTime().toString("HH:mm:ss");
    auto *item = new QListWidgetItem(QString("%1  %2").arg(time, text));

    if (text.startsWith(QStringLiteral("\u2192")) || text.startsWith(QString::fromUtf8("→"))) {
        item->setForeground(QColor(0x58, 0xA6, 0xFF));   // accent blue — outgoing
    } else if (text.startsWith(QStringLiteral("\u2190")) || text.startsWith(QString::fromUtf8("←"))) {
        item->setForeground(QColor(0x3F, 0xB9, 0x50));   // green — incoming
    } else {
        item->setForeground(QColor(0x8B, 0x94, 0x9E));   // muted
    }

    m_logList->insertItem(0, item);

    // 保留最近 50 条
    while (m_logList->count() > 50) {
        delete m_logList->takeItem(m_logList->count() - 1);
    }
}

void ConnectedPage::onMessageReceived(const UpstreamMessage &msg)
{
    switch (msg.type) {
    case UpstreamMessage::Topology: {
        m_gwAddr = msg.gatewayAddr;
        QString gwHex = MeshProtocol::addrToHex4(msg.gatewayAddr);
        m_ble->updateDeviceName(QString("sle_gw_%1").arg(gwHex));
        m_gwAddrLabel->setText(tr("网关 0x%1").arg(gwHex));

        // 包含网关自身 (hop=0)
        m_nodes.clear();
        m_nodes.append(MeshNode(msg.gatewayAddr, 0));
        m_nodes.append(msg.nodes);
        rebuildNodeList();

        addLog(tr("拓扑: 网关 0x%1, %2 个节点").arg(gwHex).arg(msg.nodes.size()));
        break;
    }
    case UpstreamMessage::DataFromNode: {
        QString txt = MeshProtocol::decodePayloadToStringOrHex(msg.payload);
        addLog(QString::fromUtf8("\xe2\x86\x90 [0x%1] %2")
                   .arg(MeshProtocol::addrToHex4(msg.srcAddr), txt));
        break;
    }
    default:
        break;
    }
}

void ConnectedPage::onQueryTopoClicked()
{
    if (m_wifiMode) {
        emit wifiTopoQueryRequested();
        addLog(QString::fromUtf8("\xe2\x86\x92 ") + tr("[WiFi] 查询节点..."));
    } else {
        m_ble->queryTopology();
        addLog(QString::fromUtf8("\xe2\x86\x92 ") + tr("查询拓扑"));
    }
}

void ConnectedPage::onDisconnectClicked()
{
    if (m_wifiMode) {
        emit wifiDisconnectRequested();
    } else {
        m_ble->disconnectDevice();
    }
}

void ConnectedPage::onBroadcastClicked()
{
    QString text = m_broadcastInput->text().trimmed();
    if (text.isEmpty()) return;

    // 有选中节点 → 单播；否则 → 广播
    if (m_selectedNodeIndex >= 0 && m_selectedNodeIndex < m_nodes.size()) {
        const MeshNode &node = m_nodes[m_selectedNodeIndex];
        m_ble->sendToNode(node.addr, text.toUtf8());
        addLog(QString::fromUtf8("\xe2\x86\x92 [0x%1] %2")
                   .arg(MeshProtocol::addrToHex4(node.addr), text));
    } else {
        m_ble->broadcast(text.toUtf8());
        addLog(QString::fromUtf8("\xe2\x86\x92 [") + tr("广播") + "] " + text);
    }
    m_broadcastInput->clear();
}

void ConnectedPage::updateSendPlaceholder()
{
    if (m_selectedNodeIndex >= 0 && m_selectedNodeIndex < m_nodes.size()) {
        QString addr = MeshProtocol::addrToHex4(m_nodes[m_selectedNodeIndex].addr);
        m_broadcastInput->setPlaceholderText(tr("发送至 0x%1...").arg(addr));
    } else {
        m_broadcastInput->setPlaceholderText(tr("输入广播数据..."));
    }
}

void ConnectedPage::onDebugInfoChanged(const QString &info)
{
    if (m_wifiMode) return;  /* WiFi 模式不覆盖 */
    m_debugLabel->setText(info);
}

void ConnectedPage::onDeviceNameChanged(const QString &name)
{
    m_deviceNameLabel->setText(name);
}

void ConnectedPage::rebuildNodeList()
{
    // 清除旧节点卡片
    QLayoutItem *child;
    while ((child = m_nodeListLayout->takeAt(0)) != nullptr) {
        if (child->widget()) child->widget()->deleteLater();
        delete child;
    }
    m_selectedNodeIndex = -1;

    for (int i = 0; i < m_nodes.size(); ++i) {
        auto *card = new NodeCardWidget(m_nodes[i], m_nodeListContainer);
        // 点击选中节点（而非直接发送）
        connect(card, &NodeCardWidget::clicked, this, [this, i](const MeshNode &node) {
            if (!m_multicastAddrs.isEmpty()) {
                // 组播模式下点击切换选中
                onNodeLongPressed(node);
                return;
            }
            // 设置选中节点（用于广播输入框上下文）
            m_selectedNodeIndex = (m_selectedNodeIndex == i) ? -1 : i;
            for (int j = 0; j < m_nodeListLayout->count(); ++j) {
                auto *w = qobject_cast<NodeCardWidget *>(m_nodeListLayout->itemAt(j)->widget());
                if (w && !w->isMulticastSelected()) {
                    w->setObjectName(j == m_selectedNodeIndex ? "nodeCardSelected" : "nodeCard");
                    w->style()->unpolish(w);
                    w->style()->polish(w);
                }
            }
            updateSendPlaceholder();
            // 通知 MainWindow 打开节点操作对话框
            emit nodeClicked(node);
        });
        // 长按进入组播选择
        connect(card, &NodeCardWidget::longPressed, this, &ConnectedPage::onNodeLongPressed);
        m_nodeListLayout->addWidget(card);
    }
    m_nodeListLayout->addStretch();
    updateSendPlaceholder();
}

// ─── 组播选择 ─────────────────────────────────────

void ConnectedPage::onNodeLongPressed(const MeshNode &node)
{
    if (node.hops == 0) return;  // 网关不能作为组播目标
    quint16 addr = node.addr;

    if (m_multicastAddrs.contains(addr))
        m_multicastAddrs.remove(addr);
    else if (m_multicastAddrs.size() < 8)
        m_multicastAddrs.insert(addr);

    // 更新卡片视觉
    for (int j = 0; j < m_nodeListLayout->count(); ++j) {
        auto *w = qobject_cast<NodeCardWidget *>(m_nodeListLayout->itemAt(j)->widget());
        if (w) w->setMulticastSelected(m_multicastAddrs.contains(w->node().addr));
    }
    updateMulticastUI();
}

void ConnectedPage::updateMulticastUI()
{
    bool hasMcast = !m_multicastAddrs.isEmpty();
    m_multicastBar->setVisible(hasMcast);
    if (hasMcast) {
        QStringList addrs;
        for (quint16 a : m_multicastAddrs)
            addrs.append(QStringLiteral("0x%1").arg(a, 4, 16, QChar('0')).toUpper());
        m_multicastLabel->setText(tr("组播: %1 (%2/8)").arg(addrs.join(", ")).arg(m_multicastAddrs.size()));
    }
}

QList<quint16> ConnectedPage::multicastTargets() const
{
    return QList<quint16>(m_multicastAddrs.begin(), m_multicastAddrs.end());
}

void ConnectedPage::clearMulticastSelection()
{
    m_multicastAddrs.clear();
    for (int j = 0; j < m_nodeListLayout->count(); ++j) {
        auto *w = qobject_cast<NodeCardWidget *>(m_nodeListLayout->itemAt(j)->widget());
        if (w) w->setMulticastSelected(false);
    }
    updateMulticastUI();
}

void ConnectedPage::onMulticastSendClicked()
{
    if (m_multicastAddrs.isEmpty()) return;
    emit multicastImageRequested(multicastTargets());
}

void ConnectedPage::onCancelImageClicked()
{
    if (m_wifiMode) {
        emit wifiCancelRequested();
    } else {
        m_ble->cancelImageSend();
    }
}

// ─── 图片传输进度 ───────────────────────────────

void ConnectedPage::onImageSendStateChanged(const BleManager::ImageSendState &state)
{
    updateProgressBar(state);
}

void ConnectedPage::updateProgressBar(const BleManager::ImageSendState &state)
{
    using S = BleManager::ImageSendStateType;
    switch (state.type) {
    case S::ImgIdle:
        m_progressContainer->setVisible(false);
        break;
    case S::ImgSending: {
        m_progressContainer->setVisible(true);
        int pct = (state.total > 0) ? state.seq * 100 / state.total : 0;
        m_progressBar->setValue(pct);
        m_progressLabel->setText(tr("上传 %1/%2").arg(state.seq).arg(state.total));
        m_cancelImgBtn->setVisible(true);
        break;
    }
    case S::ImgWaitingAck: {
        m_progressContainer->setVisible(true);
        int pct = (state.total > 0) ? state.seq * 100 / state.total : 50;
        m_progressBar->setValue(pct);
        m_progressLabel->setText(tr("等待应答 %1/%2").arg(state.seq + 1).arg(state.total));
        m_cancelImgBtn->setVisible(true);
        break;
    }
    case S::ImgMeshTransfer: {
        m_progressContainer->setVisible(true);
        int pct = (state.total > 0) ? state.rxCount * 100 / state.total : 50;
        m_progressBar->setValue(pct);
        QString phase = (state.phase == 0) ? tr("传输中") : tr("重传中");
        m_progressLabel->setText(tr("Mesh转发 %1 %2/%3").arg(phase).arg(state.rxCount).arg(state.total));
        m_cancelImgBtn->setVisible(true);
        break;
    }
    case S::ImgFinishing:
        m_progressContainer->setVisible(true);
        m_progressBar->setValue(100);
        m_progressLabel->setText(tr("等待确认..."));
        m_cancelImgBtn->setVisible(true);
        break;
    case S::ImgDone:
        m_progressContainer->setVisible(true);
        m_progressBar->setValue(state.success ? 100 : 0);
        m_progressLabel->setText(state.msg);
        m_cancelImgBtn->setVisible(false);
        // 3秒后自动隐藏
        QTimer::singleShot(3000, this, [this]() {
            if (m_ble->imageSendState().type == BleManager::ImgDone ||
                m_ble->imageSendState().type == BleManager::ImgCancelled ||
                m_ble->imageSendState().type == BleManager::ImgMulticastDone) {
                m_progressContainer->setVisible(false);
                m_ble->resetImageSendState();
            }
        });
        break;
    case S::ImgCancelled:
        m_progressContainer->setVisible(true);
        m_progressBar->setValue(0);
        m_progressLabel->setText(tr("已取消"));
        m_cancelImgBtn->setVisible(false);
        QTimer::singleShot(2000, this, [this]() {
            if (m_ble->imageSendState().type == BleManager::ImgCancelled) {
                m_progressContainer->setVisible(false);
                m_ble->resetImageSendState();
            }
        });
        break;
    case S::ImgMulticastTransfer: {
        m_progressContainer->setVisible(true);
        int pct = (state.mcastTotalTargets > 0) ? state.mcastCompleted * 100 / state.mcastTotalTargets : 50;
        m_progressBar->setValue(pct);
        m_progressLabel->setText(tr("组播 %1/%2").arg(state.mcastCompleted).arg(state.mcastTotalTargets));
        m_cancelImgBtn->setVisible(true);
        break;
    }
    case S::ImgMulticastDone: {
        m_progressContainer->setVisible(true);
        m_progressBar->setValue(100);
        int ok = 0;
        for (auto it = state.mcastResults.begin(); it != state.mcastResults.end(); ++it)
            if (it.value() == 0) ++ok;
        m_progressLabel->setText(tr("组播完成: %1/%2 成功").arg(ok).arg(state.mcastResults.size()));
        m_cancelImgBtn->setVisible(false);
        QTimer::singleShot(3000, this, [this]() {
            if (m_ble->imageSendState().type == BleManager::ImgMulticastDone) {
                m_progressContainer->setVisible(false);
                m_ble->resetImageSendState();
            }
        });
        break;
    }
    }
}

// ─── WiFi 进度 ───────────────────────────────────────────

void ConnectedPage::updateWifiProgress(int bytesSent, int totalBytes)
{
    if (totalBytes <= 0) return;
    m_progressContainer->setVisible(true);
    int pct = bytesSent * 100 / totalBytes;
    m_progressBar->setValue(pct);
    m_progressLabel->setText(tr("上传 %1/%2 KB")
        .arg(bytesSent / 1024).arg(totalBytes / 1024));
    m_cancelImgBtn->setVisible(true);
}

void ConnectedPage::onWifiStateChanged(SocketTransport::State state, const QString &msg)
{
    switch (state) {
    case SocketTransport::Connecting:
        m_progressContainer->setVisible(true);
        m_progressBar->setValue(0);
        m_progressLabel->setText(tr("正在连接..."));
        m_cancelImgBtn->setVisible(true);
        break;
    case SocketTransport::Sending:
        m_progressContainer->setVisible(true);
        m_progressLabel->setText(tr("发送中..."));
        m_cancelImgBtn->setVisible(true);
        break;
    case SocketTransport::WaitingReply:
        m_progressBar->setValue(100);
        m_progressLabel->setText(tr("等待设备响应..."));
        m_cancelImgBtn->setVisible(false);
        break;
    case SocketTransport::Done:
        m_progressBar->setValue(100);
        m_progressLabel->setText(msg.isEmpty() ? tr("发送成功 ✔") : msg);
        m_cancelImgBtn->setVisible(false);
        QTimer::singleShot(3000, this, [this]() {
            m_progressContainer->setVisible(false);
        });
        break;
    case SocketTransport::Error:
        m_progressBar->setValue(0);
        m_progressLabel->setText(msg.isEmpty() ? tr("发送失败") : tr("失败: %1").arg(msg));
        m_cancelImgBtn->setVisible(false);
        QTimer::singleShot(3000, this, [this]() {
            m_progressContainer->setVisible(false);
        });
        break;
    default:
        break;
    }
}

// ─── WiFi 模式 ────────────────────────────────────────────

void ConnectedPage::resetMode()
{
    m_wifiMode = false;
    m_queryTopoBtn->setVisible(true);
    m_queryTopoBtn->setText(tr("查询拓扑"));
    m_progressContainer->setVisible(false);
}

void ConnectedPage::setWifiMode(const WifiDevice &device)
{
    m_wifiMode = true;
    // 设备信息卡片显示 WiFi 设备
    m_deviceNameLabel->setText(device.name);
    m_gwAddrLabel->setText(
        QStringLiteral("%1:%2  [WiFi]").arg(device.host).arg(device.port));
    m_debugLabel->setText(tr("收到: %1\n%2:%3  [WiFi]").arg(device.name).arg(device.host).arg(device.port));

    // WiFi 模式也支持查询节点
    m_queryTopoBtn->setVisible(true);
    m_queryTopoBtn->setText(tr("查询节点"));

    // 从设备名称解析网关 Mesh 地址: "sle_gw_006E" → 0x006E
    bool ok = false;
    quint16 gwAddr = device.name.right(4).toUShort(&ok, 16);
    if (!ok) gwAddr = 0x0000;
    m_wifiGwAddr = gwAddr;

    // 初始只放一张"网关本地"卡片（hops=0），用户可点击后发图给网关本地 ePaper
    m_nodes.clear();
    m_nodes.append(MeshNode(gwAddr, 0));
    rebuildNodeList();
}

void ConnectedPage::onWifiTopologyReceived(const QList<MeshNode> &nodes)
{
    // 保留 hops=0 的本地网关节点，追加查询到的 Mesh 节点
    m_nodes.clear();
    m_nodes.append(MeshNode(m_wifiGwAddr, 0));   /* 网关本地 ePaper */
    for (const MeshNode &n : nodes) {
        if (n.addr != m_wifiGwAddr) {
            m_nodes.append(n);
        }
    }
    rebuildNodeList();
    addLog(tr("[WiFi] 发现 %1 个 Mesh 节点").arg(nodes.size()));
}
