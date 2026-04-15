#include "PluginManagerDialog.hpp"

#include <QTabWidget>
#include <QScrollArea>
#include <QGridLayout>
#include <QPainter>
#include <QPainterPath>
#include <QLinearGradient>
#include <QApplication>
#include <QScreen>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QMessageBox>

#include "FireButton.hpp"
#include "widgets/VelixText.hpp"

namespace
{

static const char* kPluginExt =
#if defined(Q_OS_WIN)
    ".dll";
#else
    ".so";
#endif

static const char* kManifestPlatformKey =
#if defined(Q_OS_WIN)
    "windows";
#else
    "linux";
#endif

static const QString kDialogStyle = R"(
QWidget#tabContent {
    background-color: transparent;
}
QTabWidget::pane {
    border: 1px solid #2e2e2e;
    border-radius: 6px;
    background-color: #1a1a1a;
}
QTabBar::tab {
    background-color: #1a1a1a;
    color: #888888;
    border: none;
    padding: 8px 18px;
    font-size: 12px;
}
QTabBar::tab:selected {
    color: #ff5c10;
    border-bottom: 2px solid #e04800;
}
QTabBar::tab:hover:!selected {
    color: #cccccc;
}
QScrollArea {
    border: none;
    background: transparent;
}
QScrollBar:vertical {
    background: #141414; width: 6px; border-radius: 3px;
}
QScrollBar::handle:vertical {
    background: #404040; border-radius: 3px; min-height: 20px;
}
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
QProgressBar {
    background-color: #1c1c1c;
    border: 1px solid #333333;
    border-radius: 4px;
    text-align: center;
    color: #aaaaaa;
    font-size: 11px;
    min-height: 18px;
}
QProgressBar::chunk {
    background-color: #e04800;
    border-radius: 3px;
}
)";

static QWidget* makeSeparator(QWidget* parent)
{
    auto* sep = new QWidget(parent);
    sep->setFixedHeight(1);
    sep->setStyleSheet("background-color: #2e2e2e;");
    return sep;
}

static QVector<PluginEntry> parseManifestJson(const QByteArray& data)
{
    QVector<PluginEntry> result;
    const QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject())
        return result;

    const QJsonArray plugins = doc.object().value("plugins").toArray();
    for (const QJsonValue& val : plugins)
    {
        const QJsonObject obj = val.toObject();
        const QString url = obj.value(kManifestPlatformKey).toString();

        result.append({
            obj.value("name").toString(),
            obj.value("version").toString(),
            obj.value("description").toString(),
            obj.value("category").toString(),
            url
        });
    }
    return result;
}

} // namespace

// =============================================================================

