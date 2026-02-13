#include "connectedpage.h"
#include "stylemanager.h"
#include <QDateTime>
#include <QFrame>
#include <QMouseEvent>
#include <QScrollBar>
#include <QStyle>

// ─── NodeCardWidget ─────────────────────────────────────

NodeCardWidget::NodeCardWidget(const MeshNode &node, QWidget *parent)
    : QWidget(parent), m_node(node)
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
    case 0:  hopText = tr("Gateway"); hopObj = "hopGateway"; break;
    case 1:  hopText = tr("Direct");  hopObj = "hopDirect";  break;
    default: hopText = tr("%1 hop(s)").arg(node.hops); hopObj = "hopMulti"; break;
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
}

void NodeCardWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
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

    // 调试信息
    m_debugLabel = new QLabel(m_ble->debugInfo(), this);
    m_debugLabel->setObjectName("debugLabel");
    m_debugLabel->setWordWrap(true);
    mainLayout->addWidget(m_debugLabel);

    // 设备信息卡片
    auto *deviceCard = new QWidget(this);
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

    m_disconnectBtn = new QPushButton(tr("Disconnect"), this);
    m_disconnectBtn->setObjectName("disconnectBtn");
    m_disconnectBtn->setCursor(Qt::PointingHandCursor);
    connect(m_disconnectBtn, &QPushButton::clicked, this, &ConnectedPage::onDisconnectClicked);
    deviceLayout->addWidget(m_disconnectBtn);

    mainLayout->addWidget(deviceCard);

    // 查询拓扑按钮
    m_queryTopoBtn = new QPushButton(tr("Query Nodes"), this);
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
    m_nodeScroll->setMaximumHeight(240);
    m_nodeScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    m_nodeListContainer = new QWidget();
    m_nodeListContainer->setObjectName("nodeGridContainer");
    m_nodeListLayout = new QVBoxLayout(m_nodeListContainer);
    m_nodeListLayout->setSpacing(6);
    m_nodeListLayout->setContentsMargins(0, 0, 0, 0);
    m_nodeScroll->setWidget(m_nodeListContainer);

    // 移动端触摸滚动
    QScroller::grabGesture(m_nodeScroll->viewport(), QScroller::LeftMouseButtonGesture);

    mainLayout->addWidget(m_nodeScroll);

    // 广播输入区域
    auto *broadcastCard = new QWidget(this);
    broadcastCard->setObjectName("broadcastCard");
    auto *bcastLayout = new QHBoxLayout(broadcastCard);
    bcastLayout->setContentsMargins(8, 6, 8, 6);
    bcastLayout->setSpacing(8);

    m_broadcastInput = new QLineEdit(this);
    m_broadcastInput->setObjectName("broadcastInput");
    m_broadcastInput->setPlaceholderText(tr("Enter broadcast data..."));
    connect(m_broadcastInput, &QLineEdit::returnPressed, this, &ConnectedPage::onBroadcastClicked);
    bcastLayout->addWidget(m_broadcastInput, 1);

    m_broadcastBtn = new QPushButton(tr("Send"), this);
    m_broadcastBtn->setObjectName("broadcastBtn");
    m_broadcastBtn->setCursor(Qt::PointingHandCursor);
    connect(m_broadcastBtn, &QPushButton::clicked, this, &ConnectedPage::onBroadcastClicked);
    bcastLayout->addWidget(m_broadcastBtn);

    mainLayout->addWidget(broadcastCard);

    // 通信日志标题行
    auto *logHeader = new QHBoxLayout();
    auto *logTitle = new QLabel(tr("Communication Log"), this);
    logTitle->setObjectName("logTitle");
    logHeader->addWidget(logTitle);
    logHeader->addStretch();
    auto *clearLogBtn = new QPushButton(tr("Clear"), this);
    clearLogBtn->setObjectName("clearLogBtn");
    clearLogBtn->setCursor(Qt::PointingHandCursor);
    connect(clearLogBtn, &QPushButton::clicked, this, &ConnectedPage::onClearLogClicked);
    logHeader->addWidget(clearLogBtn);
    mainLayout->addLayout(logHeader);

    // 通信日志容器
    auto *logContainer = new QWidget(this);
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
        m_gwAddrLabel->setText(tr("Gateway 0x%1").arg(gwHex));

        // 包含网关自身 (hop=0)
        m_nodes.clear();
        m_nodes.append(MeshNode(msg.gatewayAddr, 0));
        m_nodes.append(msg.nodes);
        rebuildNodeList();

        addLog(tr("Topology: Gateway 0x%1, %2 node(s)").arg(gwHex).arg(msg.nodes.size()));
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
    m_ble->queryTopology();
    addLog(QString::fromUtf8("\xe2\x86\x92 ") + tr("Query topology"));
}

void ConnectedPage::onDisconnectClicked()
{
    m_ble->disconnectDevice();
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
        addLog(QString::fromUtf8("\xe2\x86\x92 [") + tr("Broadcast") + "] " + text);
    }
    m_broadcastInput->clear();
}

void ConnectedPage::updateSendPlaceholder()
{
    if (m_selectedNodeIndex >= 0 && m_selectedNodeIndex < m_nodes.size()) {
        QString addr = MeshProtocol::addrToHex4(m_nodes[m_selectedNodeIndex].addr);
        m_broadcastInput->setPlaceholderText(tr("Send to 0x%1...").arg(addr));
    } else {
        m_broadcastInput->setPlaceholderText(tr("Enter broadcast data..."));
    }
}

void ConnectedPage::onDebugInfoChanged(const QString &info)
{
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
        connect(card, &NodeCardWidget::clicked, this, [this, i](const MeshNode &) {
            // 切换选中状态
            if (m_selectedNodeIndex == i)
                m_selectedNodeIndex = -1;   // 取消选中
            else
                m_selectedNodeIndex = i;

            // 更新所有卡片的高亮
            for (int j = 0; j < m_nodeListLayout->count(); ++j) {
                auto *w = m_nodeListLayout->itemAt(j)->widget();
                if (w) {
                    w->setObjectName(j == m_selectedNodeIndex ? "nodeCardSelected" : "nodeCard");
                    w->style()->unpolish(w);
                    w->style()->polish(w);
                }
            }
            updateSendPlaceholder();
        });
        m_nodeListLayout->addWidget(card);
    }
    m_nodeListLayout->addStretch();
    updateSendPlaceholder();
}
