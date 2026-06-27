#include "TabWidget.hpp"
#include <QHBoxLayout>
#include <QPainter>
#include <QPainterPath>
#include <QDebug>

#include "Theme.hpp"

TabWidget::TabWidget(const QString& tabName, const QString& iconPath, QWidget* parent) : QWidget(parent)
{
    auto mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(14, 6, 12, 6);
    mainLayout->setSpacing(10);

    if (iconPath == QStringLiteral(":home"))
    {
        m_iconKind = IconKind::Home;
    }
    else
    {
        m_originalPixMap.load(iconPath);
        if (m_originalPixMap.isNull())
            qWarning() << "Failed to load texture image!" << iconPath;
    }

    m_labelIcon = new QLabel(this);
    m_labelIcon->setFixedSize(18, 18);
    m_labelIcon->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    mainLayout->addWidget(m_labelIcon);

    m_textLabel = new VelixText(tabName, this);
    m_textLabel->setPointSize(10);
    m_textLabel->setBold(false);
    m_textLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
    mainLayout->addWidget(m_textLabel, 0, Qt::AlignLeft);

    mainLayout->addStretch(1);

    setMinimumHeight(36);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setCursor(Qt::PointingHandCursor);

    updateIconColor();
}

void TabWidget::setActive(bool isActive)
{
    m_isActive = isActive;
    updateIconColor();
    update();
}

void TabWidget::setHasBadge(bool hasBadge)
{
    m_hasBadge = hasBadge;
    update();
}

namespace
{
QPixmap renderHomeIcon(const QColor& tint, int size)
{
    QPixmap px(size, size);
    px.fill(Qt::transparent);
    QPainter p(&px);
    p.setRenderHint(QPainter::Antialiasing);
    QPen pen(tint, 1.6);
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    p.setPen(pen);
    const qreal s = size / 20.0;
    QPainterPath roof;
    roof.moveTo(3 * s, 10 * s);
    roof.lineTo(10 * s, 4 * s);
    roof.lineTo(17 * s, 10 * s);
    p.drawPath(roof);
    QPainterPath body;
    body.moveTo(5 * s, 9 * s);
    body.lineTo(5 * s, 16 * s);
    body.lineTo(15 * s, 16 * s);
    body.lineTo(15 * s, 9 * s);
    p.drawPath(body);
    return px;
}
}

void TabWidget::updateIconColor()
{
    QColor tint = theme::text3;
    QColor textColor = theme::text2;

    if (m_isActive)
    {
        tint = theme::accentBright;
        textColor = theme::accentBright;
    }
    else if (m_isHovered)
    {
        tint = theme::text2;
        textColor = theme::text;
    }

    m_textLabel->setTextColor(textColor);

    if (m_iconKind == IconKind::Home)
    {
        m_labelIcon->setPixmap(renderHomeIcon(tint, 16));
        return;
    }

    if (m_originalPixMap.isNull())
        return;

    QPixmap basePixmap = m_originalPixMap.scaled(16, 16, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    QPixmap colorized(basePixmap.size());
    colorized.fill(Qt::transparent);

    QPainter iconPainter(&colorized);
    iconPainter.drawPixmap(0, 0, basePixmap);
    iconPainter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    iconPainter.fillRect(colorized.rect(), tint);
    iconPainter.end();

    m_labelIcon->setPixmap(colorized);
}

void TabWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
        emit clicked();

    QWidget::mousePressEvent(event);
}

void TabWidget::enterEvent(QEnterEvent* event)
{
    m_isHovered = true;
    updateIconColor();
    update();
    QWidget::enterEvent(event);
}

void TabWidget::leaveEvent(QEvent* event)
{
    m_isHovered = false;
    updateIconColor();
    update();
    QWidget::leaveEvent(event);
}

void TabWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const QRectF bounds = rect().adjusted(0, 0, -1, -1);
    QPainterPath path;
    path.addRoundedRect(bounds, m_cornerRadius, m_cornerRadius);

    if (m_isActive)
    {
        painter.fillPath(path, theme::withAlpha(theme::accent, 31));   // ~12% alpha
    }
    else if (m_isHovered)
    {
        painter.fillPath(path, theme::surface1);
    }
    // else: transparent — let sidebar background show through

    if (m_isActive)
    {
        // 3px left accent bar
        QPainterPath bar;
        const QRectF barRect(bounds.left(), bounds.top() + 6, 3, bounds.height() - 12);
        bar.addRoundedRect(barRect, 1.5, 1.5);
        QLinearGradient grad(barRect.topLeft(), barRect.bottomLeft());
        grad.setColorAt(0.0, theme::accent);
        grad.setColorAt(1.0, theme::accentDeep);
        painter.fillPath(bar, grad);
    }

    if (m_hasBadge)
    {
        painter.setPen(Qt::NoPen);
        painter.setBrush(theme::accent);
        painter.drawEllipse(width() - 14, height() / 2 - 3, 6, 6);
    }
}
