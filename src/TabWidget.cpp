#include "TabWidget.hpp"
#include <QHBoxLayout>
#include <QIcon>
#include <QPainter>
#include <QRadialGradient>
#include <QPainterPath>
#include <QRandomGenerator>
#include <QDebug>

TabWidget::TabWidget(const QString& tabName, const QString& iconPath, QWidget* parent) : QWidget(parent)
{
    auto mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(10, 4, 10, 4);
    mainLayout->setSpacing(10);

    m_originalPixMap.load(iconPath);

    if(m_originalPixMap.isNull()) 
        qWarning() << "Failed to load texture image!";

    QIcon texturedIcon(m_originalPixMap);
    m_labelIcon = new QLabel(this);
    
    m_labelIcon->setPixmap(texturedIcon.pixmap(16, 16));
    m_labelIcon->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    mainLayout->addWidget(m_labelIcon);

    auto textLabel = new QLabel(tabName, this);
    textLabel->setStyleSheet( 
            "color: #D3D3D3;"
            "font-weight: bold;");

    textLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
    mainLayout->addWidget(textLabel, 0, Qt::AlignLeft);

    mainLayout->addStretch(1);

    this->setMinimumSize(100, 50);

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
    // if (m_originalPixMap.isNull()) return;

    // QPixmap coloredPixmap = m_originalPixMap.scaled(16, 16, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    
    // if (m_isActive) {
    //     QPainter painter(&coloredPixmap);
    //     painter.setCompositionMode(QPainter::CompositionMode_SourceAtop);
    //     painter.fillRect(coloredPixmap.rect(), QColor(255, 100, 0, 200));
    // } else {
    //     QImage image = coloredPixmap.toImage();
    //     for (int y = 0; y < image.height(); y++) {
    //         for (int x = 0; x < image.width(); x++) {
    //             QRgb pixel = image.pixel(x, y);
    //             int gray = qGray(pixel);
    //             image.setPixel(x, y, qRgba(gray, gray, gray, qAlpha(pixel)));
    //         }
    //     }
    //     coloredPixmap = QPixmap::fromImage(image);
    // }
    
    // m_labelIcon->setPixmap(coloredPixmap);
}

void TabWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
        emit clicked();

    QWidget::mousePressEvent(event);
}

void TabWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QRectF rect = this->rect();
    QPainterPath path;
    path.addRoundedRect(rect, m_cornerRadius, m_cornerRadius);

    if (m_isActive) 
    {
        QLinearGradient grad(0, 0, 0, height());
        grad.setColorAt(0.0, QColor(255, 100, 0, 220));
        grad.setColorAt(0.5, QColor(255, 60, 0, 180)); 
        grad.setColorAt(1.0, QColor(180, 40, 0, 150));
        
        painter.fillPath(path, grad);

        painter.setClipPath(path);
        painter.setOpacity(0.15);
        for (int i = 0; i < width(); i += 3) {
            for (int j = 0; j < height(); j += 3) {
                int alpha = QRandomGenerator::global()->bounded(30, 80);
                painter.setPen(QColor(255, 200, 100, alpha));
                painter.drawPoint(i, j);
            }
        }

        painter.setOpacity(1.0);
        painter.setPen(QPen(QColor(255, 140, 0, 120), 1));
        painter.drawPath(path);
    }
}