#ifndef NODEACTIONDIALOG_H
#define NODEACTIONDIALOG_H

#include <QDialog>
#include <QImage>
#include "meshprotocol.h"

class QLabel;
class QPushButton;

/**
 * 节点操作选择对话框
 * 点击节点后弹出，选择发送文本或发送图片
 * 可显示上次发送的二值化图片预览
 */
class NodeActionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit NodeActionDialog(const MeshNode &node, const QImage &lastSentBitmap = QImage(),
                              QWidget *parent = nullptr);

signals:
    void sendTextRequested(const MeshNode &node);
    void sendImageRequested(const MeshNode &node);

private:
    MeshNode m_node;
};

#endif // NODEACTIONDIALOG_H
