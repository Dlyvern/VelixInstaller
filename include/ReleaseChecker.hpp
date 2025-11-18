#ifndef RELEASE_CHECKER_HPP
#define RELEASE_CHECKER_HPP

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDebug>
#include <QElapsedTimer>

class ReleaseChecker : public QObject
{
    Q_OBJECT
public:
    explicit ReleaseChecker(QObject* parent = nullptr);

    void download(const QUrl& url);

    ~ReleaseChecker() override = default;


private slots:
    void onFinished(QNetworkReply* reply);

signals:
    void newVersionFound(const QString& tagName,  const QString& downloadLink);
    void downloadProgressChanged(qint64 bytesReceived, qint64 bytesTotal);
    void downloadDataReady(const QByteArray& chunk);
    void downloadFinished();
    void downloadError(const QString& error);
    void downloadSpeedChanged(double kbPerSec);

private:
    QNetworkAccessManager* m_manager{nullptr};
    QElapsedTimer m_timer;
    qint64 m_lastBytes = 0;
};

#endif //RELEASE_CHECKER_HPP