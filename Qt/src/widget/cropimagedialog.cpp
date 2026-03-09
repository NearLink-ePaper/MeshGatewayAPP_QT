#include "cropimagedialog.h"
#include "stylemanager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainter>
#include <QMouseEvent>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QTransform>
#include <QtMath>

// ═══════════════════════════════════════════════════
//  CropWidget — 裁剪区域绘制控件
// ═══════════════════════════════════════════════════

CropWidget::CropWidget(QWidget *parent) : QWidget(parent)
{
    setMinimumSize(dp(200), dp(200));
    setMouseTracking(true);
}

void CropWidget::setImage(const QImage &img)
{
    m_image = img;
    resetCropRect();
    update();
}

void CropWidget::setAspectRatio(float ar)
{
    m_targetAR = ar;
    resetCropRect();
    update();
}

QRectF CropWidget::cropRect() const
{
    return QRectF(m_cropL, m_cropT, m_cropR - m_cropL, m_cropB - m_cropT);
}

void CropWidget::resetCropRect()
{
    if (m_image.isNull()) return;
    float imgW = m_image.width();
    float imgH = m_image.height();
    // normAR = 裁剪框在归一化空间中的宽高比
    float normAR = m_targetAR * imgH / imgW;

    if (imgW / imgH > m_targetAR) {
        float cw = normAR;
        m_cropL = (1.0f - cw) / 2.0f;
        m_cropR = m_cropL + cw;
        m_cropT = 0; m_cropB = 1;
    } else {
        float ch = 1.0f / normAR;
        m_cropT = (1.0f - ch) / 2.0f;
        m_cropB = m_cropT + ch;
        m_cropL = 0; m_cropR = 1;
    }
}

QRectF CropWidget::imageRect() const
{
    if (m_image.isNull()) return QRectF();
    float wRatio = (float)width() / m_image.width();
    float hRatio = (float)height() / m_image.height();
    float scale = qMin(wRatio, hRatio);
    float iw = m_image.width() * scale;
    float ih = m_image.height() * scale;
    float x = (width() - iw) / 2.0f;
    float y = (height() - ih) / 2.0f;
    return QRectF(x, y, iw, ih);
}

QPointF CropWidget::toNorm(const QPointF &widgetPos) const
{
    QRectF ir = imageRect();
    if (ir.isEmpty()) return QPointF(0, 0);
    return QPointF((widgetPos.x() - ir.x()) / ir.width(),
                   (widgetPos.y() - ir.y()) / ir.height());
}

QPointF CropWidget::fromNorm(const QPointF &normPos) const
{
    QRectF ir = imageRect();
    return QPointF(ir.x() + normPos.x() * ir.width(),
                   ir.y() + normPos.y() * ir.height());
}

void CropWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::SmoothPixmapTransform);

    if (m_image.isNull()) return;

    QRectF ir = imageRect();
    p.drawImage(ir, m_image);

    float bw = ir.width();
    float bh = ir.height();
    float cL = ir.x() + m_cropL * bw;
    float cT = ir.y() + m_cropT * bh;
    float cR = ir.x() + m_cropR * bw;
    float cB = ir.y() + m_cropB * bh;

    // 半透明遮罩 (裁剪区域外)
    QColor mask(0, 0, 0, 140);
    p.fillRect(QRectF(ir.x(), ir.y(), bw, cT - ir.y()), mask);                     // 上
    p.fillRect(QRectF(ir.x(), cB, bw, ir.bottom() - cB), mask);                    // 下
    p.fillRect(QRectF(ir.x(), cT, cL - ir.x(), cB - cT), mask);                    // 左
    p.fillRect(QRectF(cR, cT, ir.right() - cR, cB - cT), mask);                    // 右

    // 裁剪框白色边框
    p.setPen(QPen(Qt::white, dp(2)));
    p.setBrush(Qt::NoBrush);
    p.drawRect(QRectF(cL, cT, cR - cL, cB - cT));

    // 三分线
    p.setPen(QPen(QColor(255, 255, 255, 77), 1));
    float w3 = (cR - cL) / 3.0f;
    float h3 = (cB - cT) / 3.0f;
    for (int i = 1; i <= 2; ++i) {
        p.drawLine(QPointF(cL + w3 * i, cT), QPointF(cL + w3 * i, cB));
        p.drawLine(QPointF(cL, cT + h3 * i), QPointF(cR, cT + h3 * i));
    }

    // 四角手柄
    float hr = dp(8);
    QPointF corners[] = {
        {cL, cT}, {cR, cT}, {cL, cB}, {cR, cB}
    };
    for (auto &c : corners) {
        p.setPen(Qt::NoPen);
        p.setBrush(Qt::white);
        p.drawEllipse(c, hr, hr);
        p.setBrush(QColor(0x4F, 0xC3, 0xF7));
        p.drawEllipse(c, hr - dp(2), hr - dp(2));
    }
}

