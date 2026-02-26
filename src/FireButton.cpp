#include "FireButton.hpp"

namespace
{
QColor blendColor(const QColor& base, int delta)
{
    return base.lighter(100 + delta);
}
}

FireButton::FireButton(const QString& text, QWidget* parent)  : FireButton(text, Variant::Primary, parent)
{
}

FireButton::FireButton(const QString& text, Variant variant, QWidget* parent) : QPushButton(text, parent), m_variant(variant)
{
    setCursor(Qt::PointingHandCursor);
    setMinimumHeight(34);

    QFont currentFont = font();
    currentFont.setBold(true);
    setFont(currentFont);
}

void FireButton::setVariant(FireButton::Variant variant)
{
    if (m_variant == variant)
        return;

    m_variant = variant;
    update();
}

void FireButton::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    QPainterPath path;
    const QRectF bounds = rect().adjusted(1, 1, -1, -1);
    path.addRoundedRect(bounds, m_cornerRadius, m_cornerRadius);

    QColor topColor;
    QColor bottomColor;
    QColor borderColor;

    if (m_variant == Variant::Primary)
    {
        topColor = QColor(255, 80, 0);
        bottomColor = QColor(220, 40, 0);
        borderColor = QColor(255, 140, 0);
    }
    else
    {
        topColor = QColor(72, 72, 72);
        bottomColor = QColor(46, 46, 46);
        borderColor = QColor(104, 104, 104);
    }

    if (!isEnabled())
    {
        topColor = QColor(54, 54, 54);
        bottomColor = QColor(44, 44, 44);
        borderColor = QColor(70, 70, 70);
    }
    else if (m_isPressed)
    {
        topColor = blendColor(topColor, -20);
        bottomColor = blendColor(bottomColor, -20);
    }
    else if (m_isHovered)
    {
        topColor = blendColor(topColor, 15);
        bottomColor = blendColor(bottomColor, 10);
    }

    QLinearGradient grad(0, 0, 0, height());
    grad.setColorAt(0, topColor);
    grad.setColorAt(1, bottomColor);
    painter.fillPath(path, grad);
    
    painter.save();
    painter.setClipPath(path);
    painter.setOpacity(m_variant == Variant::Primary ? 0.12 : 0.05);

    const QColor noiseColor = m_variant == Variant::Primary ? QColor(255, 200, 100) : QColor(155, 155, 155);
    for (int i = 0; i < width(); i += 3)
        for (int j = 0; j < height(); j += 3)
        {
            int alpha = QRandomGenerator::global()->bounded(30, 80);
            painter.setPen(QColor(noiseColor.red(), noiseColor.green(), noiseColor.blue(), alpha));
            painter.drawPoint(i, j);
        }
    painter.restore();

    painter.setPen(QPen(borderColor, 1));
    painter.drawPath(path);

    QColor textColor = isEnabled() ? QColor(240, 240, 240) : QColor(140, 140, 140);
    painter.setPen(textColor);
    painter.drawText(bounds.toRect(), Qt::AlignCenter, text());
}

void FireButton::enterEvent(QEnterEvent* event)
{
    m_isHovered = true;
    update();
    QPushButton::enterEvent(event);
}

void FireButton::leaveEvent(QEvent* event)
{
    m_isHovered = false;
    m_isPressed = false;
    update();
    QPushButton::leaveEvent(event);
}

void FireButton::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
    {
        m_isPressed = true;
        update();
    }

    QPushButton::mousePressEvent(event);
}

void FireButton::mouseReleaseEvent(QMouseEvent* event)
{
    m_isPressed = false;
    update();
    QPushButton::mouseReleaseEvent(event);
}