PluginManagerDialog::PluginManagerDialog(const QString& projectPath,
                                         const QString& projectName,
                                         QWidget* parent)
    : QDialog(parent)
    , m_projectPath(projectPath)
    , m_projectName(projectName)
{
    setWindowTitle("Plugin Manager");
    setFixedSize(680, 520);
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setStyleSheet(kDialogStyle);

    m_netManager = new QNetworkAccessManager(this);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(14, 14, 14, 14);
    root->setSpacing(10);

    // ── Title bar ────────────────────────────────────────────────────────────
    auto* titleBar = new QWidget(this);
    titleBar->setFixedHeight(42);

    auto* titleLayout = new QHBoxLayout(titleBar);
    titleLayout->setContentsMargins(6, 0, 0, 0);

    auto* titleLabel = new VelixText(
        QString("Plugins — %1").arg(projectName), titleBar);
    titleLabel->setPointSize(13);
    titleLabel->setTextColor(Qt::white);

    titleLayout->addWidget(titleLabel);
    titleLayout->addStretch(1);

    root->addWidget(titleBar);
    root->addWidget(makeSeparator(this));

    // ── Status / progress ────────────────────────────────────────────────────
    m_statusLabel = new QLabel("Fetching plugin list...", this);
    m_statusLabel->setStyleSheet("QLabel { color: #888888; font-size: 11px; }");

    m_progressBar = new QProgressBar(this);
    m_progressBar->setRange(0, 0);       // indeterminate while fetching
    m_progressBar->setFixedHeight(20);
    m_progressBar->hide();

    // ── Tabs ─────────────────────────────────────────────────────────────────
    auto* tabs = new QTabWidget(this);

    // Available tab
    auto* availableScroll = new QScrollArea();
    availableScroll->setWidgetResizable(true);
    auto* availableContainer = new QWidget();
    availableContainer->setObjectName("tabContent");
    m_availableLayout = new QVBoxLayout(availableContainer);
    m_availableLayout->setContentsMargins(8, 8, 8, 8);
    m_availableLayout->setSpacing(6);
    m_availableLayout->addStretch();
    availableScroll->setWidget(availableContainer);
    tabs->addTab(availableScroll, "Available");

    // Installed tab
    auto* installedScroll = new QScrollArea();
    installedScroll->setWidgetResizable(true);
    auto* installedContainer = new QWidget();
    installedContainer->setObjectName("tabContent");
    m_installedLayout = new QVBoxLayout(installedContainer);
    m_installedLayout->setContentsMargins(8, 8, 8, 8);
    m_installedLayout->setSpacing(6);
    m_installedLayout->addStretch();
    installedScroll->setWidget(installedContainer);
    tabs->addTab(installedScroll, "Installed");

    root->addWidget(tabs, 1);
    root->addWidget(m_statusLabel);
    root->addWidget(m_progressBar);

    // ── Close button ─────────────────────────────────────────────────────────
    root->addWidget(makeSeparator(this));

    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch(1);
    auto* closeBtn = new FireButton("Close", FireButton::Variant::Secondary, this);
    closeBtn->setFixedWidth(100);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    btnLayout->addWidget(closeBtn);
    root->addLayout(btnLayout);

    // ── Center on screen ─────────────────────────────────────────────────────
    if (const QScreen* screen = QApplication::primaryScreen())
    {
        const QRect sg = screen->availableGeometry();
        move(sg.center() - rect().center());
    }

    // ── Start loading ────────────────────────────────────────────────────────
    populateInstalled();
    fetchManifest();
}

// =============================================================================
// Manifest fetching
// =============================================================================

void PluginManagerDialog::fetchManifest()
{
    m_statusLabel->setText("Fetching plugin list...");
    m_progressBar->setRange(0, 0);
    m_progressBar->show();

    QNetworkRequest req(QUrl(QString::fromLatin1(kManifestUrl)));
    QNetworkReply* reply = m_netManager->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply]()
    {
        onManifestFetched(reply);
    });
}

void PluginManagerDialog::onManifestFetched(QNetworkReply* reply)
{
    reply->deleteLater();
    m_progressBar->hide();

    if (reply->error() != QNetworkReply::NoError)
    {
        m_statusLabel->setText("Failed to fetch plugins: " + reply->errorString());
        return;
    }

    m_availablePlugins = parseManifestJson(reply->readAll());

    if (m_availablePlugins.isEmpty())
    {
        m_statusLabel->setText("No plugins found in manifest.");
        return;
    }

    m_statusLabel->setText(QString("%1 plugin(s) available").arg(m_availablePlugins.size()));
    populateAvailable(m_availablePlugins);
}

// =============================================================================
// Populate cards
// =============================================================================

void PluginManagerDialog::populateAvailable(const QVector<PluginEntry>& entries)
{
    // Clear existing cards
    while (m_availableLayout->count() > 1)
    {
        auto* item = m_availableLayout->takeAt(0);
        if (item->widget())
            item->widget()->deleteLater();
        delete item;
    }

    const QStringList installed = installedPluginFiles();

    for (const PluginEntry& entry : entries)
    {
        const QString expectedFile = entry.name + kPluginExt;
        const bool isInstalled = installed.contains(expectedFile, Qt::CaseInsensitive);
        m_availableLayout->insertWidget(
            m_availableLayout->count() - 1,
            createAvailableCard(entry, isInstalled));
    }
}

void PluginManagerDialog::populateInstalled()
{
    while (m_installedLayout->count() > 1)
    {
        auto* item = m_installedLayout->takeAt(0);
        if (item->widget())
            item->widget()->deleteLater();
        delete item;
    }

    const QStringList files = installedPluginFiles();
    for (const QString& f : files)
    {
        m_installedLayout->insertWidget(
            m_installedLayout->count() - 1,
            createInstalledCard(f));
    }
}

