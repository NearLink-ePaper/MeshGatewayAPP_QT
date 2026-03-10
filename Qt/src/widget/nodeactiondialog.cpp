#include "nodeactiondialog.h"
#include "stylemanager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QGuiApplication>
#include <QPixmap>
#include <QScreen>
#include <QPalette>
#include <QFont>

NodeActionDialog::NodeActionDialog(const MeshNode &node, const QImage &lastSentBitmap, QWidget *parent)
    : QDialog(parent), m_node(node)
{
    setWindowTitle(QStringLiteral("0x%1").arg(node.addr, 4, 16, QChar('0')).toUpper());

    int scrW = 420, scrH = 700;
    if (auto *scr = QGuiApplication::primaryScreen()) {
        scrW = scr->availableGeometry().width();
        scrH = scr->availableGeometry().height();
    }
    setFixedWidth(scrW - 16);

    // 节点跳数对应主题色
    QString hopsColor;
    QString hopsText;
    switch (node.hops) {
    case 0:  hopsColor = "#58A6FF"; hopsText = tr("网关");   break;
    case 1:  hopsColor = "#3FB950"; hopsText = tr("直连");   break;
    default: hopsColor = "#D29922"; hopsText = tr("%1跳").arg(node.hops); break;
    }
    QColor accent(hopsColor);

    // ── iOS action-sheet 风格: 顶部大弹性区 + 底部紧凑内容卡 ──
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // 顶部弹性区（点透明，内容推到底部）
    root->addStretch(3);

    // ─────────── 底部内容卡（圆角顶部） ───────────
    auto *sheet = new AAWidget(this);
    sheet->setBgColor(QColor(22, 27, 34));
    sheet->setBorderColor(QColor(255, 255, 255, 18));
    sheet->setBorderRadius(dp(20));
    sheet->setBorderWidth(1);
    sheet->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

    auto *sheetLayout = new QVBoxLayout(sheet);
    sheetLayout->setContentsMargins(dp(16), dp(20), dp(16), dp(28));
    sheetLayout->setSpacing(dp(10));

    // ── 头部信息行 ──
    auto *headerRow = new QHBoxLayout();
    headerRow->setSpacing(dp(12));

    auto *avatarLabel = new QLabel(sheet);
    avatarLabel->setFixedSize(dp(44), dp(44));
    avatarLabel->setAlignment(Qt::AlignCenter);
    avatarLabel->setStyleSheet(QStringLiteral(
        "color: %1; font-size: %2px; font-weight: 700; "
        "background: rgba(%3,%4,%5,0.18); border-radius: %6px;")
        .arg(hopsColor).arg(dp(14))
        .arg(accent.red()).arg(accent.green()).arg(accent.blue())
        .arg(dp(22)));
    avatarLabel->setText(hopsText);

    auto *addrCol = new QVBoxLayout();
    addrCol->setSpacing(dp(3));
    auto *addrLabel = new QLabel(
        QStringLiteral("0x%1").arg(node.addr, 4, 16, QChar('0')).toUpper(), sheet);
    addrLabel->setStyleSheet(QStringLiteral(
        "color:#E6EDF3; font-size:%1px; font-weight:700; letter-spacing:1px;").arg(dp(18)));
    auto *hopsSubLabel = new QLabel(
        node.hops == 0 ? tr("网关节点")
                       : (node.hops == 1 ? tr("直连节点")
                                         : tr("%1 跳路由").arg(node.hops)), sheet);
    hopsSubLabel->setStyleSheet(QStringLiteral(
        "color:%1; font-size:%2px;").arg(hopsColor).arg(dp(12)));
    addrCol->addWidget(addrLabel);
    addrCol->addWidget(hopsSubLabel);

    headerRow->addWidget(avatarLabel);
    headerRow->addLayout(addrCol, 1);
    sheetLayout->addLayout(headerRow);

    // ── 上次发送预览 ──
    if (!lastSentBitmap.isNull()) {
        auto *prevRow = new QHBoxLayout();
        prevRow->setSpacing(dp(10));

        auto *prevTitle = new QLabel(tr("上次发送"), sheet);
        prevTitle->setStyleSheet(QStringLiteral(
            "color:#6E7681; font-size:%1px;").arg(dp(11)));

        auto *prevLabel = new QLabel(sheet);
        int prevSize = qMin(qMin(scrW / 4, scrH / 8), dp(80));
        QPixmap pix = QPixmap::fromImage(lastSentBitmap)
            .scaled(prevSize, prevSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        prevLabel->setPixmap(pix);
        prevLabel->setAlignment(Qt::AlignVCenter | Qt::AlignRight);

        prevRow->addWidget(prevTitle, 1, Qt::AlignVCenter);
        prevRow->addWidget(prevLabel);
        sheetLayout->addLayout(prevRow);
    }

    // ── 分割线 ──
    auto *sep = new QFrame(sheet);
    sep->setFrameShape(QFrame::HLine);
    sep->setStyleSheet(QStringLiteral("color: rgba(255,255,255,12);"));
    sheetLayout->addWidget(sep);

    // ── 发送图片 ──
    auto *imgBtn = new AAButton(tr("发送图片"), sheet);
    imgBtn->setMinimumHeight(dp(52));
    imgBtn->setBgColor(QColor(29, 99, 191));
    imgBtn->setBorderColor(QColor(88, 166, 255, 80));
    imgBtn->setBorderRadius(dp(13));
    imgBtn->setBorderWidth(1);
    imgBtn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    {
        QPalette p = imgBtn->palette();
        p.setColor(QPalette::ButtonText, Qt::white);
        imgBtn->setPalette(p);
    }
    QFont fBig = imgBtn->font();
    fBig.setPixelSize(dp(16));
    fBig.setWeight(QFont::DemiBold);
    imgBtn->setFont(fBig);
    connect(imgBtn, &QPushButton::clicked, this, [this]() {
        emit sendImageRequested(m_node);
        accept();
    });
    sheetLayout->addWidget(imgBtn);

    // ── 发送文本 ──
    auto *textBtn = new AAButton(tr("发送文本"), sheet);
    textBtn->setMinimumHeight(dp(46));
    textBtn->setBgColor(QColor(22, 27, 34));
    textBtn->setBorderColor(QColor(63, 185, 80, 120));
    textBtn->setBorderRadius(dp(13));
    textBtn->setBorderWidth(1);
    textBtn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    {
        QPalette p = textBtn->palette();
        p.setColor(QPalette::ButtonText, QColor("#3FB950"));
        textBtn->setPalette(p);
    }
    QFont fMid = textBtn->font();
    fMid.setPixelSize(dp(15));
    fMid.setWeight(QFont::Medium);
    textBtn->setFont(fMid);
    connect(textBtn, &QPushButton::clicked, this, [this]() {
        emit sendTextRequested(m_node);
        accept();
    });
    sheetLayout->addWidget(textBtn);

    // ── 取消 ──
    auto *cancelBtn = new AAButton(tr("取消"), sheet);
    cancelBtn->setMinimumHeight(dp(36));
    cancelBtn->setBgColor(Qt::transparent);
    cancelBtn->setBorderColor(Qt::transparent);
    cancelBtn->setBorderRadius(0);
    cancelBtn->setBorderWidth(0);
    {
        QPalette p = cancelBtn->palette();
        p.setColor(QPalette::ButtonText, QColor("#6E7681"));
        cancelBtn->setPalette(p);
    }
    QFont fSmall = cancelBtn->font();
    fSmall.setPixelSize(dp(13));
    cancelBtn->setFont(fSmall);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    sheetLayout->addWidget(cancelBtn);

    root->addWidget(sheet);
}
