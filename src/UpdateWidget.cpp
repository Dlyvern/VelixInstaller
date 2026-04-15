#include "UpdateWidget.hpp"
#include "AppVersion.hpp"
#include "Config.hpp"
#include "ToastNotification.hpp"

#include <QDir>
#include <QDirIterator>
#include <QProcess>
#include <QCoreApplication>
#include <QHBoxLayout>
#include <QPalette>
#include <QTextStream>
#include <QDebug>

#include "miniz/miniz.h"

namespace
{
void copyDir(const QString& src, const QString& dest)
{
    QDir().mkpath(dest);
    QDirIterator it(src, QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot,
                    QDirIterator::Subdirectories);
    while (it.hasNext())
    {
        it.next();
        const QString rel    = it.filePath().mid(src.length() + 1);
        const QString target = dest + "/" + rel;
        if (it.fileInfo().isDir())
            QDir().mkpath(target);
        else
        {
            QFile::remove(target);
            QFile::copy(it.filePath(), target);
        }
    }
}

QString extractAndStage(const QString& zipPath)
{
    const QString appPath    = QCoreApplication::applicationFilePath();
    const QFileInfo appInfo(appPath);
    const QString binaryName = appInfo.fileName();
    const QString installDir = appInfo.dir().absolutePath();
    const QString destPath   = appInfo.dir().filePath(binaryName + ".new");
    const QString extractDir = QDir::tempPath() + "/VelixInstaller_extract";

    QDir(extractDir).removeRecursively();
    QDir().mkpath(extractDir);

    qDebug() << "[UpdateExtract] zip:" << zipPath << "exists:" << QFileInfo::exists(zipPath)
             << "size:" << QFileInfo(zipPath).size();

    // Extract one zip layer into destDir. Returns false on error.
    auto extractZip = [](const QString& src, const QString& destDir) -> bool
    {
        mz_zip_archive zip{};
        if (!mz_zip_reader_init_file(&zip, src.toLocal8Bit().constData(), 0))
        {
            qWarning() << "[UpdateExtract] mz_zip_reader_init_file failed, error:" << static_cast<int>(zip.m_last_error);
            return false;
        }

        const mz_uint numFiles = mz_zip_reader_get_num_files(&zip);
        qDebug() << "[UpdateExtract] entries:" << numFiles << "-> dest:" << destDir;

        for (mz_uint i = 0; i < numFiles; ++i)
        {
            mz_zip_archive_file_stat stat{};
            if (!mz_zip_reader_file_stat(&zip, i, &stat))
            {
                qWarning() << "[UpdateExtract] file_stat failed at entry" << i << "error:" << static_cast<int>(zip.m_last_error);
                mz_zip_reader_end(&zip);
                return false;
            }

            const QString entryName = QString::fromUtf8(stat.m_filename);
            const QString entryDest = destDir + "/" + entryName;

            if (mz_zip_reader_is_file_a_directory(&zip, i))
            {
                QDir().mkpath(entryDest);
                continue;
            }

            QDir().mkpath(QFileInfo(entryDest).absolutePath());
            qDebug() << "[UpdateExtract] extracting:" << entryName;

            if (!mz_zip_reader_extract_to_file(&zip, i, entryDest.toLocal8Bit().constData(), 0))
            {
                qWarning() << "[UpdateExtract] extract_to_file failed:" << entryName << "error:" << static_cast<int>(zip.m_last_error);
                mz_zip_reader_end(&zip);
                return false;
            }
        }

        mz_zip_reader_end(&zip);
        return true;
    };

    if (!extractZip(zipPath, extractDir))
        return {};

    // If the archive contained a single nested zip, extract that too
    {
        QDirIterator it(extractDir, {"*.zip"}, QDir::Files, QDirIterator::Subdirectories);
        if (it.hasNext())
        {
            const QString innerZip   = it.next();
            const QString innerDir   = extractDir + "/inner";
            QDir().mkpath(innerDir);
            qDebug() << "[UpdateExtract] found nested zip:" << innerZip << "-> extracting into" << innerDir;
            if (!extractZip(innerZip, innerDir))
                return {};
            QFile::remove(innerZip);
        }
    }

    QString foundBinary;
    QDirIterator nameIt(extractDir, {binaryName}, QDir::Files,
                        QDirIterator::Subdirectories);
    if (nameIt.hasNext())
        foundBinary = nameIt.next();

    if (foundBinary.isEmpty())
    {
        QDirIterator exeIt(extractDir, QDir::Files, QDirIterator::Subdirectories);
        while (exeIt.hasNext())
        {
            exeIt.next();
            if (exeIt.fileInfo().isExecutable())
            {
                foundBinary = exeIt.filePath();
                break;
            }
        }
    }

    if (foundBinary.isEmpty())
        return {};

    QFile::remove(destPath);
    if (!QFile::copy(foundBinary, destPath))
        return {};

    QDirIterator resIt(extractDir, QDir::Dirs | QDir::NoDotAndDotDot,
                       QDirIterator::Subdirectories);
    while (resIt.hasNext())
    {
        resIt.next();
        if (resIt.fileName() == "resources")
        {
            copyDir(resIt.filePath(), installDir + "/resources");
            break;
        }
    }

    return QFileInfo::exists(destPath) ? destPath : QString{};
}
} // namespace

