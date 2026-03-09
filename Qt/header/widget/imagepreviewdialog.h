#ifndef IMAGEPREVIEWDIALOG_H
#define IMAGEPREVIEWDIALOG_H

#include <QDialog>
#include <QImage>
#include "imageutils.h"
#include "meshprotocol.h"
#include "blemanager.h"

class QLabel;
class QComboBox;
class QPushButton;
class QVBoxLayout;

/**
 * 图片预览对话框
 * 显示原图与 JPEG 压缩结果的并排对比
 * 选择传输模式后发送
 */
class ImagePreviewDialog : public QDialog
{
    Q_OBJECT

public:
    enum TransportMode { BleTransport, WifiTransport };

    explicit ImagePreviewDialog(const QImage &croppedBitmap,
                                const ImageResolution &resolution,
                                const MeshNode &node,
                                const QList<quint16> &multicastTargets = {},
                                TransportMode transport = BleTransport,
                                QWidget *parent = nullptr);

signals:
    void sendRequested(const QByteArray &imageData, int width, int height,
                       BleManager::ImageSendMode mode, quint8 imageMode,
                       const QImage &previewBitmap);
    void wifiSendRequested(const QByteArray &imageData, int width, int height,
                           quint8 imageMode, const QImage &previewBitmap);

private:
    void buildUI();
    void rebuildForQuality(int quality);

    MeshNode         m_node;
    QList<quint16>   m_multicastTargets;
    QImage           m_croppedBitmap;
    ImageResolution  m_resolution;
    ProcessedImage   m_processed;
    TransportMode    m_transport = BleTransport;
    QComboBox       *m_modeCombo = nullptr;
    int              m_jpegQuality = 50;
    QVBoxLayout     *m_rootLayout = nullptr;
};

#endif // IMAGEPREVIEWDIALOG_H
