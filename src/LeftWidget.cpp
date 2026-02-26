#include "LeftWidget.hpp"
#include <QLabel>
#include <QPixmap>
#include <QIcon>
#include <QPainter>
#include <QPainterPath>

#include <QDebug>

LeftWidget::LeftWidget(QWidget* parent) : QWidget(parent)
{
    m_mainLayout = new QVBoxLayout(this);

    m_mainLayout->setSpacing(6);

    m_mainLayout->setContentsMargins(QMargins(10, 12, 10, 12));

    QPixmap texturedPixmap("./resources/VelixFire.png");

    if(texturedPixmap.isNull()) 
    {
        qWarning() << "Failed to load texture image!";
    }

    QIcon texturedIcon(texturedPixmap);
    auto* iconLabel = new QLabel(this);

    iconLabel->setPixmap(texturedIcon.pixmap(64, 64));
    iconLabel->setAlignment(Qt::AlignCenter);

    m_mainLayout->addWidget(iconLabel, 0, Qt::AlignCenter);

    addTab("Projects", "./resources/folder.png", this);
    addTab("Documentation", "./resources/document.png", this);
    addTab("Settings", "./resources/setting.png", this);
    addTab("Installs", "./resources/installs.png", this);

    m_tabs.first()->setActive(true);

    m_mainLayout->addStretch(10);
}

void LeftWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const QRectF bounds = rect().adjusted(1, 1, -1, -1);
    QPainterPath path;
    path.addRoundedRect(bounds, 14, 14);

    QLinearGradient gradient(bounds.topLeft(), bounds.bottomLeft());
    gradient.setColorAt(0.0, QColor(44, 44, 44, 220));
    gradient.setColorAt(1.0, QColor(31, 31, 31, 220));

    painter.fillPath(path, gradient);
    painter.setPen(QPen(QColor(64, 64, 64), 1));
    painter.drawPath(path);

    painter.setPen(QPen(QColor(255, 120, 0, 90), 1));
    painter.drawLine(bounds.topRight().toPoint(), bounds.bottomRight().toPoint());
}

void LeftWidget::addTab(const QString& tabName, const QString& iconPath, QWidget* parent)
{
    auto tab = new TabWidget(tabName, iconPath, parent);

    connect(tab, &TabWidget::clicked, this, [this, tab, tabName]
    {
        for(auto* tb : m_tabs)
            tb->setActive(false);

        tab->setActive(true);

        //TODO REFACTOR THIS PIECE OF SHIT
        emit tabWidgetChanged(tabName);
    });

    m_tabs.push_back(tab);

    m_mainLayout->addWidget(tab);
}
