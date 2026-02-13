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

#include "blemanager.h"
#include "meshprotocol.h"
#include "stylemanager.h"

class QMouseEvent;

/**
 * 可点击的节点卡片 — 封装 MeshNode 并发出 clicked 信号
 */
class NodeCardWidget : public AAWidget
{
    Q_OBJECT
public:
    explicit NodeCardWidget(const MeshNode &node, QWidget *parent = nullptr);
    MeshNode node() const { return m_node; }
signals:
    void clicked(const MeshNode &node);
protected:
    void mouseReleaseEvent(QMouseEvent *event) override;
private:
    MeshNode m_node;
};

/**
 * 连接页面 — 设备信息 + 节点网格 + 广播输入 + 通信日志
 */
class ConnectedPage : public QWidget
{
    Q_OBJECT

public:
    explicit ConnectedPage(BleManager *ble, QWidget *parent = nullptr);

    void addLog(const QString &text);

signals:
    void nodeClicked(const MeshNode &node);

public slots:
    void onMessageReceived(const UpstreamMessage &msg);

private slots:
    void onQueryTopoClicked();
    void onDisconnectClicked();
    void onBroadcastClicked();
    void onDebugInfoChanged(const QString &info);
    void onDeviceNameChanged(const QString &name);
    void onClearLogClicked();

private:
    void rebuildNodeList();
    void updateSendPlaceholder();

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

    quint16            m_gwAddr = 0;
    QList<MeshNode>    m_nodes;
    int                m_selectedNodeIndex = -1;
};

#endif // CONNECTEDPAGE_H
