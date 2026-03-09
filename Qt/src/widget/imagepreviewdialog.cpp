#include "imagepreviewdialog.h"
#include "stylemanager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QComboBox>
#include <QGuiApplication>
#include <QPushButton>
#include <QPixmap>
#include <QScreen>

ImagePreviewDialog::ImagePreviewDialog(const QImage &croppedBitmap,
                                        const ImageResolution &resolution,
                                        const MeshNode &node,
                                        const QList<quint16> &multicastTargets,
                                        QWidget *parent)
    : QDialog(parent)
    , m_node(node)
    , m_multicastTargets(multicastTargets)
    , m_croppedBitmap(croppedBitmap)
    , m_resolution(resolution)
{
    m_processed = ImageUtils::processFromCropped(croppedBitmap, resolution.width, resolution.height);
    buildUI();
}

void ImagePreviewDialog::buildUI()
{
    // ── 标题 ──────────────────────────────────────────────
    QString title;
    if (!m_multicastTargets.isEmpty())
        title = tr("Preview — Multicast %1 nodes").arg(m_multicastTargets.size());
    else
        title = tr("Preview — 0x%1").arg(m_node.addr, 4, 16, QChar('0')).toUpper();
    setWindowTitle(title);

    int screenW = 420;
    if (auto *screen = QGuiApplication::primaryScreen())
        screenW = screen->availableGeometry().width();
    int dlgW = qMin(dp(400), screenW - 32);
    setMinimumWidth(dlgW);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(dp(14), dp(14), dp(14), dp(14));
    layout->setSpacing(dp(10));

    // ── 信息条 ────────────────────────────────────────────
    QString sizeLabel;
    if (m_processed.dataSize < 1024)
        sizeLabel = QStringLiteral("%1 B").arg(m_processed.dataSize);
    else
        sizeLabel = QStringLiteral("%1 KB").arg(m_processed.dataSize / 1024.0, 0, 'f', 1);

    auto *infoCard = new AAWidget(this);
    infoCard->setObjectName("debugCard");
    auto *infoCardLayout = new QHBoxLayout(infoCard);
    infoCardLayout->setContentsMargins(dp(12), dp(8), dp(12), dp(8));
    infoCardLayout->setSpacing(dp(8));

    auto *dimLabel = new QLabel(
        QStringLiteral("%1 × %2").arg(m_resolution.width).arg(m_resolution.height), infoCard);
    dimLabel->setStyleSheet(QStringLiteral(
        "color: #E6EDF3; font-size: %1px; font-weight: 600;").arg(dp(13)));

    auto *metaLabel = new QLabel(
        QStringLiteral("JPEG Q%1  ·  %2 pkts  ·  %3")
            .arg(m_processed.jpegQuality).arg(m_processed.packetCount).arg(sizeLabel), infoCard);
    metaLabel->setStyleSheet(QStringLiteral(
        "color: #888; font-size: %1px;").arg(dp(11)));
    metaLabel->setWordWrap(true);

    infoCardLayout->addWidget(dimLabel, 0);
    infoCardLayout->addWidget(metaLabel, 1);
    layout->addWidget(infoCard);

    // ── 预览图：原图 + 墨水屏模拟并排 ─────────────────────
    int availW = dlgW - dp(28);
    int maxPrevW = qMax(80, availW / 2 - dp(12));
    int maxPrevH = maxPrevW * 14 / 10;

    auto *previewCard = new AAWidget(this);
    previewCard->setObjectName("nodeCard");
    auto *previewRow = new QHBoxLayout(previewCard);
    previewRow->setContentsMargins(dp(10), dp(10), dp(10), dp(10));
    previewRow->setSpacing(dp(8));

    // 原图列
    auto *origCol = new QVBoxLayout;
    origCol->setSpacing(dp(4));
    auto *origTitle = new QLabel(tr("Original"), previewCard);
    origTitle->setStyleSheet(QStringLiteral("color: #666; font-size: %1px;").arg(dp(10)));
    origTitle->setAlignment(Qt::AlignCenter);
    auto *origImgLabel = new QLabel(previewCard);
    QPixmap origPix = QPixmap::fromImage(m_croppedBitmap.scaled(
        m_resolution.width, m_resolution.height, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
    origPix = origPix.scaled(maxPrevW, maxPrevH, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    origImgLabel->setPixmap(origPix);
    origImgLabel->setAlignment(Qt::AlignCenter);
    origImgLabel->setStyleSheet(QStringLiteral(
        "border: 1px solid #333; border-radius: %1px; background: #fff;").arg(dp(4)));
    origCol->addWidget(origTitle);
    origCol->addWidget(origImgLabel, 0, Qt::AlignCenter);
    previewRow->addLayout(origCol, 1);

    // 分隔箭头
    auto *arrowLabel = new QLabel(QStringLiteral("→"), previewCard);
    arrowLabel->setStyleSheet(QStringLiteral(
        "color: #4FC3F7; font-size: %1px; font-weight: bold;").arg(dp(18)));
    arrowLabel->setAlignment(Qt::AlignCenter);
    previewRow->addWidget(arrowLabel, 0, Qt::AlignVCenter);

    // 墨水屏预览列
    auto *epdCol = new QVBoxLayout;
    epdCol->setSpacing(dp(4));
    auto *epdTitle = new QLabel(tr("E-Paper"), previewCard);
    epdTitle->setStyleSheet(QStringLiteral("color: #666; font-size: %1px;").arg(dp(10)));
    epdTitle->setAlignment(Qt::AlignCenter);
    auto *epdImgLabel = new QLabel(previewCard);
    QPixmap epdPix = QPixmap::fromImage(m_processed.previewBitmap)
        .scaled(maxPrevW, maxPrevH, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    epdImgLabel->setPixmap(epdPix);
    epdImgLabel->setAlignment(Qt::AlignCenter);
    epdImgLabel->setStyleSheet(QStringLiteral(
        "border: 1px solid #333; border-radius: %1px; background: #fff;").arg(dp(4)));
    epdCol->addWidget(epdTitle);
    epdCol->addWidget(epdImgLabel, 0, Qt::AlignCenter);
    previewRow->addLayout(epdCol, 1);

    layout->addWidget(previewCard);

    // ── 传输模式 (单播时显示) ─────────────────────────────
    if (m_multicastTargets.isEmpty()) {
        auto *modeCard = new AAWidget(this);
        modeCard->setObjectName("debugCard");
        auto *modeLayout = new QHBoxLayout(modeCard);
        modeLayout->setContentsMargins(dp(12), dp(8), dp(12), dp(8));
        modeLayout->setSpacing(dp(6));

        auto *modeTitleLabel = new QLabel(tr("Mode"), modeCard);
        modeTitleLabel->setStyleSheet(QStringLiteral(
            "color: #888; font-size: %1px;").arg(dp(12)));
        modeLayout->addWidget(modeTitleLabel);
        modeLayout->addStretch();

        m_modeCombo = new QComboBox(modeCard);
        m_modeCombo->addItem(tr("FAST"));
        m_modeCombo->addItem(tr("ACK"));
        m_modeCombo->setCurrentIndex(0);
        m_modeCombo->setMinimumHeight(dp(32));
        m_modeCombo->setToolTip(tr("FAST: gateway flow control\nACK: per-packet confirm"));
        modeLayout->addWidget(m_modeCombo);
        layout->addWidget(modeCard);
    }

    // ── 发送按钮 ──────────────────────────────────────────
    // BLE 发送
    QString bleText = m_multicastTargets.isEmpty()
        ? tr("Send via BLE  (%1)").arg(sizeLabel)
        : tr("Multicast via BLE  (%1)  ▶").arg(sizeLabel);
    QString bleColor = m_multicastTargets.isEmpty()
        ? QStringLiteral("#4FC3F7") : QStringLiteral("#AB47BC");

    auto *sendBtn = new AAButton(bleText, this);
    sendBtn->setMinimumHeight(dp(52));
    sendBtn->setStyleSheet(QStringLiteral(
        "AAButton { background: rgba(%1, 0.18); color: %2; font-weight: 600; "
        "border-radius: %3px; font-size: %4px; }"
        "AAButton:hover { background: rgba(%1, 0.35); }")
        .arg(m_multicastTargets.isEmpty() ? "79,195,247" : "171,71,188")
        .arg(bleColor).arg(dp(10)).arg(dp(14)));
    connect(sendBtn, &QPushButton::clicked, this, [this]() {
        BleManager::ImageSendMode mode = BleManager::FastMode;
        if (m_modeCombo && m_modeCombo->currentIndex() == 1)
            mode = BleManager::AckMode;
        int sendW = m_resolution.width, sendH = m_resolution.height;
        if (m_processed.imageMode == MeshProtocol::IMG_MODE_JPEG) {
            sendW = m_resolution.height; sendH = m_resolution.width;
        }
        emit sendRequested(m_processed.imageData, sendW, sendH,
                           mode, m_processed.imageMode, m_processed.previewBitmap);
        accept();
    });
    layout->addWidget(sendBtn);

    // WiFi 发送
    auto *wifiBtn = new AAButton(tr("Send via WiFi  (%1)").arg(sizeLabel), this);
    wifiBtn->setMinimumHeight(dp(44));
    wifiBtn->setStyleSheet(QStringLiteral(
        "AAButton { background: rgba(76,175,80,0.15); color: #4CAF50; font-weight: 600; "
        "border-radius: %1px; font-size: %2px; }"
        "AAButton:hover { background: rgba(76,175,80,0.30); }")
        .arg(dp(10)).arg(dp(13)));
    connect(wifiBtn, &QPushButton::clicked, this, [this]() {
        int sendW = m_resolution.width, sendH = m_resolution.height;
        if (m_processed.imageMode == MeshProtocol::IMG_MODE_JPEG) {
            sendW = m_resolution.height; sendH = m_resolution.width;
        }
        emit wifiSendRequested(m_processed.imageData, sendW, sendH,
                               m_processed.imageMode, m_processed.previewBitmap);
        accept();
    });
    layout->addWidget(wifiBtn);

    // 取消
    auto *cancelBtn = new AAButton(tr("Cancel"), this);
    cancelBtn->setMinimumHeight(dp(36));
    cancelBtn->setStyleSheet(QStringLiteral(
        "AAButton { background: transparent; color: #484F58; "
        "border-radius: %1px; font-size: %2px; }"
        "AAButton:hover { color: #888; }")
        .arg(dp(8)).arg(dp(12)));
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    layout->addWidget(cancelBtn);
}