// ── Tab style ────────────────────────────────────────────────────────────────
static const char* kTabStyle =
    "QTabWidget::pane { border: none; background: transparent; }"
    "QTabBar::tab {"
    "  background: #2a2a2a; color: #aaa;"
    "  padding: 6px 18px; border-radius: 6px 6px 0 0;"
    "  margin-right: 2px; font-size: 10pt;"
    "}"
    "QTabBar::tab:selected { background: #ff6a00; color: #fff; }"
    "QTabBar::tab:hover:!selected { background: #3a3a3a; color: #ddd; }";

// ── Constructor ───────────────────────────────────────────────────────────────
UpdateWidget::UpdateWidget(AppUpdateChecker* checker, QWidget* parent)
    : QWidget(parent), m_checker(checker)
{
    m_stable.skipConfigKey   = "skipped_stable_version";
    m_unstable.skipConfigKey = "skipped_unstable_version";

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    mainLayout->setSpacing(12);

    auto* titleLabel = new VelixText("Updates", this);
    titleLabel->setPointSize(14);
    titleLabel->setTextColor(Qt::white);
    mainLayout->addWidget(titleLabel, 0, Qt::AlignLeft);

    m_tabWidget = new QTabWidget(this);
    m_tabWidget->setStyleSheet(kTabStyle);
    m_tabWidget->addTab(buildChannelPage(m_stable,   "Stable"),   "Stable");
    m_tabWidget->addTab(buildChannelPage(m_unstable, "Unstable"), "Unstable");
    mainLayout->addWidget(m_tabWidget);

    // Checker connections
    connect(m_checker, &AppUpdateChecker::stableUpdateAvailable,
            this, &UpdateWidget::onStableUpdateAvailable);
    connect(m_checker, &AppUpdateChecker::unstableUpdateAvailable,
            this, &UpdateWidget::onUnstableUpdateAvailable);
    connect(m_checker, &AppUpdateChecker::checkFailed,
            this, &UpdateWidget::onCheckFailed);

    // Download signals routed through m_activeChannel
    connect(m_checker, &AppUpdateChecker::downloadProgressChanged, this, [this](qint64 recv, qint64 total)
    {
        if (m_activeChannel && total > 0)
            m_activeChannel->progressBar->setValue(static_cast<int>(recv * 100 / total));
    });

    connect(m_checker, &AppUpdateChecker::downloadSpeedChanged, this, [this](double kbps)
    {
        if (m_activeChannel)
            m_activeChannel->speedLabel->setText(QString("%1 KB/s").arg(kbps, 0, 'f', 1));
    });

    connect(m_checker, &AppUpdateChecker::downloadDataReady, this, [this](const QByteArray& chunk)
    {
        if (m_activeChannel && m_activeChannel->downloadFile && m_activeChannel->downloadFile->isOpen())
            m_activeChannel->downloadFile->write(chunk);
    });

    connect(m_checker, &AppUpdateChecker::downloadFinished, this, [this]
    {
        if (!m_activeChannel) return;
        if (m_activeChannel->downloadFile)
        {
            m_activeChannel->downloadFile->close();
            m_activeChannel->downloadFile->deleteLater();
            m_activeChannel->downloadFile = nullptr;
        }
        m_activeChannel->speedLabel->setText("Installing update\u2026");
        if (m_downloadDialog)
            m_downloadDialog->onFinished();
        emit downloadEnded();
        applyUpdate();
    });

    connect(m_checker, &AppUpdateChecker::downloadError, this, [this](const QString& error)
    {
        if (!m_activeChannel) return;
        if (m_activeChannel->downloadFile)
        {
            m_activeChannel->downloadFile->close();
            m_activeChannel->downloadFile->deleteLater();
            m_activeChannel->downloadFile = nullptr;
        }
        m_activeChannel->progressBar->hide();
        m_activeChannel->speedLabel->hide();
        m_activeChannel->showProgressBtn->hide();
        m_activeChannel->downloadBtn->setEnabled(true);
        m_activeChannel->skipBtn->setEnabled(true);
        m_activeChannel = nullptr;
        if (m_downloadDialog)
        {
            m_downloadDialog->onError(error);
            m_downloadDialog = nullptr;
        }
        emit downloadEnded();
        ToastNotification::show("Download failed: " + error, ToastType::Error, this);
    });
}

