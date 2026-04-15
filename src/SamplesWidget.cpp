#include "SamplesWidget.hpp"

#include <QPainterPath>
#include <QLinearGradient>
#include <QScrollArea>
#include <QDir>
#include <QFileInfo>
#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QMessageBox>
#include <QStandardPaths>

#include "FireButton.hpp"
#include "widgets/VelixText.hpp"
#include "Config.hpp"

namespace
{

static const QString kSearchStyle = R"(
QLineEdit {
    background-color: #1e1e1e;
    border: 1px solid #333333;
    border-radius: 6px;
    color: #d0d0d0;
    padding: 6px 12px;
    font-size: 12px;
}
QLineEdit:focus { border-color: #e04800; }
)";

static const QString kScrollStyle = R"(
QScrollArea { border: none; background: transparent; }
QScrollBar:vertical {
    background: #141414; width: 6px; border-radius: 3px;
}
QScrollBar::handle:vertical {
    background: #404040; border-radius: 3px; min-height: 20px;
}
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
)";

// Category colour mapping
static QColor categoryColor(const QString& cat)
{
    const QString lower = cat.toLower();
    if (lower == "3d")       return QColor(224, 72, 0);    // orange
    if (lower == "2d")       return QColor(58, 143, 214);  // blue
    if (lower == "template") return QColor(76, 175, 80);   // green
    if (lower == "demo")     return QColor(171, 71, 188);  // purple
    return QColor(150, 150, 150);
}

} // namespace

// =============================================================================

SamplesWidget::SamplesWidget(QWidget* parent) : QWidget(parent)
{
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet("SamplesWidget { background-color: transparent; }");

    m_netManager = new QNetworkAccessManager(this);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(6, 4, 6, 6);
    root->setSpacing(10);

    // ── Header ───────────────────────────────────────────────────────────────
    auto* headerLayout = new QHBoxLayout();
    headerLayout->setSpacing(12);

    auto* title = new VelixText("Sample Projects", this);
    title->setPointSize(12);
    title->setTextColor(Qt::white);
    headerLayout->addWidget(title);
    headerLayout->addStretch(1);

    root->addLayout(headerLayout);

    // ── Search bar ───────────────────────────────────────────────────────────
    m_searchBar = new QLineEdit(this);
    m_searchBar->setPlaceholderText("Search samples...");
    m_searchBar->setFixedHeight(34);
    m_searchBar->setStyleSheet(kSearchStyle);
    connect(m_searchBar, &QLineEdit::textChanged, this, &SamplesWidget::filterCards);
    root->addWidget(m_searchBar);

    // ── Status & progress ────────────────────────────────────────────────────
    m_statusLabel = new QLabel("Fetching sample projects...", this);
    m_statusLabel->setStyleSheet("QLabel { color: #888888; font-size: 11px; }");
    root->addWidget(m_statusLabel);

    m_progressBar = new QProgressBar(this);
    m_progressBar->setFixedHeight(18);
    m_progressBar->setStyleSheet(
        "QProgressBar { background-color: #1c1c1c; border: 1px solid #333;"
        "  border-radius: 4px; text-align: center; color: #aaa; font-size: 10px; }"
        "QProgressBar::chunk { background-color: #e04800; border-radius: 3px; }");
    m_progressBar->hide();
    root->addWidget(m_progressBar);

    // ── Scroll area with card grid ───────────────────────────────────────────
    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setStyleSheet(kScrollStyle);

    auto* container = new QWidget();
    container->setStyleSheet("background: transparent;");
    m_cardsLayout = new QVBoxLayout(container);
    m_cardsLayout->setContentsMargins(4, 4, 4, 4);
    m_cardsLayout->setSpacing(8);
    m_cardsLayout->addStretch();
    scroll->setWidget(container);

    root->addWidget(scroll, 1);

    // ── Load samples ─────────────────────────────────────────────────────────
    fetchManifest();
}

// =============================================================================
// Network
// =============================================================================

void SamplesWidget::fetchManifest()
{
    m_statusLabel->setText("Fetching sample projects...");

    // Try local fallback first
    const QString localPath = QDir(QCoreApplication::applicationDirPath())
                                  .filePath("resources/samples_manifest.json");
    QFile localFile(localPath);
    if (localFile.open(QIODevice::ReadOnly))
    {
        const QJsonDocument doc = QJsonDocument::fromJson(localFile.readAll());
        localFile.close();
        if (doc.isObject())
        {
            const QJsonArray arr = doc.object().value("samples").toArray();
            m_samples.clear();
            for (const QJsonValue& val : arr)
            {
                const QJsonObject obj = val.toObject();
                m_samples.append({
                    obj.value("name").toString(),
                    obj.value("description").toString(),
                    obj.value("category").toString(),
                    obj.value("download_url").toString(),
                    obj.value("size").toString(),
                    obj.value("engine_version").toString()
                });
            }
            if (!m_samples.isEmpty())
            {
                m_statusLabel->setText(
                    QString("%1 sample project(s) available").arg(m_samples.size()));
                populateCards(m_samples);
            }
        }
    }

    // Also fetch from network for updates
    QNetworkRequest req{QUrl{QString::fromLatin1(kManifestUrl)}};
    QNetworkReply* reply = m_netManager->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply]()
    {
        onManifestFetched(reply);
    });
}

