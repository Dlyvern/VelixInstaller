#include "LeftWidget.hpp"
#include <QLabel>
#include <QPixmap>
#include <QIcon>
#include <QPushButton>

#include <QDebug>

LeftWidget::LeftWidget(QWidget* parent) : QWidget(parent)
{
    m_mainLayout = new QVBoxLayout(this);

    m_mainLayout->setSpacing(1);

    m_mainLayout->setContentsMargins(QMargins(5, 10, 10, 10));

    QPixmap texturedPixmap("./resources/VelixFire.png");

    if(texturedPixmap.isNull()) 
    {
        qWarning() << "Failed to load texture image!";
    }

    QIcon texturedIcon(texturedPixmap);
    auto *iconLabel = new QLabel;

    iconLabel->setPixmap(texturedIcon.pixmap(64, 64));

    m_mainLayout->addWidget(iconLabel, 0, Qt::AlignCenter);


    addTab("Projects", "./resources/folder.png", this);
    addTab("Documentation", "./resources/document.png", this);
    addTab("Settings", "./resources/setting.png", this);

    m_tabs.first()->setActive(true);

    m_mainLayout->addStretch(10);
}


void LeftWidget::addTab(const QString& tabName, const QString& iconPath, QWidget* parent)
{
    auto tab = new TabWidget(tabName, iconPath, parent);

    connect(tab, &TabWidget::clicked, this, [this, tab]
    {
        for(auto* tb : m_tabs)
            tb->setActive(false);

        tab->setActive(true);
    });

    m_tabs.push_back(tab);

    m_mainLayout->addWidget(tab);
}
