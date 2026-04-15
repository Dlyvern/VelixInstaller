#include "UpdateWidget.hpp"
#include "AppVersion.hpp"
#include "Config.hpp"
#include "ToastNotification.hpp"
#include "VelixConfirmDialog.hpp"

#include <QDir>
#include <QDirIterator>
#include <QProcess>
#include <QCoreApplication>
#include <QHBoxLayout>
#include <QFrame>
#include <QPalette>
#include <QTextStream>
#include <QDebug>

namespace
{
// Extract the new binary from the downloaded zip and copy it next to the running binary.
// Returns the path to the extracted binary, or empty on failure.
QString extractAndStage(const QString& zipPath)
{
    // We use miniz via unzip process for simplicity — same pattern as Installer.
    // Find the binary entry inside the zip (first non-directory file that looks like our executable).
    const QString appPath = QCoreApplication::applicationFilePath();
    const QFileInfo appInfo(appPath);
    const QString binaryName = appInfo.fileName();
    const QString destPath   = appInfo.dir().filePath(binaryName + ".new");

#ifdef _WIN32
    // On Windows, use PowerShell to extract.
    QStringList args;
    args << "-Command"
         << QString("Expand-Archive -Force '%1' '%2'; "
                    "Copy-Item -Force (Get-ChildItem '%2' -Recurse -File | "
                    "Where-Object {$_.Name -eq '%3'} | Select-Object -First 1 -ExpandProperty FullName) '%4'")
                .arg(zipPath, QDir::tempPath() + "/VelixInstaller_extract", binaryName, destPath);
    QProcess ps;
    ps.start("powershell.exe", args);
    ps.waitForFinished(30000);
#else
    // On Linux/macOS, use unzip.
    const QString extractDir = QDir::tempPath() + "/VelixInstaller_extract";
    QDir().mkpath(extractDir);
    {
        QProcess unzip;
        unzip.start("unzip", {"-o", zipPath, "-d", extractDir});
        unzip.waitForFinished(30000);
    }
    // Find the binary file in the extracted tree.
    QDir exDir(extractDir);
    const QFileInfoList all = exDir.entryInfoList(QDir::Files | QDir::Executable, QDir::NoSort);
    if (!all.isEmpty())
    {
        QFile::remove(destPath);
        QFile::copy(all.first().absoluteFilePath(), destPath);
    }
    else
    {
        // Fallback: search recursively by binary name.
        QDirIterator it(extractDir, {binaryName}, QDir::Files,
                        QDirIterator::Subdirectories);
        if (it.hasNext())
        {
            QFile::remove(destPath);
            QFile::copy(it.next(), destPath);
        }
    }
#endif

    return QFileInfo::exists(destPath) ? destPath : QString{};
}
} // namespace