// =============================================================================
// Card widgets
// =============================================================================

QWidget* PluginManagerDialog::createAvailableCard(const PluginEntry& entry, bool installed)
{
    auto* card = new QWidget();
    card->setStyleSheet(
        "QWidget#pluginCard { background-color: #222222; border: 1px solid #333333;"
        "  border-radius: 6px; }"
        "QWidget#pluginCard:hover { border-color: #555555; }"
    );
    card->setObjectName("pluginCard");

    auto* layout = new QHBoxLayout(card);
    layout->setContentsMargins(12, 10, 12, 10);
    layout->setSpacing(10);

    // Info column
    auto* infoLayout = new QVBoxLayout();
    infoLayout->setSpacing(2);

    auto* nameLabel = new QLabel(entry.name, card);
    nameLabel->setStyleSheet(
        "QLabel { color: #e0e0e0; font-size: 13px; font-weight: bold;"
        "  background: transparent; }");

    auto* descLabel = new QLabel(entry.description, card);
    descLabel->setWordWrap(true);
    descLabel->setStyleSheet(
        "QLabel { color: #888888; font-size: 11px; background: transparent; }");

    auto* metaLabel = new QLabel(
        QString("%1  •  %2").arg(entry.category, entry.version), card);
    metaLabel->setStyleSheet(
        "QLabel { color: #555555; font-size: 10px; background: transparent; }");

    infoLayout->addWidget(nameLabel);
    infoLayout->addWidget(descLabel);
    infoLayout->addWidget(metaLabel);

    layout->addLayout(infoLayout, 1);

    // Action button
    if (installed)
    {
        auto* badge = new QLabel("Installed", card);
        badge->setStyleSheet(
            "QLabel { color: #4caf50; font-size: 11px; font-weight: bold;"
            "  background: transparent; }");
        badge->setFixedWidth(80);
        badge->setAlignment(Qt::AlignCenter);
        layout->addWidget(badge);
    }
    else if (entry.downloadUrl.isEmpty())
    {
        auto* badge = new QLabel("No binary", card);
        badge->setStyleSheet(
            "QLabel { color: #666666; font-size: 11px;"
            "  background: transparent; }");
        badge->setFixedWidth(80);
        badge->setAlignment(Qt::AlignCenter);
        layout->addWidget(badge);
    }
    else
    {
        auto* installBtn = new FireButton("Install", FireButton::Variant::Primary, card);
        installBtn->setFixedWidth(80);
        connect(installBtn, &QPushButton::clicked, this, [this, entry, installBtn]()
        {
            installBtn->setEnabled(false);
            installBtn->setText("...");
            downloadPlugin(entry);
        });
        layout->addWidget(installBtn);
    }

    return card;
}

QWidget* PluginManagerDialog::createInstalledCard(const QString& filename)
{
    auto* card = new QWidget();
    card->setStyleSheet(
        "QWidget#pluginCard { background-color: #222222; border: 1px solid #333333;"
        "  border-radius: 6px; }");
    card->setObjectName("pluginCard");

    auto* layout = new QHBoxLayout(card);
    layout->setContentsMargins(12, 10, 12, 10);
    layout->setSpacing(10);

    const QString stem = QFileInfo(filename).completeBaseName();

    auto* nameLabel = new QLabel(stem, card);
    nameLabel->setStyleSheet(
        "QLabel { color: #e0e0e0; font-size: 13px; font-weight: bold;"
        "  background: transparent; }");

    auto* fileLabel = new QLabel(filename, card);
    fileLabel->setStyleSheet(
        "QLabel { color: #555555; font-size: 10px; background: transparent; }");

    auto* infoLayout = new QVBoxLayout();
    infoLayout->setSpacing(2);
    infoLayout->addWidget(nameLabel);
    infoLayout->addWidget(fileLabel);
    layout->addLayout(infoLayout, 1);

    auto* removeBtn = new FireButton("Remove", FireButton::Variant::Secondary, card);
    removeBtn->setFixedWidth(80);
    connect(removeBtn, &QPushButton::clicked, this, [this, filename]()
    {
        removePlugin(filename);
    });
    layout->addWidget(removeBtn);

    return card;
}

