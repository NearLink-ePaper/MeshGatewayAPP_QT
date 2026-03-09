#ifndef CROPIMAGEDIALOG_H
#define CROPIMAGEDIALOG_H

#include <QDialog>
#include <QImage>
#include <QPointF>
#include <QRectF>
#include "imageutils.h"
#include "meshprotocol.h"

class QLabel;
class QPushButton;
class AAButton;

/**
 * 裁剪区域绘制控件
 */
class CropWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CropWidget(QWidget *parent = nullptr);

    void setImage(const QImage &img);
    void setAspectRatio(float ar); // 目标宽高比 (w/h)
    QRectF cropRect() const; // 返回归一化裁剪区域 [0,1]

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    void resetCropRect();
    QRectF imageRect() const;
    QPointF toNorm(const QPointF &widgetPos) const;
    QPointF fromNorm(const QPointF &normPos) const;

    QImage m_image;
    float  m_targetAR = 1.0f; // 裁剪框目标宽高比（归一化空间中）
    // 归一化裁剪框 [0,1]
    float m_cropL = 0, m_cropT = 0, m_cropR = 1, m_cropB = 1;
    // 拖动状态: 0=none 1=move 2=TL 3=TR 4=BL 5=BR
    int    m_dragMode = 0;
    QPointF m_dragStart;
    float m_dragCropL, m_dragCropT, m_dragCropR, m_dragCropB;
};

/**
 * 交互式裁剪对话框
 */
class CropImageDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CropImageDialog(const QImage &originalBitmap, const MeshNode &node,
                              int multicastCount = 0, QWidget *parent = nullptr);

signals:
    void cropConfirmed(const QImage &croppedBitmap, const ImageResolution &resolution);

private slots:
    void onCycleResolution();
    void onRotate();
    void onConfirm();

private:
    MeshNode    m_node;
    QImage      m_rotatedBitmap;
    CropWidget *m_cropWidget;
    AAButton   *m_resBtn = nullptr;
    int         m_resIndex = 0;
    QList<ImageResolution> m_resolutions;
};

#endif // CROPIMAGEDIALOG_H
