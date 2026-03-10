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

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(dp(16), dp(16), dp(16), dp(12));
    root->setSpacing(dp(8));
    root->setSizeConstraint(QLayout::SetMinimumSize);  /* 高度自适应，宽度由 setFixedWidth 控制 */

    // ─────────────── 头部信息卡 ───────────────
    auto *headerCard = new AAWidget(this);
    headerCard->setBgColor(QColor(30, 38, 50));
    headerCard->setBorderColor(QColor(accent.red(), accent.green(), accent.blue(), 60));
    headerCard->setBorderRadius(dp(14));
    headerCard->setBorderWidth(1);
    headerCard->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    auto *headerLayout = new QHBoxLayout(headerCard);
    headerLayout->setContentsMargins(dp(16), dp(14), dp(16), dp(14));
    headerLayout->setSpacing(dp(12));

    // 圆形节点头像
    auto *avatarLabel = new QLabel(headerCard);
    avatarLabel->setFixedSize(dp(44), dp(44));
    avatarLabel->setAlignment(Qt::AlignCenter);
    avatarLabel->setStyleSheet(QStringLiteral(
        "color: %1; font-size: %2px; font-weight: 700; "
        "background: rgba(%3,%4,%5,0.18); border-radius: %6px;")
        .arg(hopsColor)
        .arg(dp(14))
        .arg(accent.red()).arg(accent.green()).arg(accent.blue())
        .arg(dp(22)));
    avatarLabel->setText(hopsText);

    // 地址 + 跳数列
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

    headerLayout->addWidget(avatarLabel);
    headerLayout->addLayout(addrCol, 1);
    root->addWidget(headerCard);

    // ─────────────── 上次发送预览 ───────────────
    if (!lastSentBitmap.isNull()) {
        auto *prevCard = new AAWidget(this);
        prevCard->setBgColor(QColor(22, 27, 34));
        prevCard->setBorderColor(QColor(240, 246, 252, 15));
        prevCard->setBorderRadius(dp(12));
        prevCard->setBorderWidth(1);

        auto *prevLayout = new QVBoxLayout(prevCard);
        prevLayout->setContentsMargins(dp(12), dp(10), dp(12), dp(10));
        prevLayout->setSpacing(dp(6));

        auto *prevTitle = new QLabel(tr("上次发送图片"), prevCard);
        prevTitle->setStyleSheet(QStringLiteral(
            "color:#6E7681; font-size:%1px;").arg(dp(11)));

        auto *prevLabel = new QLabel(prevCard);
        int prevSize = qMin(qMin(scrW - 80, scrH / 4), 180);
        QPixmap pix = QPixmap::fromImage(lastSentBitmap)
            .scaled(prevSize, prevSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        prevLabel->setPixmap(pix);
        prevLabel->setAlignment(Qt::AlignCenter);
        prevLayout->addWidget(prevTitle);
        prevLayout->addWidget(prevLabel, 0, Qt::AlignCenter);
        root->addWidget(prevCard);
    }

    // ─────────────── 操作按钮 ───────────────
    // 主操作: 发送图片（实心蓝色）
    auto *imgBtn = new AAButton(tr("发送图片"), this);
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
    root->addWidget(imgBtn);

    // 次操作: 发送文本（边框风格）
    auto *textBtn = new AAButton(tr("发送文本"), this);
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
    root->addWidget(textBtn);

    // 取消（淡色文字）
    auto *cancelBtn = new AAButton(tr("取消"), this);
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
    root->addWidget(cancelBtn);
}
