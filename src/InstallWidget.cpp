#include "InstallWidget.hpp"
#include "FireButton.hpp"

#include <filesystem>

InstallWidget::InstallWidget(QWidget* parent) : QWidget(parent)
{
    m_releaseChecker = new ReleaseChecker(this);
    m_versionsWidget = new VersionsWidget(this);
    m_fileDownloader = new FileDownloader(this);

    connect(m_versionsWidget, &VersionsWidget::launchVersion, this, &InstallWidget::onLaunchVersion);
    connect(m_versionsWidget, &VersionsWidget::installVersion, this, &InstallWidget::onDownloadVersion);

    m_config.load();
    m_installer.init();

    connect(m_releaseChecker, &ReleaseChecker::newVersionFound, this, &InstallWidget::onNewVersion);

    m_mainLayout = new QVBoxLayout(this);

    auto textLabel = new QLabel{"Available versions", this};
    textLabel->setStyleSheet( 
        "color: #D3D3D3;"
        "font-weight: bold;");

    m_speedLabel = new QLabel(this);
    m_speedLabel->setStyleSheet( 
    "color: #D3D3D3;"
    "font-weight: bold;");

    m_mainLayout->addWidget(textLabel);

    m_mainLayout->addWidget(m_versionsWidget);

    m_progressBar = new QProgressBar(this);
    m_progressBar->setTextVisible(false);

    m_progressBar->setStyleSheet(R"(
        QProgressBar {
            border: 2px solid #444;
            border-radius: 10px;
            background: #2E2E2E;
            height: 20px;
        }
        QProgressBar::chunk {
            background: qlineargradient(
                x1:0, y1:0, x2:1, y2:0,
                stop:0 #FF8008,
                stop:1 #FFC837
            );
            border-radius: 8px;
        }
    )");

    m_progressBar->hide();

    m_mainLayout->addWidget(m_progressBar);

    m_mainLayout->addWidget(m_speedLabel);

    m_mainLayout->addStretch(10);

    checkForInstalledVersions();
}

void InstallWidget::checkForInstalledVersions()
{
    auto config = m_config.getConfig();

    if(!config.contains("installed_versions"))
    {
        qDebug() << "No installed versions";
        return;
    }

    const auto& installedVersion = config["installed_versions"];

    for (const auto& v : installedVersion)
    {
        const std::string version = v["version"];
        const std::string path = v["path"];

        if(!std::filesystem::exists(path))
        {
            qDebug() << "Found version but path is not correct";
            continue;
        }

        //TODO make something with download link because leaving it empty is not good
        m_versionsWidget->addNewVersion(QString(version.c_str()), "", true);
    }
}

void InstallWidget::onNewVersion(const QString &tagName, const QString &downloadLink)
{
    bool isInstalled{false};

    if(m_config.getConfig().contains("installed_versions"))
    {
        const auto& installedVersion = m_config.getConfig()["installed_versions"];

        qDebug() << "Actual size: " << installedVersion.size();

        qDebug() << "Size: " << m_config.getConfig()["installed_versions"].size();
        
        for (const auto& v : m_config.getConfig()["installed_versions"])
        {
            const std::string version = v["version"];

            qDebug() << "Version json: " << QString(version.c_str());
            qDebug() << "Version: " << tagName;

            if (v["version"] == tagName.toStdString())
            {
                if(std::filesystem::exists(v["path"]))
                {
                    isInstalled = true;
                }
            }
        }
    }
    else
        qDebug() <<"Not found";

    m_versionsWidget->addNewVersion(tagName, downloadLink, isInstalled);
}

void InstallWidget::onLaunchVersion(const QString& tagName)
{
    qDebug() << "Launching " << tagName;
}

// {
//   "version": "0.0.0",
//   "installed_at": "2025-08-18",
//   "platform": "linux",
//   "path": "/home/dlyvern/.local/share/Velix/Velix_0.0.0/"
// }

void InstallWidget::onDownloadVersion(const QString& tagName, const QString& downloadLink)
{
    qDebug() << "Downloading " << tagName << " " << downloadLink;

    const QString dir{std::string(m_installer.getWorkPath().string() + "/Velix_" + tagName.toStdString() + "/Velix.zip").c_str()};

    qDebug() << dir;

    m_fileDownloader->init(dir);

    m_progressBar->show();

    m_releaseChecker->download(downloadLink);

    connect(m_releaseChecker, &ReleaseChecker::downloadProgressChanged, m_progressBar, [this](qint64 bytesReceived, qint64 bytesTotal)
    {
        m_progressBar->setRange(0, bytesTotal);
        m_progressBar->setValue(bytesReceived);
    });

    connect(m_releaseChecker, &ReleaseChecker::downloadSpeedChanged, m_speedLabel, [this](double kbPerSec)
    {
        m_speedLabel->setText(QString("%1 KB/s").arg(kbPerSec, 0, 'f', 2));
    });

    connect(m_releaseChecker, &ReleaseChecker::downloadDataReady, m_fileDownloader, &FileDownloader::writeChunk);

    connect(m_releaseChecker, &ReleaseChecker::downloadFinished, m_fileDownloader, [this]
    {
        m_fileDownloader->finish();

        qDebug() << "Installed";

        if(auto currentVersionWidget = m_versionsWidget->getCurrentVersionWidget())
            m_versionsWidget->getCurrentVersionWidget()->setInstalled(true);
    });
}