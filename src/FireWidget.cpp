#include "FireWidget.hpp"
#include "FireButton.hpp"
#include <QVBoxLayout>

FireWidget::FireWidget(QWidget* parent) : QWidget(parent)
{
    setAttribute(Qt::WA_TranslucentBackground);

    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setAlignment(Qt::AlignCenter);

    auto button = new FireButton("Create new project", this);
    button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    mainLayout->addWidget(button);
    mainLayout->setContentsMargins(20, 20, 20, 20);
}

void FireWidget::setCornerRadius(int radius)
{
    m_cornerRadius = radius;
    update();
}

void FireWidget::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    QPainterPath path;
    path.addRoundedRect(rect(), m_cornerRadius, m_cornerRadius);
    
    painter.fillPath(path, m_bgColor);
    
    painter.setPen(QPen(QColor(255, 140, 0, 60), 1));
    painter.drawPath(path);
}
