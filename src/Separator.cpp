#include "Separator.hpp"

#include "Theme.hpp"

Separator::Separator(QWidget* parent) : QWidget(parent)
{
}

void Separator::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // 1px subtle vertical divider (matches sidebar's right border).
    painter.fillRect(rect(), theme::border);
}
