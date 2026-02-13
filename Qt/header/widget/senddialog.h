#ifndef SENDDIALOG_H
#define SENDDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>

#include "meshprotocol.h"

/**
 * 单播发送对话框 — 向指定节点发送数据
 */
class SendDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SendDialog(const MeshNode &node, QWidget *parent = nullptr);

    QString text() const;

private:
    MeshNode    m_node;
    QLineEdit  *m_input;
};

#endif // SENDDIALOG_H
