#include "imagepreviewdialog.h"
#include "stylemanager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include <QPixmap>

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
    QString title;
    if (!m_multicastTargets.isEmpty())
        title = tr("Image Preview → Multicast %1 nodes").arg(m_multicastTargets.size());
    else
        title = tr("Image Preview → 0x%1").arg(m_node.addr, 4, 16, QChar('0')).toUpper();
    setWindowTitle(title);
    setMinimumWidth(dp(400));

    auto *layout = new QVBoxLayout(this);
    layout->setSpacing(dp(10));

    // 数据信息
    QString sizeLabel;
    if (m_processed.isCompressed) {
        float ratio = (float)m_processed.dataSize / m_processed.rawSize * 100.0f;
        sizeLabel = QStringLiteral("%1 B → %2 B (RLE %3%)")
                        .arg(m_processed.rawSize).arg(m_processed.dataSize)
                        .arg(ratio, 0, 'f', 1);
    } else {
        sizeLabel = QStringLiteral("%1 B (RAW)").arg(m_processed.dataSize);
    }
    auto *infoLabel = new QLabel(QStringLiteral("%1×%2 | %3 pkts | %4")
                                     .arg(m_resolution.width).arg(m_resolution.height)
                                     .arg(m_processed.packetCount).arg(sizeLabel));
    infoLabel->setStyleSheet(QStringLiteral("color: #B0BEC5; font-size: %1px;").arg(dp(11)));
    infoLabel->setWordWrap(true);
    layout->addWidget(infoLabel);

    // 预览图: 原图 + 二值化并排
    auto *previewRow = new QHBoxLayout;
    previewRow->setSpacing(dp(8));

    int maxPrevW = dp(180);
    int maxPrevH = dp(260);

    // 原图
    auto *origFrame = new QWidget;
    auto *origLayout = new QVBoxLayout(origFrame);
    origLayout->setContentsMargins(0, 0, 0, 0);
    origLayout->setSpacing(dp(4));
    auto *origTitle = new QLabel(tr("Original"));
    origTitle->setStyleSheet(QStringLiteral("color: #888; font-size: %1px;").arg(dp(10)));
    origTitle->setAlignment(Qt::AlignCenter);
    origLayout->addWidget(origTitle);
    auto *origImgLabel = new QLabel;
    QPixmap origPix = QPixmap::fromImage(m_croppedBitmap.scaled(
        m_resolution.width, m_resolution.height, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
    origPix = origPix.scaled(maxPrevW, maxPrevH, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    origImgLabel->setPixmap(origPix);
    origImgLabel->setAlignment(Qt::AlignCenter);
    origImgLabel->setStyleSheet(QStringLiteral("border: 1px solid #333; border-radius: %1px;").arg(dp(4)));
    origLayout->addWidget(origImgLabel);
    previewRow->addWidget(origFrame);

    // 箭头
    auto *arrowLabel = new QLabel(QStringLiteral("→"));
    arrowLabel->setStyleSheet(QStringLiteral("color: #4FC3F7; font-size: %1px;").arg(dp(20)));
    arrowLabel->setAlignment(Qt::AlignCenter);
    previewRow->addWidget(arrowLabel);

    // 二值化
    auto *bwFrame = new QWidget;
    auto *bwLayout = new QVBoxLayout(bwFrame);
    bwLayout->setContentsMargins(0, 0, 0, 0);
    bwLayout->setSpacing(dp(4));
    auto *bwTitle = new QLabel(tr("Binary"));
    bwTitle->setStyleSheet(QStringLiteral("color: #888; font-size: %1px;").arg(dp(10)));
    bwTitle->setAlignment(Qt::AlignCenter);
    bwLayout->addWidget(bwTitle);
    auto *bwImgLabel = new QLabel;
    QPixmap bwPix = QPixmap::fromImage(m_processed.previewBitmap);
    bwPix = bwPix.scaled(maxPrevW, maxPrevH, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    bwImgLabel->setPixmap(bwPix);
    bwImgLabel->setAlignment(Qt::AlignCenter);
    bwImgLabel->setStyleSheet(QStringLiteral("border: 1px solid #333; border-radius: %1px; background: white;").arg(dp(4)));
    bwLayout->addWidget(bwImgLabel);
    previewRow->addWidget(bwFrame);

    layout->addLayout(previewRow);

    // 传输模式选择 (仅单播时显示)
    if (m_multicastTargets.isEmpty()) {
        auto *modeRow = new QHBoxLayout;
        auto *modeLabel = new QLabel(tr("Mode"));
        modeLabel->setStyleSheet(QStringLiteral("color: #B0BEC5; font-size: %1px;").arg(dp(12)));
        modeRow->addWidget(modeLabel);
        m_modeCombo = new QComboBox;
        m_modeCombo->addItem(tr("FAST (gateway flow control)"));
        m_modeCombo->addItem(tr("ACK (per-packet confirm)"));
        m_modeCombo->setCurrentIndex(0);
        modeRow->addWidget(m_modeCombo);
        modeRow->addStretch();
        layout->addLayout(modeRow);
    }

    // 按钮行
    auto *btnRow = new QHBoxLayout;
    btnRow->addStretch();
    auto *cancelBtn = new QPushButton(tr("Cancel"));
    cancelBtn->setFlat(true);
    cancelBtn->setStyleSheet(QStringLiteral("color: #888;"));
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    btnRow->addWidget(cancelBtn);

    QString sendText;
    if (!m_multicastTargets.isEmpty())
        sendText = tr("Multicast Send (%1)").arg(sizeLabel);
    else
        sendText = tr("Send (%1)").arg(sizeLabel);

    auto *sendBtn = new AAButton(sendText);
    QString sendColor = m_multicastTargets.isEmpty()
        ? QStringLiteral("#4FC3F7") : QStringLiteral("#9C27B0");
    sendBtn->setStyleSheet(QStringLiteral(
        "AAButton { background: %1; color: white; font-weight: bold; "
        "border-radius: %2px; padding: %3px %4px; font-size: %5px; }"
        "AAButton:hover { background: %1; opacity: 0.8; }")
        .arg(sendColor).arg(dp(6)).arg(dp(8)).arg(dp(16)).arg(dp(13)));
    connect(sendBtn, &QPushButton::clicked, this, [this]() {
        BleManager::ImageSendMode mode = BleManager::FastMode;
        if (m_modeCombo && m_modeCombo->currentIndex() == 1)
            mode = BleManager::AckMode;
        emit sendRequested(m_processed.imageData, m_resolution.width, m_resolution.height,
                           mode, m_processed.imageMode, m_processed.previewBitmap);
        accept();
    });
    btnRow->addWidget(sendBtn);
    layout->addLayout(btnRow);
}