UpdateWidget::UpdateWidget(AppUpdateChecker* checker, QWidget* parent)
    : QWidget(parent), m_checker(checker)
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    mainLayout->setSpacing(12);

    auto* titleLabel = new VelixText("Updates", this);
    titleLabel->setPointSize(14);
    titleLabel->setTextColor(Qt::white);
    mainLayout->addWidget(titleLabel, 0, Qt::AlignLeft);

    m_upToDatePanel = new QWidget(this);
    m_upToDatePanel->setAutoFillBackground(true);
    QPalette p = m_upToDatePanel->palette();
    p.setColor(QPalette::Window, QColor(26, 26, 26, 235));
    m_upToDatePanel->setPalette(p);
    {
        auto* l = new QVBoxLayout(m_upToDatePanel);
        l->setContentsMargins(14, 14, 14, 14);
        auto* lbl = new VelixText("Checking for updates\u2026", m_upToDatePanel);
        lbl->setPointSize(10);
        lbl->setTextColor(QColor(160, 160, 160));
        m_statusLabel = lbl;
        l->addWidget(lbl);
    }
    mainLayout->addWidget(m_upToDatePanel);

    m_updatePanel = new QWidget(this);
    m_updatePanel->setAutoFillBackground(true);
    QPalette p2 = m_updatePanel->palette();
    p2.setColor(QPalette::Window, QColor(26, 26, 26, 235));
    m_updatePanel->setPalette(p2);
    m_updatePanel->hide();
    {
        auto* panelLayout = new QVBoxLayout(m_updatePanel);
        panelLayout->setContentsMargins(14, 14, 14, 14);
        panelLayout->setSpacing(10);

        // Version row
        auto* versionRow = new QHBoxLayout();
        m_currentVersionLabel = new VelixText(
            QString("Current:  %1").arg(QString::fromUtf8(kInstallerVersion)), m_updatePanel);
        m_currentVersionLabel->setPointSize(10);
        m_currentVersionLabel->setTextColor(QColor(200, 200, 200));

        m_latestVersionLabel = new VelixText("Latest:  —", m_updatePanel);
        m_latestVersionLabel->setPointSize(10);
        m_latestVersionLabel->setTextColor(QColor(255, 140, 30));

        versionRow->addWidget(m_currentVersionLabel);
        versionRow->addStretch(1);
        versionRow->addWidget(m_latestVersionLabel);
        panelLayout->addLayout(versionRow);

        // Separator
        auto* sep = new QWidget(m_updatePanel);
        sep->setFixedHeight(1);
        sep->setStyleSheet("background: #2e2e2e;");
        panelLayout->addWidget(sep);

        // Changelog
        m_changelogEdit = new QTextEdit(m_updatePanel);
        m_changelogEdit->setReadOnly(true);
        m_changelogEdit->setFixedHeight(180);
        m_changelogEdit->setStyleSheet(
            "QTextEdit {"
            "  background: #1a1a1a; color: #d0d0d0; border: 1px solid #333;"
            "  border-radius: 6px; font-family: monospace; font-size: 9pt;"
            "  padding: 6px;"
            "}"
        );
        panelLayout->addWidget(m_changelogEdit);

        // Buttons
        auto* btnRow = new QHBoxLayout();
        btnRow->setSpacing(10);
        m_downloadButton = new FireButton("Download && Install", FireButton::Variant::Primary, m_updatePanel);
        m_downloadButton->setFixedHeight(34);
        m_skipButton = new FireButton("Skip this version", FireButton::Variant::Secondary, m_updatePanel);
        m_skipButton->setFixedHeight(34);
        btnRow->addWidget(m_downloadButton);
        btnRow->addWidget(m_skipButton);
        btnRow->addStretch(1);
        panelLayout->addLayout(btnRow);

        // Progress
        m_progressBar = new VelixProgressBar(m_updatePanel);
        m_progressBar->setRange(0, 100);
        m_progressBar->setValue(0);
        m_progressBar->hide();
        panelLayout->addWidget(m_progressBar);

        m_speedLabel = new VelixText(m_updatePanel);
        m_speedLabel->setPointSize(9);
        m_speedLabel->setTextColor(QColor(150, 150, 150));
        m_speedLabel->hide();
        panelLayout->addWidget(m_speedLabel);
    }
    mainLayout->addWidget(m_updatePanel);
    mainLayout->addStretch(1);

    // Connections
    connect(m_downloadButton, &QPushButton::clicked, this, &UpdateWidget::onDownloadAndInstall);
    connect(m_skipButton,     &QPushButton::clicked, this, &UpdateWidget::onSkipVersion);

    connect(m_checker, &AppUpdateChecker::downloadProgressChanged, this, &UpdateWidget::onDownloadProgress);
    connect(m_checker, &AppUpdateChecker::downloadDataReady,        this, &UpdateWidget::onDownloadDataReady);
    connect(m_checker, &AppUpdateChecker::downloadFinished,         this, &UpdateWidget::onDownloadFinished);
    connect(m_checker, &AppUpdateChecker::downloadError,            this, &UpdateWidget::onDownloadError);
}

void UpdateWidget::onUpdateAvailable(const QString& version, const QString& downloadUrl, const QString& changelog)
{
    m_latestVersion = version;
    m_downloadUrl   = downloadUrl;

    m_latestVersionLabel->setText(QString("Latest:  %1").arg(version));
    m_changelogEdit->setPlainText(changelog.isEmpty() ? "(No changelog provided)" : changelog);

    m_upToDatePanel->hide();
    m_updatePanel->show();
}

void UpdateWidget::onNoUpdate()
{
    m_statusLabel->setText(QString("You\u2019re up to date  (%1)").arg(QString::fromUtf8(kInstallerVersion)));
    m_upToDatePanel->show();
    m_updatePanel->hide();
}

void UpdateWidget::onCheckFailed()
{
    m_statusLabel->setText(QString("Could not check for updates  (%1)").arg(QString::fromUtf8(kInstallerVersion)));
    m_upToDatePanel->show();
    m_updatePanel->hide();
}

void UpdateWidget::onDownloadAndInstall()
{
    if (m_downloadUrl.isEmpty())
        return;

    m_downloadButton->setEnabled(false);
    m_skipButton->setEnabled(false);
    m_progressBar->show();
    m_speedLabel->show();
    m_progressBar->setValue(0);
    m_speedLabel->setText("Starting download\u2026");

    const QString tempZip = QDir::tempPath() + "/VelixInstaller_update.zip";
    m_downloadFile = new QFile(tempZip, this);
    if (!m_downloadFile->open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        ToastNotification::show("Failed to open temp file for download.", ToastType::Error, this);
        m_downloadButton->setEnabled(true);
        m_skipButton->setEnabled(true);
        return;
    }

    connect(m_checker, &AppUpdateChecker::downloadSpeedChanged, this, [this](double kbps)
    {
        m_speedLabel->setText(QString("%1 KB/s").arg(kbps, 0, 'f', 1));
    });

    m_checker->download(QUrl(m_downloadUrl));
}

