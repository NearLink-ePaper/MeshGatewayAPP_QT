#include "senddialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDialogButtonBox>
#include <QFont>

SendDialog::SendDialog(const MeshNode &node, QWidget *parent)
    : QDialog(parent), m_node(node)
{
    setObjectName("sendDialog");
    setWindowTitle(tr("Send to 0x%1").arg(MeshProtocol::addrToHex4(node.addr)));
    setMinimumWidth(320);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 16, 20, 16);
    mainLayout->setSpacing(12);

    // 节点信息
    QString nodeDesc;
    switch (node.hops) {
    case 0:  nodeDesc = tr("Gateway (local)"); break;
    case 1:  nodeDesc = tr("Direct node");     break;
    default: nodeDesc = tr("%1-hop route").arg(node.hops); break;
    }
    auto *descLabel = new QLabel(nodeDesc, this);
    descLabel->setObjectName("sendDialogDesc");
    mainLayout->addWidget(descLabel);

    // 输入框
    m_input = new QLineEdit(this);
    m_input->setObjectName("sendDialogInput");
    m_input->setPlaceholderText(tr("Enter data..."));
    m_input->setMinimumHeight(36);
    mainLayout->addWidget(m_input);

    // 按钮
    auto *btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(12);

    auto *cancelBtn = new QPushButton(tr("Cancel"), this);
    cancelBtn->setObjectName("sendDialogCancel");
    cancelBtn->setCursor(Qt::PointingHandCursor);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    btnLayout->addWidget(cancelBtn);

    btnLayout->addStretch();

    auto *sendBtn = new QPushButton(tr("Send"), this);
    sendBtn->setObjectName("sendDialogSend");
    sendBtn->setCursor(Qt::PointingHandCursor);
    sendBtn->setDefault(true);
    connect(sendBtn, &QPushButton::clicked, this, [this]() {
        if (!m_input->text().trimmed().isEmpty())
            accept();
    });
    btnLayout->addWidget(sendBtn);

    mainLayout->addLayout(btnLayout);

    // 回车发送
    connect(m_input, &QLineEdit::returnPressed, sendBtn, &QPushButton::click);

    m_input->setFocus();
}

QString SendDialog::text() const
{
    return m_input->text().trimmed();
}
