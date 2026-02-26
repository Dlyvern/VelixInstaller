#ifndef INSTALL_WIDGET_HPP
#define INSTALL_WIDGET_HPP

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>


#include "ReleaseChecker.hpp"
#include "VersionsWidget.hpp"
#include "Config.hpp"
#include "Installer.hpp"
#include "FileDownloader.hpp"
#include "widgets/VelixProgressBar.hpp"
#include "widgets/VelixText.hpp"

class InstallWidget : public QWidget
{
    Q_OBJECT
public:
    explicit InstallWidget(QWidget* parent = nullptr);

    ~InstallWidget() override = default;

private:
    ReleaseChecker* m_releaseChecker{nullptr};
    QVBoxLayout* m_mainLayout{nullptr};
    VersionsWidget* m_versionsWidget{nullptr};
    FileDownloader* m_fileDownloader{nullptr};
    Config m_config;
    Installer m_installer;
    VelixProgressBar* m_progressBar{nullptr};
    VelixText* m_speedLabel{nullptr};
    QString m_currentDownloadTag;
    QString m_currentInstallPath;
    bool m_downloadInProgress{false};

    void bindDownloadHandlers();
    QString chooseInstallDirectory(const QString& tagName);
    bool extractArchive(const QString& archivePath, const QString& destinationDir, QString& errorMessage);
    void upsertInstalledVersion(const QString& tagName, const QString& installPath);
    void applyCurrentVersionToWidgets();
    void checkForInstalledVersions();

private slots:
    void onNewVersion(const QString &tagName, const QString &downloadLink);

    void onChooseVersion(const QString& tagName);
    void onDownloadVersion(const QString& tagName, const QString& downloadLink);

signals:
    void installedVersionsChanged();
};

#endif //INSTALL_WIDGET_HPP
