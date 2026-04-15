#include "MainWidget.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMap>
#include <QPushButton>

#include "widgets/VelixText.hpp"
#include "widgets/VelixProgressBar.hpp"

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

    m_samplesWidget = new SamplesWidget(m_stackedWidget);

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
    m_stackedWidget->addWidget(m_samplesWidget);
    m_stackedWidget->addWidget(m_settingsWidget);
    m_stackedWidget->addWidget(m_documentationWidget);
    m_stackedWidget->addWidget(m_updateWidget);

    m_stackedWidget->setCurrentWidget(m_projectWidget);

    mainLayout->addWidget(m_stackedWidget);

    // ── Download status bar (hidden until download starts) ────────────────
    m_downloadBar = new QWidget(this);
    m_downloadBar->setFixedHeight(36);
    m_downloadBar->setStyleSheet(
        "background: #1a1a1a;"
        "border-top: 1px solid #333;"
        "border-radius: 8px;");
    m_downloadBar->hide();

    auto* barLayout = new QHBoxLayout(m_downloadBar);
    barLayout->setContentsMargins(12, 0, 8, 0);
    barLayout->setSpacing(10);

    m_downloadBarLabel = new QLabel("Downloading update…  0%", m_downloadBar);
    m_downloadBarLabel->setStyleSheet("color: #ccc; font-size: 9pt;");

    auto* barProgress = new VelixProgressBar(m_downloadBar);
    barProgress->setObjectName("barProgress");
    barProgress->setRange(0, 100);
    barProgress->setValue(0);
    barProgress->setFixedHeight(6);

    auto* showBtn = new QPushButton("Show progress", m_downloadBar);
    showBtn->setFixedHeight(24);
    showBtn->setStyleSheet(
        "QPushButton {"
        "  background: #2a2a2a; color: #ff6a00; border: 1px solid #444;"
        "  border-radius: 6px; padding: 0 10px; font-size: 9pt;"
        "}"
        "QPushButton:hover { background: #333; }");
    connect(showBtn, &QPushButton::clicked, m_updateWidget, &UpdateWidget::showDownloadDialog);

    barLayout->addWidget(m_downloadBarLabel, 0);
    barLayout->addWidget(barProgress, 1);
    barLayout->addWidget(showBtn, 0);

    mainLayout->addWidget(m_downloadBar);

    // Wire download signals from UpdateWidget
    connect(m_updateWidget, &UpdateWidget::downloadStarted, this, &MainWidget::onDownloadStarted);
    connect(m_updateWidget, &UpdateWidget::downloadProgressChanged, this, &MainWidget::onDownloadProgress);
    connect(m_updateWidget, &UpdateWidget::downloadEnded, this, &MainWidget::onDownloadEnded);
}

void MainWidget::onDownloadStarted(const QString& version)
{
    m_downloadBarLabel->setText(QString("Downloading update %1  \u2014  0%").arg(version));
    if (auto* bar = m_downloadBar->findChild<VelixProgressBar*>("barProgress"))
        bar->setValue(0);
    m_downloadBar->show();
}

void MainWidget::onDownloadProgress(int percent)
{
    const QString base = m_downloadBarLabel->text().section(QChar(0x2014), 0, 0).trimmed();
    m_downloadBarLabel->setText(QString("%1  \u2014  %2%").arg(base).arg(percent));
    if (auto* bar = m_downloadBar->findChild<VelixProgressBar*>("barProgress"))
        bar->setValue(percent);
}

void MainWidget::onDownloadEnded()
{
    m_downloadBar->hide();
}

void MainWidget::changeWidget(const QString& widgetName)
{
    //TODO Refactor this
    const static QMap<QString, QWidget*> widgets
    {
        {"Projects",      m_projectWidget},
        {"Samples",       m_samplesWidget},
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
