#include "Separator.hpp"

Separator::Separator(QWidget* parent) : QWidget(parent)
{
}

void Separator::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QColor centerColor(255, 165, 0, 220);
    QColor edgeColor(255, 165, 0, 50);
    QLinearGradient grad(width()/2, 0, width()/2, height());
    grad.setColorAt(0, Qt::transparent);
    grad.setColorAt(0.3, edgeColor);
    grad.setColorAt(0.5, centerColor);
    grad.setColorAt(0.7, edgeColor);
    grad.setColorAt(1, Qt::transparent);
    
    painter.fillRect(rect(), grad);
}
