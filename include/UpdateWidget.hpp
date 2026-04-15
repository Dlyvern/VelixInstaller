#ifndef UPDATE_WIDGET_HPP
#define UPDATE_WIDGET_HPP

#include <QWidget>
#include <QVBoxLayout>
#include <QTextEdit>
#include <QFile>
#include <QPainter>
#include <QPainterPath>
#include <QPalette>

#include "AppUpdateChecker.hpp"
#include "widgets/VelixText.hpp"
#include "widgets/VelixProgressBar.hpp"
#include "FireButton.hpp"

class UpdateWidget : public QWidget
{
    Q_OBJECT
public:
    explicit UpdateWidget(AppUpdateChecker* checker, QWidget* parent = nullptr);
    ~UpdateWidget() override = default;

public slots:
    void onUpdateAvailable(const QString& version, const QString& downloadUrl, const QString& changelog);
    void onNoUpdate();
    void onCheckFailed();

protected:
    void paintEvent(QPaintEvent* event) override;

private slots:
    void onDownloadAndInstall();
    void onSkipVersion();
    void onDownloadProgress(qint64 received, qint64 total);
    void onDownloadDataReady(const QByteArray& chunk);
    void onDownloadFinished();
    void onDownloadError(const QString& error);

private:
    void applyUpdate();

    AppUpdateChecker* m_checker{nullptr};

    VelixText*        m_currentVersionLabel{nullptr};
    VelixText*        m_latestVersionLabel{nullptr};
    QTextEdit*        m_changelogEdit{nullptr};
    FireButton*       m_downloadButton{nullptr};
    FireButton*       m_skipButton{nullptr};
    VelixProgressBar* m_progressBar{nullptr};
    VelixText*        m_speedLabel{nullptr};
    VelixText*        m_statusLabel{nullptr};

    QWidget*          m_updatePanel{nullptr};  // shown when update is available
    QWidget*          m_upToDatePanel{nullptr}; // shown when no update

    QString           m_latestVersion;
    QString           m_downloadUrl;
    QFile*            m_downloadFile{nullptr};
};

#endif // UPDATE_WIDGET_HPP
