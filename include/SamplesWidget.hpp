#ifndef SAMPLES_WIDGET_HPP
#define SAMPLES_WIDGET_HPP

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QLabel>
#include <QLineEdit>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QProgressBar>
#include <QVector>
#include <QPainter>

struct SampleEntry
{
    QString name;
    QString description;
    QString category;       // "3D", "2D", "Template", etc.
    QString downloadUrl;    // URL to .zip archive
    QString size;           // human-readable, e.g. "45 MB"
    QString engineVersion; // minimum engine version
};

class SamplesWidget : public QWidget
{
    Q_OBJECT
public:
    explicit SamplesWidget(QWidget* parent = nullptr);
    ~SamplesWidget() override = default;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    void fetchManifest();
    void onManifestFetched(QNetworkReply* reply);
    void populateCards(const QVector<SampleEntry>& entries);
    QWidget* createSampleCard(const SampleEntry& entry);
    void downloadSample(const SampleEntry& entry);
    void onDownloadFinished(QNetworkReply* reply, const SampleEntry& entry);
    void filterCards(const QString& text);
    QString resolveProjectsRoot() const;

    static constexpr const char* kManifestUrl =
        "https://raw.githubusercontent.com/Dlyvern/EnginePlugins/main/samples_manifest.json";

    QNetworkAccessManager* m_netManager{nullptr};
    QVBoxLayout*  m_cardsLayout{nullptr};
    QLineEdit*    m_searchBar{nullptr};
    QLabel*       m_statusLabel{nullptr};
    QProgressBar* m_progressBar{nullptr};
    QVector<SampleEntry> m_samples;
    QVector<QWidget*>    m_cards;
};

#endif // SAMPLES_WIDGET_HPP
