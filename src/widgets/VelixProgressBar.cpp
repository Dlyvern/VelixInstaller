#include "widgets/VelixProgressBar.hpp"

#include <algorithm>
#include <QRandomGenerator>

VelixProgressBar::VelixProgressBar(QWidget* parent) : QProgressBar(parent)
{
    setTextVisible(false);
    setMinimumHeight(20);
}

void VelixProgressBar::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const QRectF bounds = rect().adjusted(1, 1, -1, -1);
    QPainterPath backgroundPath;
    backgroundPath.addRoundedRect(bounds, m_cornerRadius, m_cornerRadius);

    painter.fillPath(backgroundPath, QColor(46, 46, 46));
    painter.setPen(QPen(QColor(68, 68, 68), 1));
    painter.drawPath(backgroundPath);

    const int barRange = maximum() - minimum();
    if (barRange <= 0)
        return;

    const qreal normalizedValue = std::clamp(
        static_cast<qreal>(value() - minimum()) / static_cast<qreal>(barRange),
        0.0,
        1.0
    );

    if (normalizedValue <= 0.0)
        return;

    QRectF progressRect = bounds;
    progressRect.setWidth(bounds.width() * normalizedValue);

    QPainterPath progressPath;
    progressPath.addRoundedRect(progressRect, m_cornerRadius - 1, m_cornerRadius - 1);

    QLinearGradient progressGradient(progressRect.left(), 0, progressRect.right(), 0);
    progressGradient.setColorAt(0.0, QColor("#FF8008"));
    progressGradient.setColorAt(1.0, QColor("#FFC837"));

    painter.fillPath(progressPath, progressGradient);

    painter.save();
    painter.setClipPath(progressPath);
    painter.setOpacity(0.15);
    for (int i = 0; i < width(); i += 3)
    {
        for (int j = 0; j < height(); j += 3)
        {
            int alpha = QRandomGenerator::global()->bounded(20, 60);
            painter.setPen(QColor(255, 220, 170, alpha));
            painter.drawPoint(i, j);
        }
    }
    painter.restore();
}
