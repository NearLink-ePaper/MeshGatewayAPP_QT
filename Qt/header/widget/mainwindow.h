#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStackedWidget>
#include <QLabel>
#include <QTimer>
#include <QMap>
#include <QImage>

#include "blemanager.h"
#include "scanpage.h"
#include "connectedpage.h"
#include "senddialog.h"
#include "nodeactiondialog.h"
#include "cropimagedialog.h"
#include "imagepreviewdialog.h"
#include "nodeimagestore.h"

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
    void onNodeClicked(const MeshNode &node);
    void onSendTextToNode(const MeshNode &node);
    void onSendImageToNode(const MeshNode &node);
    void onMulticastImageRequested(const QList<quint16> &targets);
    void onImageSendDone(const BleManager::ImageSendState &state);

private:
    void setupTopBar();
    void setupPages();
    void updateStatusDot(BleManager::ConnState state);
    void openImagePicker(const MeshNode &node, const QList<quint16> &multicastTargets = {});
    void openCropDialog(const QImage &image, const MeshNode &node, const QList<quint16> &multicastTargets);
    void openPreviewDialog(const QImage &cropped, const ImageResolution &res,
                           const MeshNode &node, const QList<quint16> &multicastTargets);

    BleManager      *m_ble;
    QStackedWidget  *m_stack;
    ScanPage        *m_scanPage;
    ConnectedPage   *m_connectedPage;
    QWidget         *m_connectingPage;
    QLabel          *m_statusDot;
    QLabel          *m_connectingLabel;
    QTimer           m_topoQueryTimer;     // 连接后首次查询
    QTimer           m_autoTopoTimer;      // 30s 自动刷新
    QMap<quint16, QImage> m_nodeImages;    // 节点图片缓存
    MeshNode         m_currentNode;        // 当前操作节点
    QList<quint16>   m_currentMcastTargets;
};

#endif // MAINWINDOW_H