// ── Build a channel page ──────────────────────────────────────────────────────
QWidget* UpdateWidget::buildChannelPage(Channel& ch, const QString& label)
{
    auto* page = new QWidget();

    auto* pageLayout = new QVBoxLayout(page);
    pageLayout->setContentsMargins(0, 10, 0, 0);
    pageLayout->setSpacing(8);

    // ── Up-to-date panel ──────────────────────────────────────────────────
    ch.upToDatePanel = new QWidget(page);
    ch.upToDatePanel->setAutoFillBackground(true);
    QPalette p = ch.upToDatePanel->palette();
    p.setColor(QPalette::Window, QColor(26, 26, 26, 235));
    ch.upToDatePanel->setPalette(p);
    {
        auto* l = new QVBoxLayout(ch.upToDatePanel);
        l->setContentsMargins(14, 14, 14, 14);
        ch.statusLabel = new VelixText(QString("Checking for %1 updates\u2026").arg(label), ch.upToDatePanel);
        ch.statusLabel->setPointSize(10);
        ch.statusLabel->setTextColor(QColor(160, 160, 160));
        l->addWidget(ch.statusLabel);
    }
    pageLayout->addWidget(ch.upToDatePanel);

    // ── Update-available panel ────────────────────────────────────────────
    ch.updatePanel = new QWidget(page);
    ch.updatePanel->setAutoFillBackground(true);
    QPalette p2 = ch.updatePanel->palette();
    p2.setColor(QPalette::Window, QColor(26, 26, 26, 235));
    ch.updatePanel->setPalette(p2);
    ch.updatePanel->hide();
    {
        auto* panelLayout = new QVBoxLayout(ch.updatePanel);
        panelLayout->setContentsMargins(14, 14, 14, 14);
        panelLayout->setSpacing(10);

        // Version row
        auto* vRow = new QHBoxLayout();
        auto* curLbl = new VelixText(
            QString("Current:  %1").arg(QString::fromUtf8(kInstallerVersion)), ch.updatePanel);
        curLbl->setPointSize(10);
        curLbl->setTextColor(QColor(200, 200, 200));

        ch.latestLabel = new VelixText("Latest:  \u2014", ch.updatePanel);
        ch.latestLabel->setPointSize(10);
        ch.latestLabel->setTextColor(QColor(255, 140, 30));

        vRow->addWidget(curLbl);
        vRow->addStretch(1);
        vRow->addWidget(ch.latestLabel);
        panelLayout->addLayout(vRow);

        auto* sep = new QWidget(ch.updatePanel);
        sep->setFixedHeight(1);
        sep->setStyleSheet("background: #2e2e2e;");
        panelLayout->addWidget(sep);

        ch.changelogEdit = new QTextEdit(ch.updatePanel);
        ch.changelogEdit->setReadOnly(true);
        ch.changelogEdit->setFixedHeight(160);
        ch.changelogEdit->setStyleSheet(
            "QTextEdit {"
            "  background: #1a1a1a; color: #d0d0d0; border: 1px solid #333;"
            "  border-radius: 6px; font-family: monospace; font-size: 9pt; padding: 6px;"
            "}");
        panelLayout->addWidget(ch.changelogEdit);

        auto* btnRow = new QHBoxLayout();
        btnRow->setSpacing(10);
        ch.downloadBtn = new FireButton("Download && Install", FireButton::Variant::Primary, ch.updatePanel);
        ch.downloadBtn->setFixedHeight(34);
        ch.skipBtn     = new FireButton("Skip this version", FireButton::Variant::Secondary, ch.updatePanel);
        ch.skipBtn->setFixedHeight(34);
        btnRow->addWidget(ch.downloadBtn);
        btnRow->addWidget(ch.skipBtn);
        btnRow->addStretch(1);
        panelLayout->addLayout(btnRow);

        ch.progressBar = new VelixProgressBar(ch.updatePanel);
        ch.progressBar->setRange(0, 100);
        ch.progressBar->setValue(0);
        ch.progressBar->hide();
        panelLayout->addWidget(ch.progressBar);

        ch.speedLabel = new VelixText(ch.updatePanel);
        ch.speedLabel->setPointSize(9);
        ch.speedLabel->setTextColor(QColor(150, 150, 150));
        ch.speedLabel->hide();
        panelLayout->addWidget(ch.speedLabel);

        ch.showProgressBtn = new FireButton("Show download progress\u2026", FireButton::Variant::Secondary, ch.updatePanel);
        ch.showProgressBtn->setFixedHeight(30);
        ch.showProgressBtn->hide();
        panelLayout->addWidget(ch.showProgressBtn);
    }
    pageLayout->addWidget(ch.updatePanel);
    pageLayout->addStretch(1);

    // Button connections (capture &ch by reference — both live as long as UpdateWidget)
    connect(ch.downloadBtn, &QPushButton::clicked, this, [this, &ch]{ startDownload(ch); });
    connect(ch.skipBtn,     &QPushButton::clicked, this, [this, &ch]{ skipVersion(ch); });

    return page;
}

