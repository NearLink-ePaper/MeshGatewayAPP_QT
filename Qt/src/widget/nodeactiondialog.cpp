#include "nodeactiondialog.h"
#include "stylemanager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QGuiApplication>
#include <QPixmap>
#include <QScreen>

NodeActionDialog::NodeActionDialog(const MeshNode &node, const QImage &lastSentBitmap, QWidget *parent)
    : QDialog(parent), m_node(node)
{
    setWindowTitle(QStringLiteral("0x%1").arg(node.addr, 4, 16, QChar('0')).toUpper());

    int scrW = 420;
    if (auto *scr = QGuiApplication::primaryScreen())
        scrW = scr->availableGeometry().width();
    setMinimumWidth(qMin(320, scrW - 40));

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(dp(16), dp(16), dp(16), dp(16));
    layout->setSpacing(dp(12));

    // ── 节点信息卡片 ──────────────────────────────────
    auto *infoCard = new AAWidget(this);
    infoCard->setObjectName("nodeCard");
    auto *infoLayout = new QHBoxLayout(infoCard);
    infoLayout->setContentsMargins(dp(14), dp(12), dp(14), dp(12));
    infoLayout->setSpacing(dp(12));

    auto *addrLabel = new QLabel(
        QStringLiteral("0x%1").arg(node.addr, 4, 16, QChar('0')).toUpper(), infoCard);
    addrLabel->setStyleSheet(QStringLiteral(
        "color: #E6EDF3; font-size: %1px; font-weight: 600;").arg(dp(15)));

    QString hopsText;
    QString hopsColor;
    switch (node.hops) {
    case 0:  hopsText = tr("Gateway");  hopsColor = "#58A6FF"; break;
    case 1:  hopsText = tr("Direct");   hopsColor = "#3FB950"; break;
    default: hopsText = tr("%1 hops").arg(node.hops); hopsColor = "#D29922"; break;
    }
    auto *hopsLabel = new QLabel(hopsText, infoCard);
    hopsLabel->setStyleSheet(QStringLiteral(
        "color: %1; font-size: %2px; font-weight: 600; "
        "background: %3; border-radius: %4px; padding: %5px %6px;")
        .arg(hopsColor).arg(dp(11))
        .arg(QColor(hopsColor).name() + QStringLiteral("22"))
        .arg(dp(4)).arg(dp(3)).arg(dp(8)));

    infoLayout->addWidget(addrLabel, 1);
    infoLayout->addWidget(hopsLabel, 0);
    layout->addWidget(infoCard);

    // ── 上次发送图片预览 ──────────────────────────────
    if (!lastSentBitmap.isNull()) {
        auto *prevCard = new AAWidget(this);
        prevCard->setObjectName("debugCard");
        auto *prevLayout = new QVBoxLayout(prevCard);
        prevLayout->setContentsMargins(dp(10), dp(8), dp(10), dp(8));
        prevLayout->setSpacing(dp(6));

        auto *prevTitle = new QLabel(tr("Last Sent Image"), prevCard);
        prevTitle->setStyleSheet(QStringLiteral(
            "color: #888; font-size: %1px;").arg(dp(11)));

        auto *prevLabel = new QLabel(prevCard);
        int prevSize = qMin(scrW - 80, 200);
        QPixmap pix = QPixmap::fromImage(lastSentBitmap)
            .scaled(prevSize, prevSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        prevLabel->setPixmap(pix);
        prevLabel->setAlignment(Qt::AlignCenter);
        prevLayout->addWidget(prevTitle);
        prevLayout->addWidget(prevLabel, 0, Qt::AlignCenter);
        layout->addWidget(prevCard);
    }

    // ── 操作按钮（垂直排列，移动端友好）──────────────
    auto *imgBtn = new AAButton(tr("Send Image  ▶"), this);
    imgBtn->setMinimumHeight(dp(52));
    imgBtn->setStyleSheet(QStringLiteral(
        "AAButton { background: rgba(88,166,255,0.15); color: #58A6FF; "
        "font-weight: 600; border-radius: %1px; font-size: %2px; }"
        "AAButton:hover { background: rgba(88,166,255,0.30); }")
        .arg(dp(10)).arg(dp(14)));
    connect(imgBtn, &QPushButton::clicked, this, [this]() {
        emit sendImageRequested(m_node);
        accept();
    });
    layout->addWidget(imgBtn);

    auto *textBtn = new AAButton(tr("Send Text"), this);
    textBtn->setMinimumHeight(dp(44));
    textBtn->setStyleSheet(QStringLiteral(
        "AAButton { background: rgba(63,185,80,0.12); color: #3FB950; "
        "font-weight: 600; border-radius: %1px; font-size: %2px; }"
        "AAButton:hover { background: rgba(63,185,80,0.25); }")
        .arg(dp(10)).arg(dp(13)));
    connect(textBtn, &QPushButton::clicked, this, [this]() {
        emit sendTextRequested(m_node);
        accept();
    });
    layout->addWidget(textBtn);

    auto *cancelBtn = new AAButton(tr("Cancel"), this);
    cancelBtn->setMinimumHeight(dp(36));
    cancelBtn->setStyleSheet(QStringLiteral(
        "AAButton { background: transparent; color: #555; "
        "border-radius: %1px; font-size: %2px; }"
        "AAButton:hover { color: #888; }")
        .arg(dp(8)).arg(dp(12)));
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    layout->addWidget(cancelBtn);
}