void UpdateWidget::onSkipVersion()
{
    if (m_latestVersion.isEmpty())
        return;

    Config cfg;
    cfg.load();
    cfg.mutableConfig()["skipped_version"] = m_latestVersion.toStdString();
    cfg.save();

    m_updatePanel->hide();
    m_statusLabel->setText(QString("Skipped version %1").arg(m_latestVersion));
    m_upToDatePanel->show();

    ToastNotification::show(QString("Update %1 skipped.").arg(m_latestVersion), ToastType::Info, this);
}

void UpdateWidget::onDownloadProgress(qint64 received, qint64 total)
{
    if (total > 0)
        m_progressBar->setValue(static_cast<int>(received * 100 / total));
}

void UpdateWidget::onDownloadDataReady(const QByteArray& chunk)
{
    if (m_downloadFile && m_downloadFile->isOpen())
        m_downloadFile->write(chunk);
}

void UpdateWidget::onDownloadFinished()
{
    if (m_downloadFile)
    {
        m_downloadFile->close();
        m_downloadFile->deleteLater();
        m_downloadFile = nullptr;
    }

    m_speedLabel->setText("Download complete. Preparing update\u2026");
    applyUpdate();
}

void UpdateWidget::onDownloadError(const QString& error)
{
    if (m_downloadFile)
    {
        m_downloadFile->close();
        m_downloadFile->deleteLater();
        m_downloadFile = nullptr;
    }

    m_progressBar->hide();
    m_speedLabel->hide();
    m_downloadButton->setEnabled(true);
    m_skipButton->setEnabled(true);

    ToastNotification::show("Download failed: " + error, ToastType::Error, this);
}

void UpdateWidget::applyUpdate()
{
    const QString zipPath    = QDir::tempPath() + "/VelixInstaller_update.zip";
    const QString stagedPath = extractAndStage(zipPath);

    if (stagedPath.isEmpty())
    {
        ToastNotification::show("Failed to extract update archive.", ToastType::Error, this);
        m_downloadButton->setEnabled(true);
        m_skipButton->setEnabled(true);
        m_progressBar->hide();
        m_speedLabel->hide();
        return;
    }

    const QString currentBin = QCoreApplication::applicationFilePath();
    m_speedLabel->setText("Restarting\u2026");

#ifdef _WIN32
    // Write a .bat that waits, replaces, then launches.
    const QString batPath = QDir::tempPath() + "/velix_update.bat";
    {
        QFile bat(batPath);
        if (bat.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            QTextStream ts(&bat);
            ts << "@echo off\r\n";
            ts << "timeout /t 2 /nobreak > nul\r\n";
            ts << "copy /y \"" << stagedPath << "\" \"" << currentBin << "\"\r\n";
            ts << "del \"" << stagedPath << "\"\r\n";
            ts << "start \"\" \"" << currentBin << "\"\r\n";
        }
    }
    QProcess::startDetached("cmd.exe", {"/c", batPath});
#else
    // Write a shell script that sleeps, replaces, then execs.
    const QString shPath = QDir::tempPath() + "/velix_update.sh";
    {
        QFile sh(shPath);
        if (sh.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            QTextStream ts(&sh);
            ts << "#!/bin/sh\n";
            ts << "sleep 1\n";
            ts << "cp \"" << stagedPath << "\" \"" << currentBin << "\"\n";
            ts << "chmod +x \"" << currentBin << "\"\n";
            ts << "rm -f \"" << stagedPath << "\"\n";
            ts << "exec \"" << currentBin << "\"\n";
        }
        sh.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner);
    }
    QProcess::startDetached("sh", {shPath});
#endif

    QCoreApplication::quit();
}

void UpdateWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const QRectF bounds = rect().adjusted(1, 1, -1, -1);
    QPainterPath path;
    path.addRoundedRect(bounds, 12, 12);

    QLinearGradient gradient(bounds.topLeft(), bounds.bottomLeft());
    gradient.setColorAt(0.0, QColor(20, 20, 20, 240));
    gradient.setColorAt(1.0, QColor(12, 12, 12, 240));
    painter.fillPath(path, gradient);

    painter.setPen(QPen(QColor(62, 62, 62), 1));
    painter.drawPath(path);
}
