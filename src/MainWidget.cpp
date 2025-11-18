#include "MainWidget.hpp"

#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QMap>

MainWidget::MainWidget(QWidget* widget) : QWidget(widget)
{
    auto mainLayout = new QVBoxLayout(this);
    auto logoLabel = new QLabel{"VelixInstaller", this};

    logoLabel->setStyleSheet( 
        "color: #D3D3D3;"
        "font-weight: bold;"
        "font-size: 18pt;"
    );

    mainLayout->addWidget(logoLabel, 0, Qt::AlignTop | Qt::AlignCenter);

    m_stackedWidget = new QStackedWidget(this);

    m_projectWidget = new ProjectsWidget(m_stackedWidget);
    m_installWidget = new InstallWidget(m_stackedWidget);
    m_settingsWidget = new SettingsWidget(m_stackedWidget);
    m_documentationWidget = new DocumentationWidget(m_stackedWidget);

    m_stackedWidget->addWidget(m_installWidget);
    m_stackedWidget->addWidget(m_projectWidget);
    m_stackedWidget->addWidget(m_settingsWidget);
    m_stackedWidget->addWidget(m_documentationWidget);

    m_stackedWidget->setCurrentWidget(m_projectWidget);

    mainLayout->addWidget(m_stackedWidget);
}

void MainWidget::changeWidget(const QString& widgetName)
{
    //TODO Refactor this
    const static QMap<QString, QWidget*> widgets
    {
        {"Projects", m_projectWidget},
        {"Installs", m_installWidget},
        {"Settings", m_settingsWidget},
        {"Documentation", m_documentationWidget}
    };

    auto it = widgets.find(widgetName);

    if(it != widgets.end())
        m_stackedWidget->setCurrentWidget(it.value());
    else
        qDebug() << "Could not find " << widgetName;
}