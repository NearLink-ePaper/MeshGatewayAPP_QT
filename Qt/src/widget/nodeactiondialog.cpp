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
    // 移动端贴满屏宽，PC 端限制最大 360px
    int dlgW = (scrW <= 500) ? (scrW - 16) : qMin(360, scrW - 32);
    setFixedWidth(dlgW);

    // 节点跳数对应主题色
    QString hopsColor;
    QString hopsText;
    switch (node.hops) {
    case 0:  hopsColor = "#58A6FF"; hopsText = tr("网关");   break;
    case 1:  hopsColor = "#3FB950"; hopsText = tr("直连");   break;
    default: hopsColor = "#D29922"; hopsText = tr("%1跳").arg(node.hops); break;
    }
    QColor accent(hopsColor);

    // 半透明暗遮罩背景（避免 Android 裸透明导致的按钮合成撕裂）
    setStyleSheet(QStringLiteral(
        "QDialog { background-color: rgba(0,0,0,160); }"));

    // ── 布局：顶部信息卡 + 弹性区（可放图片）+ 底部按钮卡 ──
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(dp(12), dp(16), dp(12), dp(20));
    root->setSpacing(dp(10));

    // ─── 顶部信息卡（置顶，不贴边） ───
    auto *headerCard = new AAWidget(this);
    headerCard->setBgColor(QColor(30, 38, 50));
    headerCard->setBorderColor(QColor(accent.red(), accent.green(), accent.blue(), 60));
    headerCard->setBorderRadius(dp(14));
    headerCard->setBorderWidth(1);
    headerCard->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    auto *headerRow = new QHBoxLayout(headerCard);
    headerRow->setContentsMargins(dp(14), dp(12), dp(14), dp(12));
    headerRow->setSpacing(dp(12));

    auto *avatarLabel = new QLabel(headerCard);
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
        QStringLiteral("0x%1").arg(node.addr, 4, 16, QChar('0')).toUpper(), headerCard);
    addrLabel->setStyleSheet(QStringLiteral(
        "color:#E6EDF3; font-size:%1px; font-weight:700; letter-spacing:1px;").arg(dp(18)));
    auto *hopsSubLabel = new QLabel(
        node.hops == 0 ? tr("网关节点")
                       : (node.hops == 1 ? tr("直连节点")
                                         : tr("%1 跳路由").arg(node.hops)), headerCard);
    hopsSubLabel->setStyleSheet(QStringLiteral(
        "color:%1; font-size:%2px;").arg(hopsColor).arg(dp(12)));
    addrCol->addWidget(addrLabel);
    addrCol->addWidget(hopsSubLabel);

    headerRow->addWidget(avatarLabel);
    headerRow->addLayout(addrCol, 1);
    root->addWidget(headerCard);

    // ─── 中部弹性区（有图时显示上次发送预览） ───
    if (!lastSentBitmap.isNull()) {
        auto *imgArea = new QLabel(this);
        imgArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        imgArea->setAlignment(Qt::AlignCenter);
        int maxH = scrH * 5 / 10;
        QPixmap pix = QPixmap::fromImage(lastSentBitmap)
            .scaled(dlgW - dp(24), maxH, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        imgArea->setPixmap(pix);
        root->addWidget(imgArea, 3);
    } else {
        root->addStretch(3);
    }

    // ─────────── 底部按钮卡 ───────────
    auto *sheet = new AAWidget(this);
    sheet->setBgColor(QColor(22, 27, 34));
    sheet->setBorderColor(QColor(255, 255, 255, 18));
    sheet->setBorderRadius(dp(16));
    sheet->setBorderWidth(1);
    sheet->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

    auto *sheetLayout = new QVBoxLayout(sheet);
    sheetLayout->setContentsMargins(dp(14), dp(14), dp(14), dp(14));
    sheetLayout->setSpacing(dp(10));

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
