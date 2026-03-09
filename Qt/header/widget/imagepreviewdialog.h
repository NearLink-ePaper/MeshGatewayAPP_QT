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

/**
 * 图片预览对话框
 * 显示原图与二值化结果的并排对比
 * 选择传输模式后发送
 */
class ImagePreviewDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ImagePreviewDialog(const QImage &croppedBitmap,
                                const ImageResolution &resolution,
                                const MeshNode &node,
                                const QList<quint16> &multicastTargets = {},
                                QWidget *parent = nullptr);

signals:
    void sendRequested(const QByteArray &imageData, int width, int height,
                       BleManager::ImageSendMode mode, quint8 imageMode,
                       const QImage &previewBitmap);

private:
    void buildUI();

    MeshNode         m_node;
    QList<quint16>   m_multicastTargets;
    QImage           m_croppedBitmap;
    ImageResolution  m_resolution;
    ProcessedImage   m_processed;
    QComboBox       *m_modeCombo = nullptr;
};

#endif // IMAGEPREVIEWDIALOG_H
