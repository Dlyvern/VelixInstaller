#ifndef PLUGIN_MANAGER_DIALOG_HPP
#define PLUGIN_MANAGER_DIALOG_HPP

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QLabel>
#include <QPushButton>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QProgressBar>
#include <QMouseEvent>
#include <QVector>

class FireButton;
class VelixText;

struct PluginEntry
{
    QString name;
    QString version;
    QString description;
    QString category;
    QString downloadUrl;   // platform-resolved
};

class PluginManagerDialog : public QDialog
{
    Q_OBJECT
public:
    explicit PluginManagerDialog(const QString& projectPath,
                                 const QString& projectName,
                                 QWidget* parent = nullptr);

    // For use in ProjectSettingsDialog — returns names of selected plugins
    static QVector<PluginEntry> fetchManifestBlocking();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;

private:
    void fetchManifest();
    void onManifestFetched(QNetworkReply* reply);
    void populateAvailable(const QVector<PluginEntry>& entries);
    void populateInstalled();
    void downloadPlugin(const PluginEntry& entry);
    void onDownloadFinished(QNetworkReply* reply, const QString& pluginName);
    void removePlugin(const QString& filename);
    QWidget* createAvailableCard(const PluginEntry& entry, bool installed);
    QWidget* createInstalledCard(const QString& filename);
    QStringList installedPluginFiles() const;
    QString pluginsDir() const;

    static constexpr const char* kManifestUrl =
        "https://raw.githubusercontent.com/Dlyvern/EnginePlugins/main/manifest.json";

    QString m_projectPath;
    QString m_projectName;

    QNetworkAccessManager* m_netManager{nullptr};
    QVBoxLayout*  m_availableLayout{nullptr};
    QVBoxLayout*  m_installedLayout{nullptr};
    QLabel*       m_statusLabel{nullptr};
    QProgressBar* m_progressBar{nullptr};

    QVector<PluginEntry> m_availablePlugins;

    QPoint m_dragOffset;
    bool   m_dragging{false};
};

#endif // PLUGIN_MANAGER_DIALOG_HPP
