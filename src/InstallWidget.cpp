#include "InstallWidget.hpp"
#include "VelixConfirmDialog.hpp"
#include "ToastNotification.hpp"

#include <filesystem>
#include <algorithm>
#include <limits>
#include <QDate>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QInputDialog>
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
    connect(m_versionsWidget, &VersionsWidget::deleteVersion, this, &InstallWidget::onDeleteVersion);

    m_config.load();
    m_installer.init();

    connect(m_releaseChecker, &ReleaseChecker::newVersionFound, this, &InstallWidget::onNewVersion);
    connect(m_releaseChecker, &ReleaseChecker::releaseFetchFailed, this, &InstallWidget::onReleaseFetchFailed);
    bindDownloadHandlers();

    m_mainLayout = new QVBoxLayout(this);

    auto textLabel = new VelixText{"Available versions", this};
    textLabel->setPointSize(12);

    m_speedLabel = new VelixText(this);
    m_speedLabel->setPointSize(10);

    m_noInternetLabel = new VelixText("No internet connection — available versions cannot be loaded.", this);
    m_noInternetLabel->setPointSize(10);
    m_noInternetLabel->setTextColor(QColor(200, 80, 40));
    m_noInternetLabel->hide();

    m_mainLayout->addWidget(textLabel);
    m_mainLayout->addWidget(m_noInternetLabel);
    m_mainLayout->addWidget(m_versionsWidget);

    m_progressBar = new VelixProgressBar(this);
    m_progressBar->hide();

    m_mainLayout->addWidget(m_progressBar);
    m_mainLayout->addWidget(m_speedLabel);

    // ── Developer: add a local build ────────────────────────────────────────
    auto* devSeparator = new QWidget(this);
    devSeparator->setFixedHeight(1);
    devSeparator->setStyleSheet("background-color: #2e2e2e;");
    m_mainLayout->addWidget(devSeparator);

    auto* devRow = new QHBoxLayout();
    devRow->setSpacing(10);

    auto* devButton = new FireButton("Add Dev Build", FireButton::Variant::Secondary, this);
    devButton->setFixedWidth(140);
    connect(devButton, &QPushButton::clicked, this, &InstallWidget::onAddDevBuild);

    auto* devWarning = new VelixText("[DEV ONLY] Registers a local engine build. Use only if you know what you are doing.", this);
    devWarning->setPointSize(9);
    devWarning->setTextColor(QColor(160, 120, 40));

    devRow->addWidget(devButton);
    devRow->addWidget(devWarning, 1);
    m_mainLayout->addLayout(devRow);

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

        // Avoid Windows max macro collision with numeric_limits::max().
        const int safeTotal = static_cast<int>(std::min<qint64>(bytesTotal, (std::numeric_limits<int>::max)()));
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
        ToastNotification::show(QString("Download failed: %1").arg(error), ToastType::Error, this);
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
            ToastNotification::show(QString("Extraction failed: %1").arg(extractionError), ToastType::Error, this);
            qWarning() << "Extraction failed:" << extractionError;
            return;
        }

        QFile::remove(archivePath);

        m_speedLabel->setText("Installed");
        ToastNotification::show(
            QString("Velix %1 installed successfully!").arg(m_currentDownloadTag),
            ToastType::Success, this);

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
        ToastNotification::show(QString("Default version set to %1").arg(tagName), ToastType::Info, this);
        emit installedVersionsChanged();
    }
    else
    {
        m_speedLabel->setText("Failed to save default version");
        ToastNotification::show("Failed to save default version", ToastType::Error, this);
    }

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

void InstallWidget::onReleaseFetchFailed()
{
    m_noInternetLabel->show();
}

void InstallWidget::onAddDevBuild()
{
    if (!VelixConfirmDialog::ask(
            "Developer Feature",
            "Developer use only.\n\n"
            "Registers a local engine build from a folder you select. "
            "No validation is performed.\n\n"
            "Only proceed if you know what you are doing.",
            "Continue",
            "Cancel",
            this))
    {
        return;
    }

    const QString buildDir = QFileDialog::getExistingDirectory(
        this,
        "Select engine build folder",
        QString(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
    );

    if (buildDir.isEmpty())
        return;

    bool ok = false;
    const QString name = QInputDialog::getText(
        this,
        "Dev Build Name",
        "Enter a label for this build (e.g. dev-local, test-0.1):",
        QLineEdit::Normal,
        "dev-local",
        &ok
    );

    if (!ok || name.trimmed().isEmpty())
        return;

    const QString tag = name.trimmed();
    m_versionsWidget->addNewVersion(tag, "", true);
    upsertInstalledVersion(tag, buildDir);
    applyCurrentVersionToWidgets();

    ToastNotification::show(QString("Dev build \"%1\" added").arg(tag), ToastType::Success, this);
}

void InstallWidget::onDeleteVersion(const QString& tagName)
{
    if (!VelixConfirmDialog::ask(
            "Remove Version",
            QString("Remove Velix %1 from this machine?\n\nThe installed files will be permanently deleted.").arg(tagName),
            "Remove",
            "Cancel",
            this))
    {
        return;
    }

    auto& config = m_config.mutableConfig();

    QString installPath;
    if (config.contains("installed_versions") && config["installed_versions"].is_array())
    {
        auto& versions = config["installed_versions"];
        for (auto it = versions.begin(); it != versions.end(); ++it)
        {
            if (it->contains("version") && (*it)["version"].is_string() &&
                (*it)["version"].get<std::string>() == tagName.toStdString())
            {
                if (it->contains("path") && (*it)["path"].is_string())
                    installPath = QString::fromStdString((*it)["path"].get<std::string>());
                versions.erase(it);
                break;
            }
        }
    }

    if (config.contains("current_version") && config["current_version"].is_string() &&
        config["current_version"].get<std::string>() == tagName.toStdString())
    {
        config.erase("current_version");
    }

    m_config.save();

    if (!installPath.isEmpty())
    {
        std::error_code ec;
        std::filesystem::remove_all(installPath.toStdString(), ec);
        if (ec)
            qWarning() << "Failed to delete install directory:" << installPath;
    }

    m_versionsWidget->setVersionInstalled(tagName, false);
    applyCurrentVersionToWidgets();

    m_speedLabel->setText(QString("Removed %1").arg(tagName));
    ToastNotification::show(QString("Velix %1 removed").arg(tagName), ToastType::Warning, this);
    emit installedVersionsChanged();
}
