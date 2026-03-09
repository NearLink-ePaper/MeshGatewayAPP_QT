#include "senddialog.h"
#include "stylemanager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDialogButtonBox>
#include <QFont>

SendDialog::SendDialog(const MeshNode &node, QWidget *parent)
    : QDialog(parent), m_node(node)
{
    setObjectName("sendDialog");
    setWindowTitle(tr("发送至 0x%1").arg(MeshProtocol::addrToHex4(node.addr)));
    setMinimumWidth(320);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 16, 20, 16);
    mainLayout->setSpacing(12);

    // 节点信息
    QString nodeDesc;
    switch (node.hops) {
    case 0:  nodeDesc = tr("网关（本机）"); break;
    case 1:  nodeDesc = tr("直连节点");     break;
    default: nodeDesc = tr("%1 跳路由").arg(node.hops); break;
    }
    auto *descLabel = new QLabel(nodeDesc, this);
    descLabel->setObjectName("sendDialogDesc");
    mainLayout->addWidget(descLabel);

    // 输入框
    m_input = new AALineEdit(this);
    m_input->setObjectName("sendDialogInput");
    m_input->setPlaceholderText(tr("输入数据..."));
    m_input->setMinimumHeight(36);
    mainLayout->addWidget(m_input);

    // 按钮
    auto *btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(12);

    auto *cancelBtn = new AAButton(tr("取消"), this);
    cancelBtn->setObjectName("sendDialogCancel");
    cancelBtn->setCursor(Qt::PointingHandCursor);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    btnLayout->addWidget(cancelBtn);

    btnLayout->addStretch();

    auto *sendBtn = new AAButton(tr("发送"), this);
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
