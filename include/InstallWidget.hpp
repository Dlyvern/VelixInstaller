#ifndef INSTALL_WIDGET_HPP
#define INSTALL_WIDGET_HPP

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QProgressBar>
#include <QLabel>


#include "ReleaseChecker.hpp"
#include "VersionsWidget.hpp"
#include "Config.hpp"
#include "Installer.hpp"
#include "FileDownloader.hpp"
#include "Installer.hpp"

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
    QProgressBar* m_progressBar{nullptr};
    QLabel* m_speedLabel{nullptr};

    void checkForInstalledVersions();

private slots:
    void onNewVersion(const QString &tagName, const QString &downloadLink);

    void onLaunchVersion(const QString& tagName);
    void onDownloadVersion(const QString& tagName, const QString& downloadLink);
};

#endif //INSTALL_WIDGET_HPP