void SamplesWidget::onManifestFetched(QNetworkReply* reply)
{
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError)
    {
        if (m_samples.isEmpty())
            m_statusLabel->setText("Could not fetch samples: " + reply->errorString());
        return;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    if (!doc.isObject())
        return;

    const QJsonArray arr = doc.object().value("samples").toArray();
    QVector<SampleEntry> entries;
    for (const QJsonValue& val : arr)
    {
        const QJsonObject obj = val.toObject();
        entries.append({
            obj.value("name").toString(),
            obj.value("description").toString(),
            obj.value("category").toString(),
            obj.value("download_url").toString(),
            obj.value("size").toString(),
            obj.value("engine_version").toString()
        });
    }

    if (!entries.isEmpty())
    {
        m_samples = entries;
        m_statusLabel->setText(
            QString("%1 sample project(s) available").arg(m_samples.size()));
        populateCards(m_samples);
    }
}

// =============================================================================
// Cards
// =============================================================================

void SamplesWidget::populateCards(const QVector<SampleEntry>& entries)
{
    // Clear existing
    for (auto* w : m_cards)
        w->deleteLater();
    m_cards.clear();

    while (m_cardsLayout->count() > 1)
    {
        auto* item = m_cardsLayout->takeAt(0);
        delete item;
    }

    for (const SampleEntry& entry : entries)
    {
        auto* card = createSampleCard(entry);
        m_cards.append(card);
        m_cardsLayout->insertWidget(m_cardsLayout->count() - 1, card);
    }
}

QWidget* SamplesWidget::createSampleCard(const SampleEntry& entry)
{
    auto* card = new QWidget();
    card->setObjectName("sampleCard");
    card->setStyleSheet(
        "QWidget#sampleCard {"
        "  background-color: #222222; border: 1px solid #333333;"
        "  border-radius: 8px;"
        "}"
        "QWidget#sampleCard:hover { border-color: #555555; }");
    card->setProperty("sampleName", entry.name);

    auto* layout = new QHBoxLayout(card);
    layout->setContentsMargins(16, 14, 16, 14);
    layout->setSpacing(14);

    // ── Left: colour bar ─────────────────────────────────────────────────────
    auto* colorBar = new QWidget(card);
    colorBar->setFixedSize(4, 60);
    const QColor catCol = categoryColor(entry.category);
    colorBar->setStyleSheet(
        QString("background-color: %1; border-radius: 2px;").arg(catCol.name()));
    layout->addWidget(colorBar);

    // ── Center: info ─────────────────────────────────────────────────────────
    auto* infoLayout = new QVBoxLayout();
    infoLayout->setSpacing(4);

    // Name row
    auto* nameRow = new QHBoxLayout();
    nameRow->setSpacing(8);

    auto* nameLabel = new QLabel(entry.name, card);
    nameLabel->setStyleSheet(
        "QLabel { color: #e0e0e0; font-size: 14px; font-weight: bold;"
        "  background: transparent; }");
    nameRow->addWidget(nameLabel);

    // Category badge
    auto* catBadge = new QLabel(entry.category.toUpper(), card);
    catBadge->setStyleSheet(
        QString("QLabel { color: %1; font-size: 9px; font-weight: bold;"
                "  letter-spacing: 0.5px; background: transparent;"
                "  padding: 2px 6px; border: 1px solid %1; border-radius: 3px; }")
            .arg(catCol.name()));
    catBadge->setFixedHeight(18);
    nameRow->addWidget(catBadge);
    nameRow->addStretch();
    infoLayout->addLayout(nameRow);

    // Description
    auto* descLabel = new QLabel(entry.description, card);
    descLabel->setWordWrap(true);
    descLabel->setStyleSheet(
        "QLabel { color: #888888; font-size: 11px; background: transparent;"
        "  line-height: 1.4; }");
    infoLayout->addWidget(descLabel);

    // Meta row
    auto* metaRow = new QHBoxLayout();
    metaRow->setSpacing(16);

    if (!entry.size.isEmpty())
    {
        auto* sizeLabel = new QLabel(entry.size, card);
        sizeLabel->setStyleSheet(
            "QLabel { color: #555555; font-size: 10px; background: transparent; }");
        metaRow->addWidget(sizeLabel);
    }
    if (!entry.engineVersion.isEmpty())
    {
        auto* verLabel = new QLabel("Velix " + entry.engineVersion + "+", card);
        verLabel->setStyleSheet(
            "QLabel { color: #555555; font-size: 10px; background: transparent; }");
        metaRow->addWidget(verLabel);
    }
    metaRow->addStretch();
    infoLayout->addLayout(metaRow);

    layout->addLayout(infoLayout, 1);

    // ── Right: download button ───────────────────────────────────────────────
    auto* btnLayout = new QVBoxLayout();
    btnLayout->setSpacing(6);

    if (!entry.downloadUrl.isEmpty())
    {
        auto* downloadBtn = new FireButton("Download", FireButton::Variant::Primary, card);
        downloadBtn->setFixedWidth(105);
        connect(downloadBtn, &QPushButton::clicked, this,
            [this, entry, downloadBtn]()
            {
                downloadBtn->setEnabled(false);
                downloadBtn->setText("...");
                downloadSample(entry);
            });
        btnLayout->addWidget(downloadBtn);
    }
    else
    {
        auto* badge = new QLabel("Coming soon", card);
        badge->setStyleSheet(
            "QLabel { color: #555555; font-size: 10px; background: transparent; }");
        badge->setAlignment(Qt::AlignCenter);
        badge->setFixedWidth(105);
        btnLayout->addWidget(badge);
    }
    btnLayout->addStretch();
    layout->addLayout(btnLayout);

    return card;
}

