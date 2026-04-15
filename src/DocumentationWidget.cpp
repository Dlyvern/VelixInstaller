#include "DocumentationWidget.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QScrollBar>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QListWidgetItem>
#include <QPainter>
#include <QPainterPath>
#include <QTextDocument>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QStandardPaths>
#include <QNetworkReply>
#include <QDesktopServices>
#include <QUrl>

namespace
{
// Minimal Markdown-to-HTML converter for QTextBrowser.
// Handles: headings, bold, italic, inline code, fenced code blocks, tables,
// horizontal rules, bullet lists, and paragraphs. Good enough for docs.
QString markdownToHtml(const QString& md)
{
    QStringList lines = md.split('\n');
    QString html;
    html.reserve(md.size() * 2);

    bool inCode   = false;
    bool inTable  = false;
    bool inList   = false;
    int  listDepth = 0;

    auto closeList = [&]() {
        if (inList) { html += "</ul>\n"; inList = false; }
    };
    auto closeTable = [&]() {
        if (inTable) { html += "</table>\n"; inTable = false; }
    };

    // Inline formatting: bold, italic, inline code
    auto inlineFormat = [](QString line) -> QString
    {
        // Inline code first (protect from further substitution)
        QStringList codeParts = line.split('`');
        QString result;
        for (int i = 0; i < codeParts.size(); ++i)
        {
            if (i % 2 == 0)
            {
                // Normal text — apply bold/italic
                QString part = codeParts[i];
                part.replace(QRegularExpression("\\*\\*(.+?)\\*\\*"), "<b>\\1</b>");
                part.replace(QRegularExpression("\\*(.+?)\\*"),       "<i>\\1</i>");
                result += part;
            }
            else
            {
                result += "<code>" + codeParts[i].toHtmlEscaped() + "</code>";
            }
        }
        return result;
    };

    for (int i = 0; i < lines.size(); ++i)
    {
        const QString& raw = lines[i];
        QString line = raw;

        // Fenced code block
        if (line.startsWith("```"))
        {
            if (!inCode)
            {
                closeList();
                closeTable();
                inCode = true;
                html += "<pre><code>";
            }
            else
            {
                inCode = false;
                html += "</code></pre>\n";
            }
            continue;
        }
        if (inCode)
        {
            html += line.toHtmlEscaped() + "\n";
            continue;
        }

        // Horizontal rule
        if (line.startsWith("---") && line.trimmed().replace("-","").isEmpty())
        {
            closeList();
            closeTable();
            html += "<hr/>\n";
            continue;
        }

        // Heading
        if (line.startsWith('#'))
        {
            closeList();
            closeTable();
            int level = 0;
            while (level < line.size() && line[level] == '#') ++level;
            level = qMin(level, 6);
            QString text = line.mid(level).trimmed();
            html += QString("<h%1>%2</h%1>\n").arg(level).arg(inlineFormat(text));
            continue;
        }

        // Table row
        if (line.trimmed().startsWith('|') && line.trimmed().endsWith('|'))
        {
            // Skip separator rows (|---|---|)
            QString stripped = line.trimmed();
            stripped.remove('|'); stripped.remove('-'); stripped.remove(':');
            if (stripped.trimmed().isEmpty())
                continue;

            if (!inTable)
            {
                closeList();
                inTable = true;
                html += "<table>\n";
            }

            QStringList cells = line.trimmed().split('|');
            // first and last elements are empty (before first | and after last |)
            html += "<tr>";
            for (int c = 1; c < cells.size() - 1; ++c)
                html += "<td>" + inlineFormat(cells[c].trimmed()) + "</td>";
            html += "</tr>\n";
            continue;
        }

        closeTable();

        // Bullet list
        if (line.startsWith("- ") || line.startsWith("* "))
        {
            if (!inList) { html += "<ul>\n"; inList = true; }
            html += "<li>" + inlineFormat(line.mid(2).trimmed()) + "</li>\n";
            continue;
        }

        closeList();

        // Empty line = paragraph break
        if (line.trimmed().isEmpty())
        {
            html += "<br/>\n";
            continue;
        }

        // Normal paragraph line
        html += "<p>" + inlineFormat(line.trimmed()) + "</p>\n";
    }

    if (inCode)  html += "</code></pre>\n";
    if (inList)  html += "</ul>\n";
    if (inTable) html += "</table>\n";

    return html;
}

static const QString kNavStyle = R"(
QListWidget {
    background-color: #161616;
    border: none;
    border-right: 1px solid #2a2a2a;
    outline: none;
    padding: 4px 0;
}
QListWidget::item {
    color: #aaaaaa;
    padding: 7px 16px;
    border-radius: 0;
    font-size: 12px;
}
QListWidget::item:selected {
    background-color: #2a2a2a;
    color: #ff5c10;
    border-left: 3px solid #e04800;
}
QListWidget::item:hover:!selected {
    background-color: #1e1e1e;
    color: #cccccc;
}
)";

