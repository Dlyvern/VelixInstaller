#include "AppUpdateChecker.hpp"
#include "AppVersion.hpp"
#include "Config.hpp"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

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

// Parse "v1.2.3" or "1.2.3" → {major, minor, patch}. Returns {-1,-1,-1} on failure.
std::tuple<int,int,int> parseSemVer(const QString& tag)
{
    QString s = tag;
    if (s.startsWith('v') || s.startsWith('V'))
        s = s.mid(1);

    const QStringList parts = s.split('.');
    if (parts.size() < 3)
        return {-1, -1, -1};

    bool ok1, ok2, ok3;
    int major = parts[0].toInt(&ok1);
    int minor = parts[1].toInt(&ok2);
    int patch = parts[2].toInt(&ok3);

    if (!ok1 || !ok2 || !ok3)
        return {-1, -1, -1};

    return {major, minor, patch};
}

bool isNewer(const QString& latest, const QString& current)
{
    auto [lMaj, lMin, lPat] = parseSemVer(latest);
    auto [cMaj, cMin, cPat] = parseSemVer(current);

    if (lMaj < 0 || cMaj < 0)
        return false;

    if (lMaj != cMaj) return lMaj > cMaj;
    if (lMin != cMin) return lMin > cMin;
    return lPat > cPat;
}
} // namespace

AppUpdateChecker::AppUpdateChecker(QObject* parent) : QObject(parent)
{
    m_manager = new QNetworkAccessManager(this);
    connect(m_manager, &QNetworkAccessManager::finished, this, &AppUpdateChecker::onFinished);

    QUrl url("https://api.github.com/repos/Dlyvern/VelixInstaller/releases/latest");
    QNetworkRequest request(url);
    request.setRawHeader("Accept", "application/vnd.github+json");
    request.setRawHeader("User-Agent", "VelixInstaller");
    m_manager->get(request);
}

void AppUpdateChecker::download(const QUrl& url)
{
    QNetworkRequest request(url);
    QNetworkReply* reply = m_manager->get(request);

    m_timer.start();
    m_lastBytes = 0;

    connect(reply, &QNetworkReply::downloadProgress, this, [this](qint64 received, qint64 total)
    {
        emit downloadProgressChanged(received, total);

        const qint64 elapsed = m_timer.elapsed();
        const qint64 diff    = received - m_lastBytes;
        if (elapsed > 0)
            emit downloadSpeedChanged((diff / 1024.0) / (elapsed / 1000.0));

        m_timer.restart();
        m_lastBytes = received;
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

void AppUpdateChecker::onFinished(QNetworkReply* reply)
{
    const QString urlStr = reply->request().url().toString();
    if (!urlStr.contains("/repos/Dlyvern/VelixInstaller/releases/latest"))
        return;

    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError)
    {
        emit checkFailed();
        return;
    }

    const QByteArray data = reply->readAll();
    const QJsonDocument doc = QJsonDocument::fromJson(data);

    if (!doc.isObject())
    {
        emit checkFailed();
        return;
    }

    const QJsonObject obj = doc.object();
    const QString latestTag = obj["tag_name"].toString();
    const QString changelog  = obj["body"].toString();

    // Check skipped version in config
    Config cfg;
    cfg.load();
    const auto& json = cfg.getConfig();
    if (json.contains("skipped_version") && json["skipped_version"].is_string())
    {
        const QString skipped = QString::fromStdString(json["skipped_version"].get<std::string>());
        if (skipped == latestTag)
        {
            emit noUpdateAvailable();
            return;
        }
    }

    if (!isNewer(latestTag, QString::fromUtf8(kInstallerVersion)))
    {
        emit noUpdateAvailable();
        return;
    }

    // Find download URL for this platform
    const QString preferred = currentPlatformToken();
    QString matchedUrl;
    QString fallbackUrl;

    for (const QJsonValue& assetVal : obj["assets"].toArray())
    {
        const QJsonObject asset = assetVal.toObject();
        const QString name = asset["name"].toString().toLower();
        const QString url  = asset["browser_download_url"].toString();

        if (!name.endsWith(".zip"))
            continue;

        if (fallbackUrl.isEmpty())
            fallbackUrl = url;

        if (name.contains(preferred))
        {
            matchedUrl = url;
            break;
        }
    }

    const QString selectedUrl = !matchedUrl.isEmpty() ? matchedUrl : fallbackUrl;
    if (selectedUrl.isEmpty())
    {
        emit checkFailed();
        return;
    }

    emit updateAvailable(latestTag, selectedUrl, changelog);
}
