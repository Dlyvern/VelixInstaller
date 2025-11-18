#include "ReleaseChecker.hpp"

ReleaseChecker::ReleaseChecker(QObject* parent) : QObject(parent)
{
    m_manager = new QNetworkAccessManager(this);

    connect(m_manager, &QNetworkAccessManager::finished,  this, &ReleaseChecker::onFinished);

    QUrl url("https://api.github.com/repos/Dlyvern/Velix/releases");
    QNetworkRequest request(url);

    request.setRawHeader("Accept", "application/vnd.github+json");
    request.setRawHeader("User-Agent", "VelixInstaller");  

    QNetworkReply *reply = m_manager->get(request);
}

void ReleaseChecker::download(const QUrl& url)
{
    QNetworkRequest request(url);
    QNetworkReply *reply = m_manager->get(request);

    m_timer.start();
    m_lastBytes = 0;

    connect(reply, &QNetworkReply::downloadProgress, this, [this](qint64 bytesReceived, qint64 bytesTotal)
    {
        emit downloadProgressChanged(bytesReceived, bytesTotal);

        qint64 elapsedMs = m_timer.elapsed();
        qint64 bytesDiff = bytesReceived - m_lastBytes;

        if (elapsedMs > 0)
        {
            double speed = (bytesDiff / 1024.0) / (elapsedMs / 1000.0);
            emit downloadSpeedChanged(speed);
        }

        m_timer.restart();
        m_lastBytes = bytesReceived;
    });

    connect(reply, &QIODevice::readyRead, this, [this, reply]
    {
        emit downloadDataReady(reply->readAll());
    });

    connect(reply, &QNetworkReply::finished, this, [this, reply]
    {
        if (reply->error() != QNetworkReply::NoError)
            emit downloadError(reply->errorString()); 
        else
            emit downloadFinished();

        reply->deleteLater();
    });
}

void ReleaseChecker::onFinished(QNetworkReply* reply)
{
    if (reply->error() != QNetworkReply::NoError) 
    {
        qWarning() << "Error:" << reply->errorString();
        return;
    }

    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);

    if (!doc.isArray())
    {
        qWarning() << "Invalid JSON";
        return;
    }

    QJsonArray releases = doc.array();

    for (const QJsonValue& val : releases)
    {
        QJsonObject obj = val.toObject();
        QString version = obj["tag_name"].toString();

        QJsonArray assets = obj["assets"].toArray();

        for (const QJsonValue& assetVal : assets) 
        {
            QJsonObject asset = assetVal.toObject();
            QString name = asset["name"].toString();
            QString url = asset["browser_download_url"].toString();

            emit newVersionFound(version, url);
        }
    }
}