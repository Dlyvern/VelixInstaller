#include "FireButton.hpp"

FireButton::FireButton(const QString& text, QWidget* parent)  : QPushButton(text, parent) 
{
    setCursor(Qt::PointingHandCursor);
    setStyleSheet("color: white; font-weight: bold;");
}

void FireButton::paintEvent(QPaintEvent*) 
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    QPainterPath path;
    path.addRoundedRect(rect(), m_cornerRadius, m_cornerRadius);
    
    QLinearGradient grad(0, 0, 0, height());
    grad.setColorAt(0, QColor(255, 80, 0));
    grad.setColorAt(1, QColor(220, 40, 0));
    
    painter.fillPath(path, grad);
    
    painter.setClipPath(path);
    painter.setOpacity(0.15);

    for (int i = 0; i < width(); i += 3) 
        for (int j = 0; j < height(); j += 3)
        {
            int alpha = QRandomGenerator::global()->bounded(30, 80);
            painter.setPen(QColor(255, 200, 100, alpha));
            painter.drawPoint(i, j);
        }
    
    painter.setOpacity(1.0);
    painter.setPen(QPen(QColor(255, 140, 0), 1));
    painter.drawPath(path);
    
    painter.setClipping(false);
    painter.setPen(Qt::white);
    painter.drawText(rect(), Qt::AlignCenter, text());
}
