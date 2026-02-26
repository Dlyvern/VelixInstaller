#include "InstallWidget.hpp"

#include <filesystem>
#include <algorithm>
#include <limits>
#include <QDate>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QProcess>
#include <QDebug>

namespace
{
std::string currentPlatformName()
{
#ifdef _WIN32
    return "windows";
#elif defined(__APPLE__)
    return "macos";
#else
    return "linux";
#endif
}
}

InstallWidget::InstallWidget(QWidget* parent) : QWidget(parent)
{
    m_releaseChecker = new ReleaseChecker(this);
    m_versionsWidget = new VersionsWidget(this);
    m_fileDownloader = new FileDownloader(this);

    connect(m_versionsWidget, &VersionsWidget::chooseVersion, this, &InstallWidget::onChooseVersion);
    connect(m_versionsWidget, &VersionsWidget::installVersion, this, &InstallWidget::onDownloadVersion);

    m_config.load();
    m_installer.init();

    connect(m_releaseChecker, &ReleaseChecker::newVersionFound, this, &InstallWidget::onNewVersion);
    bindDownloadHandlers();

    m_mainLayout = new QVBoxLayout(this);

    auto textLabel = new VelixText{"Available versions", this};
    textLabel->setPointSize(12);

    m_speedLabel = new VelixText(this);
    m_speedLabel->setPointSize(10);

    m_mainLayout->addWidget(textLabel);

    m_mainLayout->addWidget(m_versionsWidget);

    m_progressBar = new VelixProgressBar(this);

    m_progressBar->hide();

    m_mainLayout->addWidget(m_progressBar);

    m_mainLayout->addWidget(m_speedLabel);

    m_mainLayout->addStretch(10);

    checkForInstalledVersions();
}

void InstallWidget::bindDownloadHandlers()
{
    connect(m_releaseChecker, &ReleaseChecker::downloadProgressChanged, this, [this](qint64 bytesReceived, qint64 bytesTotal)
    {
        if (!m_downloadInProgress)
            return;

        if (bytesTotal <= 0)
            return;

        const int safeTotal = static_cast<int>(std::min<qint64>(bytesTotal, std::numeric_limits<int>::max()));
        const int safeCurrent = static_cast<int>(std::min<qint64>(bytesReceived, static_cast<qint64>(safeTotal)));

        m_progressBar->setRange(0, safeTotal);
        m_progressBar->setValue(safeCurrent);
    });

    connect(m_releaseChecker, &ReleaseChecker::downloadSpeedChanged, this, [this](double kbPerSec)
    {
        if (!m_downloadInProgress)
            return;

        m_speedLabel->setText(QString("%1 KB/s").arg(kbPerSec, 0, 'f', 2));
    });

    connect(m_releaseChecker, &ReleaseChecker::downloadDataReady, m_fileDownloader, &FileDownloader::writeChunk);

    connect(m_releaseChecker, &ReleaseChecker::downloadError, this, [this](const QString& error)
    {
        m_fileDownloader->finish();
        m_downloadInProgress = false;
        m_progressBar->hide();
        m_speedLabel->setText(QString("Download failed: %1").arg(error));
        qWarning() << "Download failed:" << error;
    });

    connect(m_releaseChecker, &ReleaseChecker::downloadFinished, this, [this]
    {
        m_fileDownloader->finish();
        m_downloadInProgress = false;

        const QString archivePath = QDir(m_currentInstallPath).filePath("Velix.zip");
        QString extractionError;
        m_speedLabel->setText("Extracting...");

        if (!extractArchive(archivePath, m_currentInstallPath, extractionError))
        {
            m_speedLabel->setText(QString("Extraction failed: %1").arg(extractionError));
            qWarning() << "Extraction failed:" << extractionError;
            return;
        }

        m_speedLabel->setText("Installed");

        if (!m_currentDownloadTag.isEmpty())
            m_versionsWidget->setVersionInstalled(m_currentDownloadTag, true);

        if (!m_currentDownloadTag.isEmpty() && !m_currentInstallPath.isEmpty())
            upsertInstalledVersion(m_currentDownloadTag, m_currentInstallPath);

        applyCurrentVersionToWidgets();
    });
}

bool InstallWidget::extractArchive(const QString& archivePath, const QString& destinationDir, QString& errorMessage)
{
    if (!QFileInfo::exists(archivePath))
    {
        errorMessage = QString("Archive not found: %1").arg(archivePath);
        return false;
    }

    QDir().mkpath(destinationDir);

#ifdef _WIN32
    QProcess extractProcess;
    QStringList args;
    args << "-NoProfile"
         << "-ExecutionPolicy" << "Bypass"
         << "-Command"
         << QString("Expand-Archive -LiteralPath \"%1\" -DestinationPath \"%2\" -Force")
                .arg(archivePath, destinationDir);

    extractProcess.start("powershell", args);
    if (!extractProcess.waitForFinished(-1) || extractProcess.exitCode() != 0)
    {
        errorMessage = QString::fromLocal8Bit(extractProcess.readAllStandardError()).trimmed();
        if (errorMessage.isEmpty())
            errorMessage = "PowerShell Expand-Archive failed";
        return false;
    }

    return true;
#else
    QProcess unzipProcess;
    unzipProcess.start("unzip", {"-o", archivePath, "-d", destinationDir});
    if (unzipProcess.waitForFinished(-1) && unzipProcess.exitCode() == 0)
        return true;

    QProcess tarProcess;
    tarProcess.start("tar", {"-xf", archivePath, "-C", destinationDir});
    if (tarProcess.waitForFinished(-1) && tarProcess.exitCode() == 0)
        return true;

    QString unzipError = QString::fromLocal8Bit(unzipProcess.readAllStandardError()).trimmed();
    QString tarError = QString::fromLocal8Bit(tarProcess.readAllStandardError()).trimmed();
    errorMessage = QString("unzip failed (%1); tar failed (%2)").arg(unzipError, tarError);
    return false;
#endif
}