void CropWidget::mousePressEvent(QMouseEvent *event)
{
    QRectF ir = imageRect();
    if (ir.isEmpty()) return;

    QPointF pos = event->position();
    float bw = ir.width();
    float bh = ir.height();

    QPointF corners[] = {
        {ir.x() + m_cropL * bw, ir.y() + m_cropT * bh},
        {ir.x() + m_cropR * bw, ir.y() + m_cropT * bh},
        {ir.x() + m_cropL * bw, ir.y() + m_cropB * bh},
        {ir.x() + m_cropR * bw, ir.y() + m_cropB * bh}
    };

    float handleR = dp(40);
    int closest = -1;
    float minDist = 1e9f;
    for (int i = 0; i < 4; ++i) {
        float d = QPointF(pos - corners[i]).manhattanLength();
        if (d < minDist) { minDist = d; closest = i; }
    }

    if (closest >= 0 && minDist < handleR * 2.5f) {
        m_dragMode = closest + 2; // 2=TL 3=TR 4=BL 5=BR
    } else {
        float cL = ir.x() + m_cropL * bw;
        float cT = ir.y() + m_cropT * bh;
        float cR = ir.x() + m_cropR * bw;
        float cB = ir.y() + m_cropB * bh;
        if (pos.x() >= cL && pos.x() <= cR && pos.y() >= cT && pos.y() <= cB) {
            m_dragMode = 1; // move
        } else {
            m_dragMode = 0;
        }
    }

    m_dragStart = pos;
    m_dragCropL = m_cropL; m_dragCropT = m_cropT;
    m_dragCropR = m_cropR; m_dragCropB = m_cropB;
}

void CropWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (m_dragMode == 0) return;

    QRectF ir = imageRect();
    if (ir.isEmpty()) return;

    QPointF pos = event->position();
    float dx = (pos.x() - m_dragStart.x()) / ir.width();
    float dy = (pos.y() - m_dragStart.y()) / ir.height();
    float minS = 0.08f;

    float imgW = m_image.width();
    float imgH = m_image.height();
    float normAR = m_targetAR * imgH / imgW;

    switch (m_dragMode) {
    case 1: { // move
        float w = m_dragCropR - m_dragCropL;
        float h = m_dragCropB - m_dragCropT;
        float nl = qBound(0.0f, m_dragCropL + dx, 1.0f - w);
        float nt = qBound(0.0f, m_dragCropT + dy, 1.0f - h);
        m_cropL = nl; m_cropT = nt;
        m_cropR = nl + w; m_cropB = nt + h;
        break;
    }
    case 2: { // TL — 锚点 BR
        float nl = qBound(0.0f, m_dragCropL + dx, m_cropR - minS);
        float w = m_cropR - nl;
        float h = w / normAR;
        float nt = m_cropB - h;
        if (nt < 0) { nt = 0; h = m_cropB; w = h * normAR; nl = m_cropR - w; }
        if (w >= minS && h >= minS) { m_cropL = nl; m_cropT = nt; }
        break;
    }
    case 3: { // TR — 锚点 BL
        float nr = qBound(m_cropL + minS, m_dragCropR + dx, 1.0f);
        float w = nr - m_cropL;
        float h = w / normAR;
        float nt = m_cropB - h;
        if (nt < 0) { nt = 0; h = m_cropB; w = h * normAR; nr = m_cropL + w; }
        if (w >= minS && h >= minS) { m_cropR = nr; m_cropT = nt; }
        break;
    }
    case 4: { // BL — 锚点 TR
        float nl = qBound(0.0f, m_dragCropL + dx, m_cropR - minS);
        float w = m_cropR - nl;
        float h = w / normAR;
        float nb = m_cropT + h;
        if (nb > 1) { nb = 1; h = nb - m_cropT; w = h * normAR; nl = m_cropR - w; }
        if (w >= minS && h >= minS) { m_cropL = nl; m_cropB = nb; }
        break;
    }
    case 5: { // BR — 锚点 TL
        float nr = qBound(m_cropL + minS, m_dragCropR + dx, 1.0f);
        float w = nr - m_cropL;
        float h = w / normAR;
        float nb = m_cropT + h;
        if (nb > 1) { nb = 1; h = nb - m_cropT; w = h * normAR; nr = m_cropL + w; }
        if (w >= minS && h >= minS) { m_cropR = nr; m_cropB = nb; }
        break;
    }
    }
    update();
}

