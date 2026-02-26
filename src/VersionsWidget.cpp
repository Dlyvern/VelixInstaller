#include "VersionsWidget.hpp"

VersionsWidget::VersionsWidget(QWidget* parent) : QWidget(parent)
{
    m_mainLayout = new QVBoxLayout(this);
}

void VersionsWidget::addNewVersion(const QString& tagName, const QString& downloadLink, bool isInstalled)
{
    if (auto it = m_versionsByTag.find(tagName); it != m_versionsByTag.end())
    {
        if (isInstalled)
            it.value()->setInstalled(true);
        return;
    }

    auto* versionWidget = new VersionWidget(tagName, downloadLink, isInstalled, this);

    connect(versionWidget, &VersionWidget::clicked, this, &VersionsWidget::handleVersionClicked);

    connect(versionWidget, &VersionWidget::chooseVersion, this, [this](const QString& tagName)
    {
         emit chooseVersion(tagName);
    });

    connect(versionWidget, &VersionWidget::installVersion, this, [this](const QString& tagName, const QString& downloadLink)
    {
        emit installVersion(tagName, downloadLink);
    });
    
    m_versionWidgets.push_back(versionWidget);
    m_versionsByTag.insert(tagName, versionWidget);
    m_mainLayout->addWidget(versionWidget);
}

void VersionsWidget::setCurrentVersionTag(const QString& tagName)
{
    for (auto it = m_versionsByTag.begin(); it != m_versionsByTag.end(); ++it)
        it.value()->setCurrentVersion(it.key() == tagName);
}

void VersionsWidget::setVersionInstalled(const QString& tagName, bool isInstalled)
{
    if (auto it = m_versionsByTag.find(tagName); it != m_versionsByTag.end())
        it.value()->setInstalled(isInstalled);
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