// =============================================================================
// Download
// =============================================================================

void SamplesWidget::downloadSample(const SampleEntry& entry)
{
    m_statusLabel->setText(QString("Downloading %1...").arg(entry.name));
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->show();

    QNetworkRequest req{QUrl{entry.downloadUrl}};
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                     QNetworkRequest::NoLessSafeRedirectPolicy);
    QNetworkReply* reply = m_netManager->get(req);

    connect(reply, &QNetworkReply::downloadProgress, this,
        [this](qint64 received, qint64 total)
        {
            if (total > 0)
                m_progressBar->setValue(static_cast<int>(received * 100 / total));
        });

    connect(reply, &QNetworkReply::finished, this,
        [this, reply, entry]() { onDownloadFinished(reply, entry); });
}

void SamplesWidget::onDownloadFinished(QNetworkReply* reply, const SampleEntry& entry)
{
    reply->deleteLater();
    m_progressBar->hide();

    if (reply->error() != QNetworkReply::NoError)
    {
        m_statusLabel->setText(QString("Download failed: %1").arg(reply->errorString()));
        return;
    }

    const QByteArray data = reply->readAll();
    const QString projectsRoot = resolveProjectsRoot();
    QDir().mkpath(projectsRoot);

    const QString zipPath = QDir(projectsRoot).filePath(entry.name + ".zip");
    QFile file(zipPath);
    if (!file.open(QIODevice::WriteOnly))
    {
        m_statusLabel->setText("Failed to write download to disk.");
        return;
    }
    file.write(data);
    file.close();

    m_statusLabel->setText(
        QString("%1 downloaded to %2. Extract and open from the Projects tab.")
            .arg(entry.name, projectsRoot));
}

// =============================================================================
// Filter
// =============================================================================

void SamplesWidget::filterCards(const QString& text)
{
    const QString lower = text.trimmed().toLower();
    for (auto* card : m_cards)
    {
        const QString name = card->property("sampleName").toString().toLower();
        card->setVisible(lower.isEmpty() || name.contains(lower));
    }
}

// =============================================================================
// Helpers
// =============================================================================

QString SamplesWidget::resolveProjectsRoot() const
{
    Config cfg;
    cfg.load();
    const auto& config = cfg.getConfig();

    if (config.contains("project_root") && config["project_root"].is_string())
    {
        const QString root = QString::fromStdString(config["project_root"].get<std::string>());
        if (!root.isEmpty())
            return root;
    }

#if defined(Q_OS_WIN)
    return QDir::home().filePath("Documents/ElixProjects");
#else
    return QDir::home().filePath("Documents/ElixProjects");
#endif
}

// =============================================================================
// Paint
// =============================================================================

void SamplesWidget::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const QRectF bounds = rect().adjusted(1, 1, -1, -1);
    QPainterPath path;
    path.addRoundedRect(bounds, 12, 12);

    QLinearGradient bg(bounds.topLeft(), bounds.bottomLeft());
    bg.setColorAt(0.0, QColor(20, 20, 20, 240));
    bg.setColorAt(1.0, QColor(14, 14, 14, 240));
    painter.fillPath(path, bg);

    painter.setPen(QPen(QColor(62, 62, 62), 1));
    painter.drawPath(path);
}