void CropWidget::mouseReleaseEvent(QMouseEvent *)
{
    m_dragMode = 0;
}

// ═══════════════════════════════════════════════════
//  CropImageDialog
// ═══════════════════════════════════════════════════

CropImageDialog::CropImageDialog(const QImage &originalBitmap, const MeshNode &node,
                                  int multicastCount, QWidget *parent)
    : QDialog(parent), m_node(node), m_rotatedBitmap(originalBitmap)
{
    m_resolutions = ImageUtils::resolutions();

    QString title;
    if (multicastCount > 0)
        title = tr("Crop Image → Multicast %1 nodes").arg(multicastCount);
    else
        title = tr("Crop Image → 0x%1").arg(node.addr, 4, 16, QChar('0')).toUpper();
    setWindowTitle(title);
    setMinimumSize(dp(350), dp(400));

    auto *layout = new QVBoxLayout(this);
    layout->setSpacing(dp(8));

    // 提示
    auto *hint = new QLabel(tr("Drag crop box to adjust area, drag corners to resize"));
    hint->setStyleSheet(QStringLiteral("color: #888; font-size: %1px;").arg(dp(11)));
    layout->addWidget(hint);

    // 分辨率选择
    auto *resRow = new QHBoxLayout;
    auto *resLabel = new QLabel(tr("Ratio"));
    resLabel->setStyleSheet(QStringLiteral("color: #B0BEC5; font-size: %1px;").arg(dp(12)));
    resRow->addWidget(resLabel);

    m_resCombo = new QComboBox;
    for (const auto &r : m_resolutions)
        m_resCombo->addItem(r.label);
    m_resCombo->setCurrentIndex(0);
    connect(m_resCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &CropImageDialog::onResolutionChanged);
    resRow->addWidget(m_resCombo);
    resRow->addStretch();

    // 旋转按钮
    auto *rotateBtn = new QPushButton(tr("Rotate"));
    connect(rotateBtn, &QPushButton::clicked, this, &CropImageDialog::onRotate);
    resRow->addWidget(rotateBtn);

    layout->addLayout(resRow);

    // 裁剪控件
    m_cropWidget = new CropWidget;
    m_cropWidget->setImage(m_rotatedBitmap);
    float ar = (float)m_resolutions[0].width / m_resolutions[0].height;
    m_cropWidget->setAspectRatio(ar);
    layout->addWidget(m_cropWidget, 1);

    // 按钮行
    auto *btnRow = new QHBoxLayout;
    btnRow->addStretch();
    auto *cancelBtn = new QPushButton(tr("Cancel"));
    cancelBtn->setFlat(true);
    cancelBtn->setStyleSheet(QStringLiteral("color: #888;"));
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    btnRow->addWidget(cancelBtn);

    auto *confirmBtn = new AAButton(tr("Confirm Crop"));
    confirmBtn->setStyleSheet(QStringLiteral(
        "AAButton { background: #4FC3F7; color: white; font-weight: bold; "
        "border-radius: %1px; padding: %2px %3px; font-size: %4px; }"
        "AAButton:hover { background: #29B6F6; }")
        .arg(dp(6)).arg(dp(8)).arg(dp(16)).arg(dp(13)));
    connect(confirmBtn, &QPushButton::clicked, this, &CropImageDialog::onConfirm);
    btnRow->addWidget(confirmBtn);
    layout->addLayout(btnRow);
}

void CropImageDialog::onResolutionChanged(int index)
{
    if (index < 0 || index >= m_resolutions.size()) return;
    float ar = (float)m_resolutions[index].width / m_resolutions[index].height;
    m_cropWidget->setAspectRatio(ar);
}

void CropImageDialog::onRotate()
{
    QTransform t;
    t.rotate(90);
    m_rotatedBitmap = m_rotatedBitmap.transformed(t, Qt::SmoothTransformation);
    m_cropWidget->setImage(m_rotatedBitmap);
}

void CropImageDialog::onConfirm()
{
    QRectF cr = m_cropWidget->cropRect();
    int imgW = m_rotatedBitmap.width();
    int imgH = m_rotatedBitmap.height();

    int px = qMax(0, (int)(cr.x() * imgW));
    int py = qMax(0, (int)(cr.y() * imgH));
    int pw = qMax(1, qMin((int)(cr.width() * imgW), imgW - px));
    int ph = qMax(1, qMin((int)(cr.height() * imgH), imgH - py));

    QImage cropped = m_rotatedBitmap.copy(px, py, pw, ph);
    int resIdx = m_resCombo->currentIndex();
    emit cropConfirmed(cropped, m_resolutions[resIdx]);
    accept();
}
