#include "MainWidget.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMap>

#include "widgets/VelixText.hpp"

MainWidget::MainWidget(QWidget* widget) : QWidget(widget)
{
    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(14, 10, 14, 14);
    mainLayout->setSpacing(10);

    auto headerWidget = new QWidget(this);
    auto headerLayout = new QHBoxLayout(headerWidget);
    headerLayout->setContentsMargins(0, 0, 0, 0);

    auto logoLabel = new VelixText{"VelixInstaller", this};
    logoLabel->setPointSize(18);

    auto subtitleLabel = new VelixText{"ENGINE PROJECT HUB", this};
    subtitleLabel->setPointSize(9);
    subtitleLabel->setBold(false);
    subtitleLabel->setTextColor(QColor(170, 170, 170));

    headerLayout->addWidget(logoLabel, 0, Qt::AlignLeft | Qt::AlignVCenter);
    headerLayout->addStretch(1);
    headerLayout->addWidget(subtitleLabel, 0, Qt::AlignRight | Qt::AlignVCenter);

    mainLayout->addWidget(headerWidget);

    m_stackedWidget = new QStackedWidget(this);
    m_stackedWidget->setContentsMargins(0, 0, 0, 0);

    m_projectWidget = new ProjectsWidget(m_stackedWidget);
    m_installWidget = new InstallWidget(m_stackedWidget);
    m_settingsWidget = new SettingsWidget(m_stackedWidget);
    m_documentationWidget = new DocumentationWidget(m_stackedWidget);

    m_updateChecker = new AppUpdateChecker(this);
    m_updateWidget  = new UpdateWidget(m_updateChecker, m_stackedWidget);

    connect(m_installWidget, &InstallWidget::installedVersionsChanged, m_settingsWidget, &SettingsWidget::reloadInstalledVersions);

    connect(m_updateChecker, &AppUpdateChecker::stableUpdateAvailable,
            m_updateWidget,  &UpdateWidget::onStableUpdateAvailable);
    connect(m_updateChecker, &AppUpdateChecker::unstableUpdateAvailable,
            m_updateWidget,  &UpdateWidget::onUnstableUpdateAvailable);
    connect(m_updateChecker, &AppUpdateChecker::noStableUpdate,
            m_updateWidget,  &UpdateWidget::onNoStableUpdate);
    connect(m_updateChecker, &AppUpdateChecker::noUnstableUpdate,
            m_updateWidget,  &UpdateWidget::onNoUnstableUpdate);
    connect(m_updateChecker, &AppUpdateChecker::checkFailed,
            m_updateWidget,  &UpdateWidget::onCheckFailed);
    connect(m_updateChecker, &AppUpdateChecker::stableUpdateAvailable,
            this, [this](const QString&, const QString&, const QString&){ emit updateAvailable(); });
    connect(m_updateChecker, &AppUpdateChecker::unstableUpdateAvailable,
            this, [this](const QString&, const QString&, const QString&){ emit updateAvailable(); });

    m_stackedWidget->addWidget(m_installWidget);
    m_stackedWidget->addWidget(m_projectWidget);
    m_stackedWidget->addWidget(m_settingsWidget);
    m_stackedWidget->addWidget(m_documentationWidget);
    m_stackedWidget->addWidget(m_updateWidget);

    m_stackedWidget->setCurrentWidget(m_projectWidget);

    mainLayout->addWidget(m_stackedWidget);
}

void MainWidget::changeWidget(const QString& widgetName)
{
    //TODO Refactor this
    const static QMap<QString, QWidget*> widgets
    {
        {"Projects",      m_projectWidget},
        {"Installs",      m_installWidget},
        {"Settings",      m_settingsWidget},
        {"Documentation", m_documentationWidget},
        {"Updates",       m_updateWidget}
    };

    auto it = widgets.find(widgetName);

    if(it != widgets.end())
        m_stackedWidget->setCurrentWidget(it.value());
    else
        qDebug() << "Could not find " << widgetName;
}