// =============================================================================
// Download / Remove
// =============================================================================

void PluginManagerDialog::downloadPlugin(const PluginEntry& entry)
{
    // Ensure Plugins/ directory exists
    QDir().mkpath(pluginsDir());

    m_statusLabel->setText(QString("Downloading %1...").arg(entry.name));
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->show();

    QNetworkRequest req(QUrl(entry.downloadUrl));
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                     QNetworkRequest::NoLessSafeRedirectPolicy);
    QNetworkReply* reply = m_netManager->get(req);

    connect(reply, &QNetworkReply::downloadProgress, this,
        [this](qint64 received, qint64 total)
        {
            if (total > 0)
                m_progressBar->setValue(static_cast<int>(received * 100 / total));
        });

    const QString pluginName = entry.name;
    connect(reply, &QNetworkReply::finished, this, [this, reply, pluginName]()
    {
        onDownloadFinished(reply, pluginName);
    });
}

void PluginManagerDialog::onDownloadFinished(QNetworkReply* reply, const QString& pluginName)
{
    reply->deleteLater();
    m_progressBar->hide();

    if (reply->error() != QNetworkReply::NoError)
    {
        m_statusLabel->setText(QString("Download failed: %1").arg(reply->errorString()));
        return;
    }

    const QByteArray data = reply->readAll();
    const QString destPath = QDir(pluginsDir()).filePath(pluginName + kPluginExt);
    QFile file(destPath);
    if (!file.open(QIODevice::WriteOnly))
    {
        m_statusLabel->setText(QString("Failed to write %1").arg(destPath));
        return;
    }
    file.write(data);
    file.close();

    // Make executable on Linux
#if !defined(Q_OS_WIN)
    file.setPermissions(file.permissions() | QFile::ExeOwner | QFile::ExeGroup | QFile::ExeOther);
#endif

    m_statusLabel->setText(QString("%1 installed successfully.").arg(pluginName));

    // Refresh both tabs
    populateInstalled();
    populateAvailable(m_availablePlugins);
}

void PluginManagerDialog::removePlugin(const QString& filename)
{
    const QString path = QDir(pluginsDir()).filePath(filename);
    if (QFile::remove(path))
    {
        m_statusLabel->setText(QString("Removed %1.").arg(filename));
        populateInstalled();
        populateAvailable(m_availablePlugins);
    }
    else
    {
        m_statusLabel->setText(QString("Failed to remove %1.").arg(filename));
    }
}

// =============================================================================
// Helpers
// =============================================================================

QString PluginManagerDialog::pluginsDir() const
{
    return QDir(m_projectPath).filePath("Plugins");
}

QStringList PluginManagerDialog::installedPluginFiles() const
{
    const QDir dir(pluginsDir());
    if (!dir.exists())
        return {};

    QStringList filters;
    filters << QString("*") + kPluginExt;
    return dir.entryList(filters, QDir::Files);
}

// =============================================================================
// Paint / drag
// =============================================================================

void PluginManagerDialog::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const QRectF bounds = rect().adjusted(1, 1, -1, -1);
    QPainterPath path;
    path.addRoundedRect(bounds, 10, 10);

    QLinearGradient bg(bounds.topLeft(), bounds.bottomLeft());
    bg.setColorAt(0.0, QColor(22, 22, 22, 252));
    bg.setColorAt(1.0, QColor(14, 14, 14, 252));
    painter.fillPath(path, bg);

    painter.setPen(QPen(QColor(60, 60, 60), 1));
    painter.drawPath(path);
}

void PluginManagerDialog::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && event->pos().y() < 56)
    {
        m_dragging = true;
        m_dragOffset = event->globalPosition().toPoint() - frameGeometry().topLeft();
    }
    QDialog::mousePressEvent(event);
}

void PluginManagerDialog::mouseMoveEvent(QMouseEvent* event)
{
    if (m_dragging)
        move(event->globalPosition().toPoint() - m_dragOffset);
    QDialog::mouseMoveEvent(event);
}
