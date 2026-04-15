#ifndef APP_UPDATE_CHECKER_HPP
#define APP_UPDATE_CHECKER_HPP

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QElapsedTimer>

class AppUpdateChecker : public QObject
{
    Q_OBJECT
public:
    explicit AppUpdateChecker(QObject* parent = nullptr);
    ~AppUpdateChecker() override = default;

    void download(const QUrl& url);

signals:
    void stableUpdateAvailable(const QString& version, const QString& downloadUrl, const QString& changelog);
    void unstableUpdateAvailable(const QString& version, const QString& downloadUrl, const QString& changelog);
    void noStableUpdate();
    void noUnstableUpdate();
    void checkFailed();

    void downloadProgressChanged(qint64 bytesReceived, qint64 bytesTotal);
    void downloadSpeedChanged(double kbPerSec);
    void downloadDataReady(const QByteArray& chunk);
    void downloadFinished();
    void downloadError(const QString& error);

private slots:
    void onFinished(QNetworkReply* reply);

private:
    QNetworkAccessManager* m_manager{nullptr};
    QElapsedTimer          m_timer;
    qint64                 m_lastBytes{0};
};

#endif // APP_UPDATE_CHECKER_HPP