static const QString kSearchStyle = R"(
QLineEdit {
    background-color: #1a1a1a;
    border: 1px solid #333333;
    border-radius: 6px;
    color: #d0d0d0;
    padding: 4px 10px;
    font-size: 12px;
}
QLineEdit:focus { border-color: #e04800; }
)";

static const QString kNavBtnStyle = R"(
QPushButton {
    background-color: #1e1e1e;
    border: 1px solid #333333;
    border-radius: 5px;
    color: #888888;
    font-size: 14px;
    padding: 2px 10px;
    min-width: 28px;
    min-height: 26px;
}
QPushButton:hover  { background-color: #282828; color: #cccccc; }
QPushButton:pressed { background-color: #e04800; color: #ffffff; }
QPushButton:disabled { color: #444444; border-color: #252525; }
)";

// CSS injected into the QTextDocument for syntax highlighting
static const QString kDocCss = R"(
body  { background-color: #181818; color: #d0d0d0;
        font-family: 'Segoe UI', 'Ubuntu', sans-serif; font-size: 13px; margin: 20px; }
h1    { color: #ff5c10; font-size: 20px; border-bottom: 1px solid #333; padding-bottom: 6px; }
h2    { color: #e06030; font-size: 16px; margin-top: 20px; }
h3    { color: #cc8050; font-size: 14px; }
code  { background-color: #222222; color: #f0a070;
        font-family: 'Consolas', 'Courier New', monospace; font-size: 12px;
        padding: 1px 4px; border-radius: 3px; }
pre   { background-color: #1c1c1c; border: 1px solid #2e2e2e; border-radius: 5px;
        padding: 12px; margin: 8px 0; }
pre code { background-color: transparent; color: #d4d4d4; padding: 0; }
table { border-collapse: collapse; width: 100%; margin: 8px 0; }
td, th { border: 1px solid #2e2e2e; padding: 6px 10px; }
th    { background-color: #222222; color: #cccccc; }
tr:nth-child(even) { background-color: #1e1e1e; }
hr    { border: none; border-top: 1px solid #2e2e2e; margin: 14px 0; }
a     { color: #e04800; }
ul    { padding-left: 20px; }
li    { margin: 3px 0; }
p     { margin: 4px 0; line-height: 1.6; }
)";
} // namespace

DocumentationWidget::DocumentationWidget(QWidget* parent) : QWidget(parent)
{
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet("DocumentationWidget { background-color: #181818; }");

    // ── Pages definition ──────────────────────────────────────────────────────
    m_pages = {
        {"INTRODUCTION",         ""},
        {"Getting Started",      "getting-started.md"},
        {"",                     ""},
        {"SCRIPTING & SDK",      ""},
        {"SDK Overview",         "sdk.md"},
        {"Writing Scripts",      "scripting.md"},
        {"",                     ""},
        {"EXTENDING VELIX",      ""},
        {"Custom Components",    "components.md"},
        {"Writing Plugins",      "plugins.md"},
    };

    // ── Top bar ───────────────────────────────────────────────────────────────
    m_backBtn = new QPushButton("‹", this);
    m_fwdBtn  = new QPushButton("›", this);
    m_backBtn->setStyleSheet(kNavBtnStyle);
    m_fwdBtn->setStyleSheet(kNavBtnStyle);
    m_backBtn->setEnabled(false);
    m_fwdBtn->setEnabled(false);
    m_backBtn->setToolTip("Back");
    m_fwdBtn->setToolTip("Forward");

    m_bookmarkBtn = new QPushButton("☆", this);
    m_bookmarkBtn->setStyleSheet(kNavBtnStyle);
    m_bookmarkBtn->setToolTip("Bookmark this page");

    m_searchBar = new QLineEdit(this);
    m_searchBar->setPlaceholderText("Search documentation...");
    m_searchBar->setStyleSheet(kSearchStyle);

    auto* topBar = new QHBoxLayout();
    topBar->setSpacing(6);
    topBar->setContentsMargins(8, 6, 8, 6);
    topBar->addWidget(m_backBtn);
    topBar->addWidget(m_fwdBtn);
    topBar->addWidget(m_bookmarkBtn);
    topBar->addWidget(m_searchBar);

    // ── Nav sidebar ───────────────────────────────────────────────────────────
    m_nav = new QListWidget(this);
    m_nav->setStyleSheet(kNavStyle);
    m_nav->setFixedWidth(200);
    m_nav->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    loadBookmarks();
    buildNav();

    // ── Content browser ───────────────────────────────────────────────────────
    m_browser = new QTextBrowser(this);
    m_browser->setOpenExternalLinks(true);
    m_browser->setOpenLinks(false);            // we handle internal nav ourselves
    m_browser->document()->setDefaultStyleSheet(kDocCss);
    m_browser->setStyleSheet(
        "QTextBrowser { background-color: #181818; border: none; color: #d0d0d0; }"
        "QScrollBar:vertical { background: #141414; width: 7px; border-radius: 3px; }"
        "QScrollBar::handle:vertical { background: #404040; border-radius: 3px; min-height: 20px; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }"
    );

    // ── News feed panel ───────────────────────────────────────────────────────
    buildNewsFeed();

    // ── Splitter ──────────────────────────────────────────────────────────────
    auto* splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setHandleWidth(1);
    splitter->setStyleSheet("QSplitter::handle { background-color: #2a2a2a; }");
    splitter->addWidget(m_nav);
    splitter->addWidget(m_browser);
    splitter->addWidget(m_newsPanel);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setStretchFactor(2, 0);

    // ── Root layout ───────────────────────────────────────────────────────────
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);
    root->addLayout(topBar);

    auto* divider = new QWidget(this);
    divider->setFixedHeight(1);
    divider->setStyleSheet("background-color: #2a2a2a;");
    root->addWidget(divider);
    root->addWidget(splitter, 1);

    // ── Connections ───────────────────────────────────────────────────────────
    connect(m_nav,         &QListWidget::itemClicked,  this, &DocumentationWidget::onNavItemClicked);
    connect(m_searchBar,   &QLineEdit::textChanged,    this, &DocumentationWidget::onSearchChanged);
    connect(m_backBtn,     &QPushButton::clicked,      this, &DocumentationWidget::onBackClicked);
    connect(m_fwdBtn,      &QPushButton::clicked,      this, &DocumentationWidget::onForwardClicked);
    connect(m_bookmarkBtn, &QPushButton::clicked,      this, &DocumentationWidget::toggleBookmark);

    connect(m_browser, &QTextBrowser::backwardAvailable, m_backBtn, &QPushButton::setEnabled);
    connect(m_browser, &QTextBrowser::forwardAvailable,  m_fwdBtn,  &QPushButton::setEnabled);

    // Load first real page
    loadPage("getting-started.md");
    if (m_nav->count() > 1)
        m_nav->setCurrentRow(1);

    // Load news feed
    loadNewsFromFile();
    loadNewsFromNetwork();
}

void DocumentationWidget::buildNav()
{
    m_nav->clear();

    // ── Bookmarks section (if any) ───────────────────────────────────────────
    if (!m_bookmarks.isEmpty())
    {
        auto* header = new QListWidgetItem("BOOKMARKS", m_nav);
        header->setFlags(Qt::NoItemFlags);
        header->setForeground(QColor(224, 72, 0));
        QFont hf = header->font();
        hf.setPointSize(9);
        hf.setBold(true);
        hf.setLetterSpacing(QFont::AbsoluteSpacing, 1.0);
        header->setFont(hf);
        header->setSizeHint({0, 28});

        for (const QString& bm : m_bookmarks)
        {
            // Find the page title
            QString title = bm;
            for (const DocPage& p : m_pages)
            {
                if (p.filename == bm)
                {
                    title = p.title;
                    break;
                }
            }
            auto* item = new QListWidgetItem("  " + title, m_nav);
            item->setData(Qt::UserRole, bm);
            item->setForeground(QColor(224, 150, 90));
        }

        // Spacer after bookmarks
        auto* spacer = new QListWidgetItem("", m_nav);
        spacer->setSizeHint({0, 8});
        spacer->setFlags(Qt::NoItemFlags);
    }

    // ── Regular pages ────────────────────────────────────────────────────────
    for (const DocPage& page : m_pages)
    {
        if (page.filename.isEmpty() && page.title.isEmpty())
        {
            auto* item = new QListWidgetItem("", m_nav);
            item->setSizeHint({0, 8});
            item->setFlags(Qt::NoItemFlags);
            continue;
        }

        if (page.filename.isEmpty())
        {
            auto* item = new QListWidgetItem(page.title, m_nav);
            item->setFlags(Qt::NoItemFlags);
            item->setForeground(QColor(100, 100, 100));
            QFont f = item->font();
            f.setPointSize(9);
            f.setBold(true);
            f.setLetterSpacing(QFont::AbsoluteSpacing, 1.0);
            item->setFont(f);
            item->setSizeHint({0, 28});
            continue;
        }

        auto* item = new QListWidgetItem("  " + page.title, m_nav);
        item->setData(Qt::UserRole, page.filename);
    }
}

QString DocumentationWidget::resolveDocsDir() const
{
    const QString fromApp = QDir(QCoreApplication::applicationDirPath()).filePath("resources/docs");
    if (QFileInfo::exists(fromApp))
        return fromApp;

    const QString fromCwd = QDir::current().filePath("resources/docs");
    if (QFileInfo::exists(fromCwd))
        return fromCwd;

    return {};
}

void DocumentationWidget::loadPage(const QString& filename)
{
    const QString docsDir = resolveDocsDir();
    if (docsDir.isEmpty())
    {
        m_browser->setHtml("<h2 style='color:#e04800'>Documentation not found</h2>"
                           "<p>Could not locate <code>resources/docs/</code> directory.</p>");
        return;
    }

    QFile file(QDir(docsDir).filePath(filename));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        m_browser->setHtml("<h2 style='color:#e04800'>Page not found</h2>"
                           "<p>Could not open <code>" + filename + "</code>.</p>");
        return;
    }

    QTextStream stream(&file);
    const QString markdown = stream.readAll();
    file.close();

    const QString body = markdownToHtml(markdown);
    const QString html = "<html><body>" + body + "</body></html>";
    m_browser->document()->setDefaultStyleSheet(kDocCss);
    m_browser->setHtml(html);
    m_browser->verticalScrollBar()->setValue(0);

    m_currentFile = filename;
    updateBookmarkButton();
}

bool DocumentationWidget::eventFilter(QObject* obj, QEvent* event)
{
    if (event->type() == QEvent::MouseButtonRelease)
    {
        auto* widget = qobject_cast<QWidget*>(obj);
        if (widget)
        {
            const QString url = widget->property("newsUrl").toString();
            if (!url.isEmpty())
            {
                QDesktopServices::openUrl(QUrl(url));
                return true;
            }
        }
    }
    return QWidget::eventFilter(obj, event);
}

// ── News feed ─────────────────────────────────────────────────────────────────

void DocumentationWidget::buildNewsFeed()
{
    m_newsPanel = new QWidget(this);
    m_newsPanel->setFixedWidth(280);
    m_newsPanel->setStyleSheet("background-color: #161616; border-left: 1px solid #2a2a2a;");

    auto* panelLayout = new QVBoxLayout(m_newsPanel);
    panelLayout->setContentsMargins(0, 0, 0, 0);
    panelLayout->setSpacing(0);

    // Header
    m_newsHeader = new QLabel("NEWS & BLOG", m_newsPanel);
    m_newsHeader->setStyleSheet(
        "QLabel { color: #646464; font-size: 10px; font-weight: bold;"
        "  letter-spacing: 1px; padding: 12px 14px 8px 14px;"
        "  border: none; background: transparent; }"
    );
    panelLayout->addWidget(m_newsHeader);

    // Scroll area for cards
    m_newsScroll = new QScrollArea(m_newsPanel);
    m_newsScroll->setWidgetResizable(true);
    m_newsScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_newsScroll->setStyleSheet(
        "QScrollArea { border: none; background: transparent; }"
        "QScrollBar:vertical { background: #141414; width: 5px; border-radius: 2px; }"
        "QScrollBar::handle:vertical { background: #383838; border-radius: 2px; min-height: 20px; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }"
    );

    auto* container = new QWidget();
    container->setStyleSheet("background: transparent;");
    m_newsLayout = new QVBoxLayout(container);
    m_newsLayout->setContentsMargins(10, 4, 10, 10);
    m_newsLayout->setSpacing(8);
    m_newsLayout->addStretch();
    m_newsScroll->setWidget(container);

    panelLayout->addWidget(m_newsScroll, 1);
}

QWidget* DocumentationWidget::createNewsCard(const NewsEntry& entry)
{
    auto* card = new QWidget();
    card->setCursor(Qt::PointingHandCursor);
    card->setStyleSheet(
        "QWidget#newsCard { background-color: #1e1e1e; border: 1px solid #2a2a2a;"
        "  border-radius: 6px; }"
        "QWidget#newsCard:hover { border-color: #e04800; background-color: #222222; }"
    );
    card->setObjectName("newsCard");

    auto* layout = new QVBoxLayout(card);
    layout->setContentsMargins(12, 10, 12, 10);
    layout->setSpacing(4);

    // Tag badge
    if (!entry.tag.isEmpty())
    {
        auto* tag = new QLabel(entry.tag.toUpper(), card);
        QString tagColor = "#e04800";
        if (entry.tag.toLower() == "blog")        tagColor = "#3a8fd6";
        else if (entry.tag.toLower() == "update")  tagColor = "#4caf50";
        else if (entry.tag.toLower() == "release") tagColor = "#e04800";

        tag->setStyleSheet(
            QString("QLabel { color: %1; font-size: 9px; font-weight: bold;"
                    "  letter-spacing: 0.5px; border: none; background: transparent;"
                    "  padding: 0; }").arg(tagColor)
        );
        layout->addWidget(tag);
    }

    // Title
    auto* title = new QLabel(entry.title, card);
    title->setWordWrap(true);
    title->setStyleSheet(
        "QLabel { color: #e0e0e0; font-size: 12px; font-weight: bold;"
        "  border: none; background: transparent; padding: 0; }"
    );
    layout->addWidget(title);

    // Summary
    if (!entry.summary.isEmpty())
    {
        auto* summary = new QLabel(entry.summary, card);
        summary->setWordWrap(true);
        summary->setMaximumHeight(48);
        summary->setStyleSheet(
            "QLabel { color: #888888; font-size: 11px; border: none;"
            "  background: transparent; padding: 0; line-height: 1.3; }"
        );
        layout->addWidget(summary);
    }

    // Date
    if (!entry.date.isEmpty())
    {
        auto* date = new QLabel(entry.date, card);
        date->setStyleSheet(
            "QLabel { color: #555555; font-size: 10px; border: none;"
            "  background: transparent; padding: 2px 0 0 0; }"
        );
        layout->addWidget(date);
    }

    // Click to open URL
    if (!entry.url.isEmpty())
    {
        const QString url = entry.url;
        card->installEventFilter(this);
        card->setProperty("newsUrl", url);
    }

    return card;
}

void DocumentationWidget::populateNewsCards(const QList<NewsEntry>& entries)
{
    // Remove existing cards (keep the stretch)
    while (m_newsLayout->count() > 1)
    {
        auto* item = m_newsLayout->takeAt(0);
        if (item->widget())
            item->widget()->deleteLater();
        delete item;
    }

    for (const NewsEntry& entry : entries)
        m_newsLayout->insertWidget(m_newsLayout->count() - 1, createNewsCard(entry));
}

void DocumentationWidget::loadNewsFromFile()
{
    // Try to load from resources/news.json
    QString path;
    const QString fromApp = QDir(QCoreApplication::applicationDirPath()).filePath("resources/news.json");
    if (QFileInfo::exists(fromApp))
        path = fromApp;
    else
    {
        const QString fromCwd = QDir::current().filePath("resources/news.json");
        if (QFileInfo::exists(fromCwd))
            path = fromCwd;
    }

    if (path.isEmpty())
        return;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return;

    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (!doc.isArray())
        return;

    QList<NewsEntry> entries;
    for (const QJsonValue& val : doc.array())
    {
        const QJsonObject obj = val.toObject();
        entries.append({
            obj["title"].toString(),
            obj["date"].toString(),
            obj["summary"].toString(),
            obj["url"].toString(),
            obj["tag"].toString()
        });
    }

    if (!entries.isEmpty())
        populateNewsCards(entries);
}

void DocumentationWidget::loadNewsFromNetwork()
{
    m_netManager = new QNetworkAccessManager(this);
    connect(m_netManager, &QNetworkAccessManager::finished,
            this, &DocumentationWidget::onNewsFetchFinished);

    // Fetch from Velix news endpoint (configurable)
    QString feedUrl;
    const QString configPath = QDir(QCoreApplication::applicationDirPath()).filePath("resources/news_url.txt");
    QFile configFile(configPath);
    if (configFile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        feedUrl = QString::fromUtf8(configFile.readAll()).trimmed();
        configFile.close();
    }

    if (feedUrl.isEmpty())
        return;

    m_netManager->get(QNetworkRequest(QUrl(feedUrl)));
}

void DocumentationWidget::onNewsFetchFinished(QNetworkReply* reply)
{
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError)
        return;

    const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    if (!doc.isArray())
        return;

    QList<NewsEntry> entries;
    for (const QJsonValue& val : doc.array())
    {
        const QJsonObject obj = val.toObject();
        entries.append({
            obj["title"].toString(),
            obj["date"].toString(),
            obj["summary"].toString(),
            obj["url"].toString(),
            obj["tag"].toString()
        });
    }

    if (!entries.isEmpty())
        populateNewsCards(entries);
}

// ── Navigation & search slots ────────────────────────────────────────────────

void DocumentationWidget::onNavItemClicked(QListWidgetItem* item)
{
    const QString filename = item->data(Qt::UserRole).toString();
    if (filename.isEmpty())
        return;

    loadPage(filename);
}

void DocumentationWidget::onSearchChanged(const QString& text)
{
    const QString lower = text.trimmed().toLower();
    for (int i = 0; i < m_nav->count(); ++i)
    {
        QListWidgetItem* item = m_nav->item(i);
        const QString filename = item->data(Qt::UserRole).toString();
        if (filename.isEmpty())
        {
            // Always show section headers and spacers
            item->setHidden(false);
            continue;
        }
        const bool match = lower.isEmpty() ||
                           item->text().toLower().contains(lower);
        item->setHidden(!match);
    }

    // If searching, try to find matching content in files
    if (!lower.isEmpty())
    {
        const QString docsDir = resolveDocsDir();
        if (docsDir.isEmpty()) return;

        QString results;
        for (const DocPage& page : m_pages)
        {
            if (page.filename.isEmpty()) continue;

            QFile f(QDir(docsDir).filePath(page.filename));
            if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) continue;

            QTextStream ts(&f);
            const QString content = ts.readAll();
            f.close();

            if (!content.toLower().contains(lower)) continue;

            // Extract lines containing the search term
            const QStringList lines = content.split('\n');
            results += "<h3>" + page.title + "</h3><ul>";
            int found = 0;
            for (const QString& line : lines)
            {
                if (found >= 5) break;
                if (!line.toLower().contains(lower)) continue;

                QString escaped = line.trimmed().toHtmlEscaped();
                // Highlight the match
                const int idx = escaped.toLower().indexOf(lower);
                if (idx >= 0)
                {
                    escaped = escaped.left(idx)
                        + "<b style='color:#ff5c10'>"
                        + escaped.mid(idx, lower.length())
                        + "</b>"
                        + escaped.mid(idx + lower.length());
                }
                results += "<li>" + escaped + "</li>";
                ++found;
            }
            results += "</ul>";
        }

        if (!results.isEmpty())
        {
            m_browser->document()->setDefaultStyleSheet(kDocCss);
            m_browser->setHtml("<html><body><h1>Search results for \"" +
                               text.toHtmlEscaped() + "\"</h1>" + results + "</body></html>");
        }
    }
    else if (!m_currentFile.isEmpty())
    {
        // Restore the current page when search is cleared
        loadPage(m_currentFile);
    }
}

void DocumentationWidget::onBackClicked()
{
    m_browser->backward();
}

void DocumentationWidget::onForwardClicked()
{
    m_browser->forward();
}

// ── Bookmarks ────────────────────────────────────────────────────────────────

void DocumentationWidget::loadBookmarks()
{
    m_bookmarks.clear();
    const QString path = QDir(QCoreApplication::applicationDirPath())
                             .filePath("resources/bookmarks.json");
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return;

    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (!doc.isArray())
        return;

    for (const QJsonValue& val : doc.array())
    {
        const QString fn = val.toString();
        if (!fn.isEmpty())
            m_bookmarks.append(fn);
    }
}

void DocumentationWidget::saveBookmarks()
{
    QJsonArray arr;
    for (const QString& bm : m_bookmarks)
        arr.append(bm);

    const QString path = QDir(QCoreApplication::applicationDirPath())
                             .filePath("resources/bookmarks.json");
    QFile file(path);
    if (file.open(QIODevice::WriteOnly))
    {
        file.write(QJsonDocument(arr).toJson(QJsonDocument::Indented));
        file.close();
    }
}

void DocumentationWidget::toggleBookmark()
{
    if (m_currentFile.isEmpty())
        return;

    if (m_bookmarks.contains(m_currentFile))
        m_bookmarks.removeAll(m_currentFile);
    else
        m_bookmarks.append(m_currentFile);

    saveBookmarks();
    buildNav();
    updateBookmarkButton();
}

void DocumentationWidget::updateBookmarkButton()
{
    if (m_currentFile.isEmpty())
    {
        m_bookmarkBtn->setText(QString::fromUtf8("☆"));
        m_bookmarkBtn->setToolTip("Bookmark this page");
        return;
    }

    if (m_bookmarks.contains(m_currentFile))
    {
        m_bookmarkBtn->setText(QString::fromUtf8("★"));
        m_bookmarkBtn->setToolTip("Remove bookmark");
    }
    else
    {
        m_bookmarkBtn->setText(QString::fromUtf8("☆"));
        m_bookmarkBtn->setToolTip("Bookmark this page");
    }
}
