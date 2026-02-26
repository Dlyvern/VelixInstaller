#include "ReleaseChecker.hpp"

namespace
{
QString currentPlatformToken()
{
#ifdef _WIN32
    return "windows";
#elif defined(__APPLE__)
    return "mac";
#else
    return "linux";
#endif
}
}

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
    const QUrl requestUrl = reply->request().url();
    if (!requestUrl.toString().contains("/repos/Dlyvern/Velix/releases"))
        return;

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
        const QString preferredToken = currentPlatformToken();
        QString matchedUrl;
        QString fallbackUrl;

        for (const QJsonValue& assetVal : assets) 
        {
            QJsonObject asset = assetVal.toObject();
            const QString name = asset["name"].toString().toLower();
            const QString url = asset["browser_download_url"].toString();

            if (!name.endsWith(".zip"))
                continue;

            if (fallbackUrl.isEmpty())
                fallbackUrl = url;

            if (name.contains(preferredToken))
            {
                matchedUrl = url;
                break;
            }
        }

        const QString selectedUrl = !matchedUrl.isEmpty() ? matchedUrl : fallbackUrl;
        if (!selectedUrl.isEmpty())
            emit newVersionFound(version, selectedUrl);
    }
}
