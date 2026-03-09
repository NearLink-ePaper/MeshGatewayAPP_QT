#ifndef CONNECTEDPAGE_H
#define CONNECTEDPAGE_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QListWidget>
#include <QScrollArea>
#include <QScroller>
#include <QProgressBar>
#include <QSet>
#include <QTimer>

#include "blemanager.h"
#include "meshprotocol.h"
#include "stylemanager.h"

class QMouseEvent;

/**
 * 可点击的节点卡片 — 封装 MeshNode 并发出 clicked / longPressed 信号
 */
class NodeCardWidget : public AAWidget
{
    Q_OBJECT
public:
    explicit NodeCardWidget(const MeshNode &node, QWidget *parent = nullptr);
    MeshNode node() const { return m_node; }
    void setMulticastSelected(bool sel);
    bool isMulticastSelected() const { return m_mcastSelected; }
signals:
    void clicked(const MeshNode &node);
    void longPressed(const MeshNode &node);
protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
private:
    MeshNode m_node;
    QTimer   m_longPressTimer;
    bool     m_longPressTriggered = false;
    bool     m_mcastSelected = false;
};

/**
 * 连接页面 — 设备信息 + 节点网格 + 广播输入 + 通信日志 + 图片进度条
 */
class ConnectedPage : public QWidget
{
    Q_OBJECT

public:
    explicit ConnectedPage(BleManager *ble, QWidget *parent = nullptr);

    void addLog(const QString &text);
    QList<quint16> multicastTargets() const;
    void clearMulticastSelection();

signals:
    void nodeClicked(const MeshNode &node);
    void multicastImageRequested(const QList<quint16> &targets);

public slots:
    void onMessageReceived(const UpstreamMessage &msg);
    void onImageSendStateChanged(const BleManager::ImageSendState &state);

private slots:
    void onQueryTopoClicked();
    void onDisconnectClicked();
    void onBroadcastClicked();
    void onDebugInfoChanged(const QString &info);
    void onDeviceNameChanged(const QString &name);
    void onClearLogClicked();
    void onNodeLongPressed(const MeshNode &node);
    void onMulticastSendClicked();
    void onCancelImageClicked();

private:
    void rebuildNodeList();
    void updateSendPlaceholder();
    void updateMulticastUI();
    void updateProgressBar(const BleManager::ImageSendState &state);

    BleManager        *m_ble;

    QLabel            *m_debugLabel;
    QLabel            *m_deviceNameLabel;
    QLabel            *m_gwAddrLabel;
    AAButton          *m_disconnectBtn;
    AAButton          *m_queryTopoBtn;
    QVBoxLayout       *m_nodeListLayout;
    QWidget           *m_nodeListContainer;
    QScrollArea       *m_nodeScroll;
    AALineEdit        *m_broadcastInput;
    AAButton          *m_broadcastBtn;
    QListWidget       *m_logList;

    // 图片进度条
    QWidget           *m_progressContainer = nullptr;
    QProgressBar      *m_progressBar = nullptr;
    QLabel            *m_progressLabel = nullptr;
    AAButton          *m_cancelImgBtn = nullptr;

    // 组播
    QWidget           *m_multicastBar = nullptr;
    QLabel            *m_multicastLabel = nullptr;
    AAButton          *m_multicastSendBtn = nullptr;
    AAButton          *m_multicastClearBtn = nullptr;
    QSet<quint16>      m_multicastAddrs;

    quint16            m_gwAddr = 0;
    QList<MeshNode>    m_nodes;
    int                m_selectedNodeIndex = -1;
};

#endif // CONNECTEDPAGE_H
