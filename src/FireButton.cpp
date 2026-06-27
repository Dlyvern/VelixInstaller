#include "FireButton.hpp"

#include "Theme.hpp"

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
    setMinimumHeight(32);

    QFont currentFont = font();
    currentFont.setWeight(QFont::DemiBold);
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
    const QRectF bounds = rect().adjusted(0.5, 0.5, -0.5, -0.5);
    path.addRoundedRect(bounds, m_cornerRadius, m_cornerRadius);

    QColor topColor;
    QColor bottomColor;
    QColor borderColor;
    QColor textColor;

    if (m_variant == Variant::Primary)
    {
        topColor    = theme::accentSoft;
        bottomColor = theme::accentDeep;
        borderColor = theme::withAlpha(theme::accent, 153);  // ~60%
        textColor   = QColor(0xFF, 0xFF, 0xFF);
    }
    else
    {
        topColor    = theme::surface2;
        bottomColor = theme::surface1;
        borderColor = theme::border;
        textColor   = theme::text;
    }

    if (!isEnabled())
    {
        topColor.setAlpha(110);
        bottomColor.setAlpha(110);
        borderColor.setAlpha(120);
        textColor = theme::text3;
    }
    else if (m_isPressed)
    {
        topColor    = blendColor(topColor, -10);
        bottomColor = blendColor(bottomColor, -10);
    }
    else if (m_isHovered)
    {
        if (m_variant == Variant::Primary)
        {
            topColor    = theme::accent;
            bottomColor = theme::accentDeep;
            borderColor = theme::withAlpha(theme::accent, 200);
        }
        else
        {
            topColor    = theme::surface2Hover;
            bottomColor = theme::surface1Hover;
            borderColor = theme::borderHover;
        }
    }

    QLinearGradient grad(0, 0, 0, height());
    grad.setColorAt(0, topColor);
    grad.setColorAt(1, bottomColor);
    painter.fillPath(path, grad);

    painter.setPen(QPen(borderColor, 1));
    painter.drawPath(path);

    // Soft inner highlight on primary
    if (m_variant == Variant::Primary && isEnabled())
    {
        painter.setPen(QPen(QColor(255, 255, 255, 22), 1));
        painter.drawLine(QPointF(bounds.left() + m_cornerRadius, bounds.top() + 1),
                         QPointF(bounds.right() - m_cornerRadius, bounds.top() + 1));
    }

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