// ── Slots ─────────────────────────────────────────────────────────────────────
void UpdateWidget::onStableUpdateAvailable(const QString& version, const QString& url, const QString& changelog)
{
    m_stable.version     = version;
    m_stable.downloadUrl = url;
    m_stable.latestLabel->setText(QString("Latest:  %1").arg(version));
    m_stable.changelogEdit->setPlainText(changelog.isEmpty() ? "(No changelog provided)" : changelog);
    m_stable.upToDatePanel->hide();
    m_stable.updatePanel->show();
}

void UpdateWidget::onUnstableUpdateAvailable(const QString& version, const QString& url, const QString& changelog)
{
    m_unstable.version     = version;
    m_unstable.downloadUrl = url;
    m_unstable.latestLabel->setText(QString("Latest:  %1").arg(version));
    m_unstable.changelogEdit->setPlainText(changelog.isEmpty() ? "(No changelog provided)" : changelog);
    m_unstable.upToDatePanel->hide();
    m_unstable.updatePanel->show();

    // Switch to Unstable tab to draw attention
    m_tabWidget->setCurrentIndex(1);
}

void UpdateWidget::onNoStableUpdate()
{
    m_stable.statusLabel->setText(
        QString("You\u2019re up to date  (%1)").arg(QString::fromUtf8(kInstallerVersion)));
    m_stable.upToDatePanel->show();
    m_stable.updatePanel->hide();
}

void UpdateWidget::onNoUnstableUpdate()
{
    m_unstable.statusLabel->setText("No pre-releases available.");
    m_unstable.upToDatePanel->show();
    m_unstable.updatePanel->hide();
}

void UpdateWidget::onCheckFailed()
{
    const QString msg = QString("Could not check for updates  (%1)").arg(QString::fromUtf8(kInstallerVersion));
    m_stable.statusLabel->setText(msg);
    m_unstable.statusLabel->setText(msg);
}

