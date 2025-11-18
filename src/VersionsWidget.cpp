#include "VersionsWidget.hpp"

VersionsWidget::VersionsWidget(QWidget* parent) : QWidget(parent)
{
    m_mainLayout = new QVBoxLayout(this);
}

void VersionsWidget::addNewVersion(const QString& tagName, const QString& downloadLink, bool isInstalled)
{
    auto* versionWidget = new VersionWidget(tagName, downloadLink, isInstalled, this);

    connect(versionWidget, &VersionWidget::clicked, this, &VersionsWidget::handleVersionClicked);

    connect(versionWidget, &VersionWidget::launchVersion, this, [this](const QString& tagName)
    {
         emit launchVersion(tagName);
    });

    connect(versionWidget, &VersionWidget::installVersion, this, [this](const QString& tagName, const QString& downloadLink)
    {
        emit installVersion(tagName, downloadLink);
    });
    
    m_versionWidgets.push_back(versionWidget);
    m_mainLayout->addWidget(versionWidget);
}

VersionWidget* VersionsWidget::getCurrentVersionWidget()
{
    return m_currentSelected;
}

void VersionsWidget::handleVersionClicked(VersionWidget* widget)
{
    if(m_currentSelected == widget)
        return;
    
    if (m_currentSelected)
        m_currentSelected->setSelected(false);

    widget->setSelected(true);
    
    m_currentSelected = widget;
}