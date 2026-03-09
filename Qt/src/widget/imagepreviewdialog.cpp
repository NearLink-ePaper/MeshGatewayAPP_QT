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
    QString title;
    if (!m_multicastTargets.isEmpty())
        title = tr("Image Preview → Multicast %1 nodes").arg(m_multicastTargets.size());
    else
        title = tr("Image Preview → 0x%1").arg(m_node.addr, 4, 16, QChar('0')).toUpper();
    setWindowTitle(title);
    // 移动端自适应宽度
    int screenW = 420;
    if (auto *screen = QGuiApplication::primaryScreen())
        screenW = screen->availableGeometry().width();
    setMinimumWidth(qMin(dp(400), screenW - 40));

    auto *layout = new QVBoxLayout(this);
    layout->setSpacing(dp(10));

    // 数据信息
    QString sizeLabel;
    if (m_processed.dataSize < 1024)
        sizeLabel = QStringLiteral("%1 B").arg(m_processed.dataSize);
    else
        sizeLabel = QStringLiteral("%1 KB").arg(m_processed.dataSize / 1024.0, 0, 'f', 1);
    auto *infoLabel = new QLabel(QStringLiteral("%1×%2 | JPEG Q%3 | %4 pkts | %5")
                                     .arg(m_resolution.width).arg(m_resolution.height)
                                     .arg(m_processed.jpegQuality)
                                     .arg(m_processed.packetCount).arg(sizeLabel));
    infoLabel->setStyleSheet(QStringLiteral("color: #B0BEC5; font-size: %1px;").arg(dp(11)));
    infoLabel->setWordWrap(true);
    layout->addWidget(infoLabel);

    // 预览图: 原图 + JPEG 压缩后并排
    auto *previewRow = new QHBoxLayout;
    previewRow->setSpacing(dp(8));

    // 预览图尺寸按可用宽度自适应（两张并排 + 间距 + 边距）
    int availW = qMin(dp(400), screenW - 40) - dp(40);
    int maxPrevW = qMax(80, availW / 2 - dp(20));
    int maxPrevH = maxPrevW * 14 / 10;

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

    // 墨水屏模拟预览 (6色抖动)
    auto *epdFrame = new QWidget;
    auto *epdLayout = new QVBoxLayout(epdFrame);
    epdLayout->setContentsMargins(0, 0, 0, 0);
    epdLayout->setSpacing(dp(4));
    auto *epdTitle = new QLabel(tr("E-Paper Preview"));
    epdTitle->setStyleSheet(QStringLiteral("color: #888; font-size: %1px;").arg(dp(10)));
    epdTitle->setAlignment(Qt::AlignCenter);
    epdLayout->addWidget(epdTitle);
    auto *epdImgLabel = new QLabel;
    QPixmap epdPix = QPixmap::fromImage(m_processed.previewBitmap);
    epdPix = epdPix.scaled(maxPrevW, maxPrevH, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    epdImgLabel->setPixmap(epdPix);
    epdImgLabel->setAlignment(Qt::AlignCenter);
    epdImgLabel->setStyleSheet(QStringLiteral("border: 1px solid #333; border-radius: %1px;").arg(dp(4)));
    epdLayout->addWidget(epdImgLabel);
    previewRow->addWidget(epdFrame);

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
        // JPEG 模式: 图像已预旋转为 landscape, 发送实际 JPEG 尺寸 (height×width)
        int sendW = m_resolution.width;
        int sendH = m_resolution.height;
        if (m_processed.imageMode == MeshProtocol::IMG_MODE_JPEG) {
            sendW = m_resolution.height;
            sendH = m_resolution.width;
        }
        emit sendRequested(m_processed.imageData, sendW, sendH,
                           mode, m_processed.imageMode, m_processed.previewBitmap);
        accept();
    });
    btnRow->addWidget(sendBtn);

    /* WiFi 直传按钮 */
    auto *wifiBtn = new AAButton(tr("WiFi Send (%1)").arg(sizeLabel));
    wifiBtn->setStyleSheet(QStringLiteral(
        "AAButton { background: #4CAF50; color: white; font-weight: bold; "
        "border-radius: %1px; padding: %2px %3px; font-size: %4px; }"
        "AAButton:hover { background: #388E3C; }")
        .arg(dp(6)).arg(dp(8)).arg(dp(16)).arg(dp(13)));
    connect(wifiBtn, &QPushButton::clicked, this, [this]() {
        int sendW = m_resolution.width;
        int sendH = m_resolution.height;
        if (m_processed.imageMode == MeshProtocol::IMG_MODE_JPEG) {
            sendW = m_resolution.height;
            sendH = m_resolution.width;
        }
        emit wifiSendRequested(m_processed.imageData, sendW, sendH,
                               m_processed.imageMode, m_processed.previewBitmap);
        accept();
    });
    btnRow->addWidget(wifiBtn);

    layout->addLayout(btnRow);
}