void UpdateWidget::startDownload(Channel& ch)
{
    if (ch.downloadUrl.isEmpty() || m_activeChannel)
        return;

    m_activeChannel = &ch;

    ch.downloadBtn->setEnabled(false);
    ch.skipBtn->setEnabled(false);
    ch.progressBar->setValue(0);
    ch.progressBar->show();
    ch.speedLabel->setText("Starting download\u2026");
    ch.speedLabel->show();

    const QString tempZip = QDir::tempPath() + "/VelixInstaller_update.zip";
    ch.downloadFile = new QFile(tempZip, this);
    if (!ch.downloadFile->open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        ToastNotification::show("Failed to open temp file.", ToastType::Error, this);
        ch.downloadBtn->setEnabled(true);
        ch.skipBtn->setEnabled(true);
        ch.progressBar->hide();
        ch.speedLabel->hide();
        m_activeChannel = nullptr;
        delete ch.downloadFile;
        ch.downloadFile = nullptr;
        return;
    }

    // ── Create download dialog (no parent so it's never behind main window) ─
    m_downloadDialog = new UpdateDownloadDialog(ch.version, ch.changelogEdit->toPlainText(), nullptr);
    m_downloadDialog->show();
    m_downloadDialog->raise();
    m_downloadDialog->activateWindow();

    // Forward progress to dialog
    connect(m_checker, &AppUpdateChecker::downloadProgressChanged,
            m_downloadDialog, &UpdateDownloadDialog::onProgress);
    connect(m_checker, &AppUpdateChecker::downloadSpeedChanged,
            m_downloadDialog, &UpdateDownloadDialog::onSpeed);

    // Emit signals for MainWidget status bar
    emit downloadStarted(ch.version);
    connect(m_checker, &AppUpdateChecker::downloadProgressChanged, this, [this](qint64 recv, qint64 total)
    {
        if (total > 0)
            emit downloadProgressChanged(static_cast<int>(recv * 100 / total));
    });

    // "Show progress" button reopens the dialog if minimized
    ch.showProgressBtn->show();
    connect(ch.showProgressBtn, &QPushButton::clicked, this, [this]{ showDownloadDialog(); });

    m_checker->download(QUrl(ch.downloadUrl));
}

void UpdateWidget::skipVersion(Channel& ch)
{
    if (ch.version.isEmpty())
        return;

    Config cfg;
    cfg.load();
    cfg.mutableConfig()[ch.skipConfigKey.toStdString()] = ch.version.toStdString();
    cfg.save();

    ch.updatePanel->hide();
    ch.statusLabel->setText(QString("Skipped version %1").arg(ch.version));
    ch.upToDatePanel->show();

    ToastNotification::show(QString("Update %1 skipped.").arg(ch.version), ToastType::Info, this);
}

void UpdateWidget::applyUpdate()
{
    const QString zipPath    = QDir::tempPath() + "/VelixInstaller_update.zip";
    const QString stagedPath = extractAndStage(zipPath);

    if (stagedPath.isEmpty())
    {
        const QString zipPath = QDir::tempPath() + "/VelixInstaller_update.zip";
        const bool zipExists  = QFileInfo::exists(zipPath);
        const QString detail  = zipExists
            ? "Extraction failed — check terminal output for details."
            : "Downloaded file is missing from temp directory.";

        ToastNotification::show("Update failed: " + detail, ToastType::Error, this);
        if (m_activeChannel)
        {
            m_activeChannel->speedLabel->setText(detail);
            m_activeChannel->downloadBtn->setEnabled(true);
            m_activeChannel->skipBtn->setEnabled(true);
            m_activeChannel->progressBar->hide();
        }
        m_activeChannel = nullptr;
        return;
    }

    if (m_activeChannel)
        m_activeChannel->speedLabel->setText("Restarting\u2026");

    const QString currentBin = QCoreApplication::applicationFilePath();

#ifdef _WIN32
    const QString batPath = QDir::tempPath() + "/velix_update.bat";
    {
        QFile bat(batPath);
        if (bat.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            QTextStream ts(&bat);
            ts << "@echo off\r\n"
               << "timeout /t 2 /nobreak > nul\r\n"
               << "copy /y \"" << stagedPath << "\" \"" << currentBin << "\"\r\n"
               << "del \"" << stagedPath << "\"\r\n"
               << "start \"\" \"" << currentBin << "\"\r\n";
        }
    }
    QProcess::startDetached("cmd.exe", {"/c", batPath});
#else
    const QString shPath = QDir::tempPath() + "/velix_update.sh";
    {
        QFile sh(shPath);
        if (sh.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            QTextStream ts(&sh);
            ts << "#!/bin/sh\n"
               << "sleep 1\n"
               << "cp \"" << stagedPath << "\" \"" << currentBin << "\"\n"
               << "chmod +x \"" << currentBin << "\"\n"
               << "rm -f \"" << stagedPath << "\"\n"
               << "exec \"" << currentBin << "\"\n";
        }
        sh.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner);
    }
    QProcess::startDetached("sh", {shPath});
#endif

    QCoreApplication::quit();
}

// ── showDownloadDialog ────────────────────────────────────────────────────────
void UpdateWidget::showDownloadDialog()
{
    if (!m_downloadDialog) return;
    m_downloadDialog->show();
    m_downloadDialog->raise();
    m_downloadDialog->activateWindow();
}

// ── Paint ─────────────────────────────────────────────────────────────────────
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
