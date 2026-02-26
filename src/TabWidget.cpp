#include "TabWidget.hpp"
#include <QHBoxLayout>
#include <QPainter>
#include <QPainterPath>
#include <QRandomGenerator>
#include <QDebug>

TabWidget::TabWidget(const QString& tabName, const QString& iconPath, QWidget* parent) : QWidget(parent)
{
    auto mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(12, 6, 12, 6);
    mainLayout->setSpacing(9);

    m_originalPixMap.load(iconPath);

    if(m_originalPixMap.isNull()) 
        qWarning() << "Failed to load texture image!";

    m_labelIcon = new QLabel(this);
    m_labelIcon->setFixedSize(18, 18);
    m_labelIcon->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    mainLayout->addWidget(m_labelIcon);

    m_textLabel = new VelixText(tabName, this);
    m_textLabel->setPointSize(10);
    m_textLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
    mainLayout->addWidget(m_textLabel, 0, Qt::AlignLeft);

    mainLayout->addStretch(1);

    setMinimumHeight(44);
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

void TabWidget::updateIconColor()
{
    if (m_originalPixMap.isNull())
        return;

    QPixmap basePixmap = m_originalPixMap.scaled(16, 16, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    QPixmap colorized(basePixmap.size());
    colorized.fill(Qt::transparent);

    QColor tint = QColor(165, 165, 165);
    QColor textColor = QColor(190, 190, 190);

    if (m_isActive)
    {
        tint = QColor(255, 196, 124);
        textColor = QColor(244, 244, 244);
    }
    else if (m_isHovered)
    {
        tint = QColor(210, 210, 210);
        textColor = QColor(220, 220, 220);
    }

    QPainter iconPainter(&colorized);
    iconPainter.drawPixmap(0, 0, basePixmap);
    iconPainter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    iconPainter.fillRect(colorized.rect(), tint);
    iconPainter.end();

    m_labelIcon->setPixmap(colorized);
    m_textLabel->setTextColor(textColor);
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

    QRectF rect = this->rect().adjusted(1, 1, -1, -1);
    QPainterPath path;
    path.addRoundedRect(rect, m_cornerRadius, m_cornerRadius);

    QColor borderColor(52, 52, 52);
    if (m_isActive)
    {
        QLinearGradient grad(0, 0, 0, height());
        grad.setColorAt(0.0, QColor(255, 100, 0, 220));
        grad.setColorAt(0.6, QColor(220, 68, 0, 185));
        grad.setColorAt(1.0, QColor(130, 35, 0, 160));
        
        painter.fillPath(path, grad);
        borderColor = QColor(255, 140, 0, 120);

        painter.setClipPath(path);
        painter.setOpacity(0.12);
        for (int i = 0; i < width(); i += 3) {
            for (int j = 0; j < height(); j += 3) {
                int alpha = QRandomGenerator::global()->bounded(30, 80);
                painter.setPen(QColor(255, 200, 100, alpha));
                painter.drawPoint(i, j);
            }
        }
    }
    else if (m_isHovered)
    {
        QLinearGradient grad(0, 0, 0, height());
        grad.setColorAt(0.0, QColor(68, 68, 68, 220));
        grad.setColorAt(1.0, QColor(52, 52, 52, 220));
        painter.fillPath(path, grad);
        borderColor = QColor(88, 88, 88);
    }
    else
    {
        QLinearGradient grad(0, 0, 0, height());
        grad.setColorAt(0.0, QColor(52, 52, 52, 170));
        grad.setColorAt(1.0, QColor(40, 40, 40, 170));
        painter.fillPath(path, grad);
    }

    painter.setClipping(false);
    painter.setOpacity(1.0);
    painter.setPen(QPen(borderColor, 1));
    painter.drawPath(path);
}
