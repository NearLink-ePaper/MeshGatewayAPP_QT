#include "nodeactiondialog.h"
#include "stylemanager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QPixmap>

NodeActionDialog::NodeActionDialog(const MeshNode &node, const QImage &lastSentBitmap, QWidget *parent)
    : QDialog(parent), m_node(node)
{
    setWindowTitle(QStringLiteral("0x%1").arg(node.addr, 4, 16, QChar('0')).toUpper());
    setMinimumWidth(dp(280));

    auto *layout = new QVBoxLayout(this);
    layout->setSpacing(dp(12));

    // 节点信息
    QString hopsText;
    switch (node.hops) {
    case 0:  hopsText = tr("Gateway (local)"); break;
    case 1:  hopsText = tr("Direct connection"); break;
    default: hopsText = tr("%1 hops routing").arg(node.hops); break;
    }
    auto *hopsLabel = new QLabel(hopsText);
    hopsLabel->setStyleSheet(QStringLiteral("color: #888; font-size: %1px;").arg(dp(12)));
    layout->addWidget(hopsLabel);

    // 上次发送的图片预览
    if (!lastSentBitmap.isNull()) {
        auto *prevTitle = new QLabel(tr("Last sent image"));
        prevTitle->setStyleSheet(QStringLiteral("color: #888; font-size: %1px;").arg(dp(11)));
        layout->addWidget(prevTitle);

        auto *prevLabel = new QLabel;
        QPixmap pix = QPixmap::fromImage(lastSentBitmap);
        int maxW = dp(160);
        int maxH = dp(200);
        pix = pix.scaled(maxW, maxH, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        prevLabel->setPixmap(pix);
        prevLabel->setAlignment(Qt::AlignCenter);
        prevLabel->setStyleSheet(QStringLiteral("border: 1px solid #444; border-radius: %1px; "
                                                 "background: white; padding: %2px;")
                                 .arg(dp(6)).arg(dp(4)));
        layout->addWidget(prevLabel, 0, Qt::AlignCenter);
    } else {
        auto *noPrev = new QLabel(tr("No send history"));
        noPrev->setStyleSheet(QStringLiteral("color: #555; font-size: %1px;").arg(dp(11)));
        noPrev->setAlignment(Qt::AlignCenter);
        layout->addWidget(noPrev);
    }

    // 操作按钮
    auto *btnRow = new QHBoxLayout;
    btnRow->setSpacing(dp(10));

    auto *textBtn = new AAButton(tr("Send Text"));
    textBtn->setMinimumHeight(dp(64));
    textBtn->setStyleSheet(QStringLiteral(
        "AAButton { background: rgba(79,195,247,0.15); color: white; font-weight: bold; "
        "border-radius: %1px; font-size: %2px; }"
        "AAButton:hover { background: rgba(79,195,247,0.30); }")
        .arg(dp(10)).arg(dp(13)));
    connect(textBtn, &QPushButton::clicked, this, [this]() {
        emit sendTextRequested(m_node);
        accept();
    });
    btnRow->addWidget(textBtn);

    auto *imgBtn = new AAButton(tr("Send Image"));
    imgBtn->setMinimumHeight(dp(64));
    imgBtn->setStyleSheet(QStringLiteral(
        "AAButton { background: rgba(156,39,176,0.15); color: white; font-weight: bold; "
        "border-radius: %1px; font-size: %2px; }"
        "AAButton:hover { background: rgba(156,39,176,0.30); }")
        .arg(dp(10)).arg(dp(13)));
    connect(imgBtn, &QPushButton::clicked, this, [this]() {
        emit sendImageRequested(m_node);
        accept();
    });
    btnRow->addWidget(imgBtn);

    layout->addLayout(btnRow);

    // 取消按钮
    auto *cancelBtn = new QPushButton(tr("Cancel"));
    cancelBtn->setFlat(true);
    cancelBtn->setStyleSheet(QStringLiteral("color: #888;"));
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    layout->addWidget(cancelBtn, 0, Qt::AlignRight);
}