void InstallWidget::applyCurrentVersionToWidgets()
{
    const auto& config = m_config.getConfig();
    if (!config.contains("current_version") || !config["current_version"].is_string())
        return;

    const QString currentVersion = QString::fromStdString(config["current_version"].get<std::string>());
    if (currentVersion.isEmpty())
        return;

    m_versionsWidget->setCurrentVersionTag(currentVersion);
}

void InstallWidget::checkForInstalledVersions()
{
    const auto& config = m_config.getConfig();

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

        m_versionsWidget->addNewVersion(QString(version.c_str()), "", true);
    }

    applyCurrentVersionToWidgets();
}

void InstallWidget::onNewVersion(const QString &tagName, const QString &downloadLink)
{
    bool isInstalled{false};
    const auto& config = m_config.getConfig();

    if(config.contains("installed_versions"))
    {
        for (const auto& v : config["installed_versions"])
        {
            if (v.contains("version") && v["version"].is_string() && v["version"] == tagName.toStdString())
            {
                if (v.contains("path") && v["path"].is_string() && std::filesystem::exists(v["path"]))
                    isInstalled = true;
            }
        }
    }

    m_versionsWidget->addNewVersion(tagName, downloadLink, isInstalled);
    applyCurrentVersionToWidgets();
}

void InstallWidget::onChooseVersion(const QString& tagName)
{
    auto& config = m_config.mutableConfig();
    config["current_version"] = tagName.toStdString();
    if (m_config.save())
    {
        m_speedLabel->setText(QString("Default version set to %1").arg(tagName));
        emit installedVersionsChanged();
    }
    else
        m_speedLabel->setText("Failed to save default version");

    applyCurrentVersionToWidgets();
}

QString InstallWidget::chooseInstallDirectory(const QString& tagName)
{
    const auto& config = m_config.getConfig();
    QString defaultRoot = QString::fromStdString(m_installer.getWorkPath().string());

    if (config.contains("install_root") && config["install_root"].is_string())
        defaultRoot = QString::fromStdString(config["install_root"].get<std::string>());

    QDir().mkpath(defaultRoot);

    const QString selectedRoot = QFileDialog::getExistingDirectory(
        this,
        QString("Choose where Velix %1 will be installed").arg(tagName),
        defaultRoot,
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
    );

    if (selectedRoot.isEmpty())
        return {};

    return QDir(selectedRoot).filePath(QString("Velix_%1").arg(tagName));
}

void InstallWidget::upsertInstalledVersion(const QString& tagName, const QString& installPath)
{
    auto& config = m_config.mutableConfig();

    if (!config.contains("installed_versions") || !config["installed_versions"].is_array())
        config["installed_versions"] = nlohmann::json::array();

    nlohmann::json entry = {
        {"version", tagName.toStdString()},
        {"installed_at", QDate::currentDate().toString("yyyy-MM-dd").toStdString()},
        {"platform", currentPlatformName()},
        {"path", installPath.toStdString()}
    };

    bool updated = false;
    for (auto& item : config["installed_versions"])
    {
        if (item.contains("version") && item["version"].is_string() && item["version"] == tagName.toStdString())
        {
            item = entry;
            updated = true;
            break;
        }
    }

    if (!updated)
        config["installed_versions"].push_back(entry);

    if (!config.contains("current_version") || !config["current_version"].is_string() || config["current_version"].get<std::string>().empty())
        config["current_version"] = tagName.toStdString();

    config["install_root"] = QFileInfo(installPath).absolutePath().toStdString();
    if (m_config.save())
        emit installedVersionsChanged();
}

void InstallWidget::onDownloadVersion(const QString& tagName, const QString& downloadLink)
{
    if (m_downloadInProgress)
    {
        qWarning() << "Download already in progress";
        return;
    }

    const QString installDir = chooseInstallDirectory(tagName);
    if (installDir.isEmpty())
        return;

    QDir().mkpath(installDir);

    const QString archivePath = QDir(installDir).filePath("Velix.zip");
    if (!m_fileDownloader->init(archivePath))
    {
        m_speedLabel->setText(QString("Failed to write %1").arg(archivePath));
        return;
    }

    m_currentDownloadTag = tagName;
    m_currentInstallPath = installDir;
    m_downloadInProgress = true;

    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->show();
    m_speedLabel->setText(QString("Downloading %1...").arg(tagName));

    m_releaseChecker->download(downloadLink);
}
