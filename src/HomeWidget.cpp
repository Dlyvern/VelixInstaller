#include "HomeWidget.hpp"

#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPainter>
#include <QPainterPath>
#include <QRadialGradient>
#include <QLinearGradient>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QMouseEvent>

#include "FireLogoWidget.hpp"
#include "Theme.hpp"
#include "Config.hpp"

namespace
{
// A QFrame that paints itself with a card-like rounded background.
class Card : public QWidget
{
public:
    explicit Card(QWidget* parent = nullptr, bool dashed = false, bool interactive = false)
        : QWidget(parent), m_dashed(dashed), m_interactive(interactive)
    {
        setAttribute(Qt::WA_Hover, true);
        if (interactive)
            setCursor(Qt::PointingHandCursor);
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        const QRectF r = rect().adjusted(0.5, 0.5, -0.5, -0.5);
        QPainterPath path;
        path.addRoundedRect(r, theme::radiusCard, theme::radiusCard);
        p.fillPath(path, m_hover ? theme::surface1Hover : theme::surface1);
        QPen pen(m_hover ? theme::borderHover : theme::border, 1);
        if (m_dashed)
            pen.setStyle(Qt::DashLine);
        p.setPen(pen);
        p.drawPath(path);
    }

    void enterEvent(QEnterEvent*) override { if (m_interactive) { m_hover = true;  update(); } }
    void leaveEvent(QEvent*) override      { if (m_interactive) { m_hover = false; update(); } }

private:
    bool m_dashed{false};
    bool m_interactive{false};
    bool m_hover{false};
};

// Small clickable label-style button used for "View all →" links.
class LinkLabel : public QLabel
{
public:
    explicit LinkLabel(const QString& text, QWidget* parent = nullptr) : QLabel(text, parent)
    {
        setCursor(Qt::PointingHandCursor);
        setFont(theme::uiFont(9));
        QPalette pal = palette();
        pal.setColor(QPalette::WindowText, theme::accentBright);
        setPalette(pal);
    }
};

QString findResource(const QString& relative)
{
    const QString fromApp = QDir(QCoreApplication::applicationDirPath()).filePath(relative);
    if (QFileInfo::exists(fromApp))
        return fromApp;
    const QString fromCwd = QDir::current().filePath(relative);
    if (QFileInfo::exists(fromCwd))
        return fromCwd;
    return {};
}

// Pill widget — small rounded label coloured by tone.
class Pill : public QLabel
{
public:
    enum Tone { Neutral, Accent, Warn, Dev };

    Pill(const QString& text, Tone tone, QWidget* parent = nullptr) : QLabel(text.toUpper(), parent), m_tone(tone)
    {
        setFont(theme::uiFont(7, true));
        setAlignment(Qt::AlignCenter);
        setContentsMargins(8, 0, 8, 0);
        setFixedHeight(18);
        adjustSize();
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        QColor base;
        switch (m_tone)
        {
            case Accent:  base = theme::accent;  break;
            case Warn:    base = theme::preview; break;
            case Dev:     base = theme::dev;     break;
            case Neutral: base = theme::text3;   break;
        }
        const QRectF r = rect().adjusted(0.5, 0.5, -0.5, -0.5);
        QPainterPath path;
        path.addRoundedRect(r, height() / 2.0, height() / 2.0);
        p.fillPath(path, theme::withAlpha(base, m_tone == Neutral ? 30 : 36));
        p.setPen(QPen(theme::withAlpha(base, m_tone == Neutral ? 60 : 90), 1));
        p.drawPath(path);
        p.setPen(m_tone == Accent ? theme::accentBright : base);
        p.drawText(rect(), Qt::AlignCenter, text());
    }

private:
    Tone m_tone;
};

// Event filter that paints the stat-tile background (rounded rect + accent
// stripe) underneath its label children.
class TilePainter : public QObject
{
public:
    TilePainter(bool accent, QWidget* target) : QObject(target), m_accent(accent), m_target(target) {}

    bool eventFilter(QObject* obj, QEvent* e) override
    {
        if (obj == m_target && e->type() == QEvent::Paint)
        {
            QPainter p(m_target);
            p.setRenderHint(QPainter::Antialiasing);
            const QRectF r = m_target->rect().adjusted(0.5, 0.5, -0.5, -0.5);
            QPainterPath path;
            path.addRoundedRect(r, 10, 10);
            p.fillPath(path, theme::withAlpha(theme::surface2, 200));
            QColor stroke = m_accent ? theme::withAlpha(theme::accent, 95) : theme::border;
            p.setPen(QPen(stroke, 1));
            p.drawPath(path);
            if (m_accent)
            {
                QPainterPath bar;
                bar.addRoundedRect(QRectF(r.left(), r.top() + 6, 2, r.height() - 12), 1, 1);
                QLinearGradient g(QPointF(0, r.top()), QPointF(0, r.bottom()));
                g.setColorAt(0.0, theme::accent);
                g.setColorAt(1.0, theme::accentDeep);
                p.fillPath(bar, g);
            }
        }
        return false;
    }

private:
    bool m_accent;
    QWidget* m_target;
};

// The hero header — gradient surface with an ember radial glow + flame + title.
class Hero : public QWidget
{
public:
    explicit Hero(QWidget* parent = nullptr) : QWidget(parent)
    {
        setMinimumHeight(180);
        setAttribute(Qt::WA_StyledBackground, false);
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        const QRectF r = rect().adjusted(0.5, 0.5, -0.5, -0.5);
        QPainterPath path;
        path.addRoundedRect(r, 14, 14);

        // Diagonal surface gradient
        QLinearGradient grad(r.topLeft(), r.bottomRight());
        grad.setColorAt(0.0, theme::surface1);
        grad.setColorAt(1.0, theme::surface0);
        p.fillPath(path, grad);

        // Ember glow top-right
        p.save();
        p.setClipPath(path);
        QRadialGradient ember(r.right() - 20, r.top() - 40, 360);
        ember.setColorAt(0.0, theme::withAlpha(theme::accent, 70));
        ember.setColorAt(1.0, Qt::transparent);
        p.fillRect(r, ember);
        p.restore();

        p.setPen(QPen(theme::border, 1));
        p.drawPath(path);
    }
};
} // namespace

HomeWidget::HomeWidget(QWidget* parent) : QWidget(parent)
{
    setAttribute(Qt::WA_StyledBackground, true);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(16, 12, 16, 16);
    root->setSpacing(20);

    buildHero(root);
    buildBody(root);
    root->addStretch(1);

    refresh();
}

void HomeWidget::buildHero(QVBoxLayout* parent)
{
    auto* hero = new Hero(this);
    auto* heroLayout = new QVBoxLayout(hero);
    heroLayout->setContentsMargins(28, 24, 28, 22);
    heroLayout->setSpacing(20);

    // Title row
    auto* titleRow = new QHBoxLayout();
    titleRow->setContentsMargins(0, 0, 0, 0);
    titleRow->setSpacing(14);

    auto* flame = new FireLogoWidget(hero);
    titleRow->addWidget(flame, 0, Qt::AlignVCenter);

    auto* titleCol = new QVBoxLayout();
    titleCol->setContentsMargins(0, 0, 0, 0);
    titleCol->setSpacing(2);

    auto* welcome = new QLabel("WELCOME BACK", hero);
    welcome->setFont(theme::uiFont(8, true));
    {
        QPalette pal = welcome->palette();
        pal.setColor(QPalette::WindowText, theme::text3);
        welcome->setPalette(pal);
    }

    auto* title = new QLabel("Velix Installer", hero);
    QFont titleFont = theme::uiFont(22, true);
    titleFont.setLetterSpacing(QFont::PercentageSpacing, 98);
    title->setFont(titleFont);
    {
        QPalette pal = title->palette();
        pal.setColor(QPalette::WindowText, theme::text);
        title->setPalette(pal);
    }

    titleCol->addWidget(welcome);
    titleCol->addWidget(title);
    titleRow->addLayout(titleCol);
    titleRow->addStretch(1);

    heroLayout->addLayout(titleRow);

    // Stat tiles row
    auto* tiles = new QGridLayout();
    tiles->setContentsMargins(0, 0, 0, 0);
    tiles->setHorizontalSpacing(12);
    tiles->setVerticalSpacing(12);

    auto* tileEngine = buildStatTile("ACTIVE ENGINE", "—", "Default version", true);
    auto* tileInstalled = buildStatTile("INSTALLED VERSIONS", "0", "0 available to install", false);
    auto* tileProjects = buildStatTile("PROJECTS", "0", "Local workspace", false);

    tiles->addWidget(tileEngine, 0, 0);
    tiles->addWidget(tileInstalled, 0, 1);
    tiles->addWidget(tileProjects, 0, 2);
    tiles->setColumnStretch(0, 1);
    tiles->setColumnStretch(1, 1);
    tiles->setColumnStretch(2, 1);

    heroLayout->addLayout(tiles);

    parent->addWidget(hero);
}

QWidget* HomeWidget::buildStatTile(const QString& label, const QString& value,
                                    const QString& sub, bool accent)
{
    auto* tile = new QWidget(this);
    tile->setMinimumHeight(76);
    tile->setAttribute(Qt::WA_StyledBackground, true);
    tile->setObjectName(accent ? "stat-tile-accent" : "stat-tile");

    auto* layout = new QVBoxLayout(tile);
    layout->setContentsMargins(16, 12, 16, 12);
    layout->setSpacing(2);

    auto* lbl = new QLabel(label, tile);
    lbl->setFont(theme::uiFont(7, true));
    {
        QPalette pal = lbl->palette();
        pal.setColor(QPalette::WindowText, theme::text3);
        lbl->setPalette(pal);
    }

    auto* val = new QLabel(value, tile);
    val->setFont(theme::monoFont(15, true));
    {
        QPalette pal = val->palette();
        pal.setColor(QPalette::WindowText, theme::text);
        val->setPalette(pal);
    }

    auto* subLbl = new QLabel(sub, tile);
    subLbl->setFont(theme::uiFont(8));
    {
        QPalette pal = subLbl->palette();
        pal.setColor(QPalette::WindowText, theme::text3);
        subLbl->setPalette(pal);
    }

    layout->addWidget(lbl);
    layout->addWidget(val);
    layout->addWidget(subLbl);

    if (accent)
    {
        m_currentVersionValue = val;
        m_currentVersionSub = subLbl;
    }
    else if (label == "INSTALLED VERSIONS")
    {
        m_installedCountValue = val;
        m_installedCountSub = subLbl;
    }
    else
    {
        m_projectsCountValue = val;
        m_projectsCountSub = subLbl;
    }

    // Custom paint for the tile background + 2px accent stripe.
    tile->installEventFilter(new TilePainter(accent, tile));

    return tile;
}

void HomeWidget::buildBody(QVBoxLayout* parent)
{
    auto* row = new QHBoxLayout();
    row->setContentsMargins(0, 0, 0, 0);
    row->setSpacing(20);

    // Left column: news
    auto* leftCol = new QVBoxLayout();
    leftCol->setContentsMargins(0, 0, 0, 0);
    leftCol->setSpacing(10);

    auto* newsHead = new QHBoxLayout();
    newsHead->setContentsMargins(0, 0, 0, 6);
    newsHead->setSpacing(12);
    auto* newsTitle = new QLabel("What's new", this);
    newsTitle->setFont(theme::uiFont(14, true));
    {
        QPalette pal = newsTitle->palette();
        pal.setColor(QPalette::WindowText, theme::text);
        newsTitle->setPalette(pal);
    }
    auto* newsSub = new QLabel("Latest from the Velix team", this);
    newsSub->setFont(theme::uiFont(8));
    {
        QPalette pal = newsSub->palette();
        pal.setColor(QPalette::WindowText, theme::text3);
        newsSub->setPalette(pal);
    }
    auto* newsTitleBlock = new QVBoxLayout();
    newsTitleBlock->setContentsMargins(0, 0, 0, 0);
    newsTitleBlock->setSpacing(0);
    newsTitleBlock->addWidget(newsTitle);
    newsTitleBlock->addWidget(newsSub);

    newsHead->addLayout(newsTitleBlock, 1);

    auto* newsLink = new LinkLabel("View all →", this);
    connect(static_cast<QObject*>(newsLink), &QObject::destroyed, this, []{});
    newsLink->setAlignment(Qt::AlignBottom | Qt::AlignRight);
    newsHead->addWidget(newsLink, 0, Qt::AlignBottom);
    leftCol->addLayout(newsHead);

    m_newsList = new QWidget(this);
    auto* newsListLayout = new QVBoxLayout(m_newsList);
    newsListLayout->setContentsMargins(0, 0, 0, 0);
    newsListLayout->setSpacing(10);
    leftCol->addWidget(m_newsList);
    leftCol->addStretch(1);

    // Right column: recent projects
    auto* rightCol = new QVBoxLayout();
    rightCol->setContentsMargins(0, 0, 0, 0);
    rightCol->setSpacing(10);

    auto* recentHead = new QHBoxLayout();
    recentHead->setContentsMargins(0, 0, 0, 6);
    recentHead->setSpacing(12);
    auto* recentTitle = new QLabel("Recent projects", this);
    recentTitle->setFont(theme::uiFont(14, true));
    {
        QPalette pal = recentTitle->palette();
        pal.setColor(QPalette::WindowText, theme::text);
        recentTitle->setPalette(pal);
    }
    recentHead->addWidget(recentTitle, 1, Qt::AlignBottom);
    auto* recentLink = new LinkLabel("All projects →", this);
    recentLink->setAlignment(Qt::AlignBottom | Qt::AlignRight);
    recentLink->installEventFilter(this);
    recentLink->setObjectName("link-projects");
    recentHead->addWidget(recentLink, 0, Qt::AlignBottom);
    rightCol->addLayout(recentHead);

    m_recentList = new QWidget(this);
    auto* recentListLayout = new QVBoxLayout(m_recentList);
    recentListLayout->setContentsMargins(0, 0, 0, 0);
    recentListLayout->setSpacing(10);
    rightCol->addWidget(m_recentList);
    rightCol->addStretch(1);

    row->addLayout(leftCol, 135);   // ratio 1.35 : 1 from prototype
    row->addLayout(rightCol, 100);

    parent->addLayout(row);
}

QWidget* HomeWidget::buildNewsRow(const NewsEntry& n, bool featured)
{
    auto* card = new Card(this, /*dashed=*/false, /*interactive=*/true);
    card->setMinimumHeight(featured ? 80 : 54);
    auto* layout = new QHBoxLayout(card);
    layout->setContentsMargins(featured ? 16 : 12, featured ? 14 : 10,
                               featured ? 16 : 12, featured ? 14 : 10);
    layout->setSpacing(12);

    auto* col = new QVBoxLayout();
    col->setContentsMargins(0, 0, 0, 0);
    col->setSpacing(featured ? 6 : 4);

    auto* meta = new QHBoxLayout();
    meta->setContentsMargins(0, 0, 0, 0);
    meta->setSpacing(8);

    Pill::Tone tone = Pill::Neutral;
    if (n.tag.compare("release", Qt::CaseInsensitive) == 0)      tone = Pill::Accent;
    else if (n.tag.compare("update",  Qt::CaseInsensitive) == 0) tone = Pill::Warn;
    else if (n.tag.compare("dev",     Qt::CaseInsensitive) == 0) tone = Pill::Dev;
    auto* pill = new Pill(n.tag.isEmpty() ? "NEWS" : n.tag, tone, card);
    meta->addWidget(pill, 0, Qt::AlignVCenter);

    auto* date = new QLabel(n.date, card);
    date->setFont(theme::monoFont(8));
    {
        QPalette pal = date->palette();
        pal.setColor(QPalette::WindowText, theme::text3);
        date->setPalette(pal);
    }
    meta->addWidget(date, 0, Qt::AlignVCenter);
    meta->addStretch(1);
    col->addLayout(meta);

    auto* title = new QLabel(n.title, card);
    title->setFont(theme::uiFont(featured ? 11 : 10, true));
    title->setWordWrap(true);
    {
        QPalette pal = title->palette();
        pal.setColor(QPalette::WindowText, theme::text);
        title->setPalette(pal);
    }
    col->addWidget(title);

    if (featured && !n.summary.isEmpty())
    {
        auto* sum = new QLabel(n.summary, card);
        sum->setFont(theme::uiFont(9));
        sum->setWordWrap(true);
        {
            QPalette pal = sum->palette();
            pal.setColor(QPalette::WindowText, theme::text2);
            sum->setPalette(pal);
        }
        col->addWidget(sum);
    }

    layout->addLayout(col, 1);
    return card;
}

QWidget* HomeWidget::buildMiniProjectRow(const QString& name, const QString& engine,
                                         const QString& opened)
{
    auto* card = new Card(this, /*dashed=*/false, /*interactive=*/true);
    card->setMinimumHeight(48);
    auto* layout = new QHBoxLayout(card);
    layout->setContentsMargins(12, 8, 12, 8);
    layout->setSpacing(10);

    auto* col = new QVBoxLayout();
    col->setContentsMargins(0, 0, 0, 0);
    col->setSpacing(2);

    auto* nameLbl = new QLabel(name, card);
    nameLbl->setFont(theme::uiFont(10, true));
    {
        QPalette pal = nameLbl->palette();
        pal.setColor(QPalette::WindowText, theme::text);
        nameLbl->setPalette(pal);
    }

    auto* metaLbl = new QLabel(QString("<span style=\"font-family: 'JetBrains Mono', monospace;\">%1</span> · %2")
                               .arg(engine, opened), card);
    metaLbl->setFont(theme::uiFont(8));
    metaLbl->setTextFormat(Qt::RichText);
    {
        QPalette pal = metaLbl->palette();
        pal.setColor(QPalette::WindowText, theme::text3);
        metaLbl->setPalette(pal);
    }

    col->addWidget(nameLbl);
    col->addWidget(metaLbl);
    layout->addLayout(col, 1);

    auto* arrow = new QLabel("→", card);
    arrow->setFont(theme::uiFont(11));
    {
        QPalette pal = arrow->palette();
        pal.setColor(QPalette::WindowText, theme::text3);
        arrow->setPalette(pal);
    }
    layout->addWidget(arrow, 0, Qt::AlignVCenter | Qt::AlignRight);

    return card;
}

QWidget* HomeWidget::buildNewProjectRow()
{
    auto* card = new Card(this, /*dashed=*/true, /*interactive=*/true);
    card->setMinimumHeight(44);
    auto* layout = new QHBoxLayout(card);
    layout->setContentsMargins(14, 10, 14, 10);
    layout->setSpacing(8);

    auto* plus = new QLabel("+", card);
    plus->setFont(theme::uiFont(11, true));
    {
        QPalette pal = plus->palette();
        pal.setColor(QPalette::WindowText, theme::text3);
        plus->setPalette(pal);
    }
    auto* lbl = new QLabel("New project", card);
    lbl->setFont(theme::uiFont(9));
    {
        QPalette pal = lbl->palette();
        pal.setColor(QPalette::WindowText, theme::text3);
        lbl->setPalette(pal);
    }
    layout->addStretch(1);
    layout->addWidget(plus);
    layout->addWidget(lbl);
    layout->addStretch(1);

    card->installEventFilter(this);
    card->setObjectName("new-project-row");
    return card;
}

QList<HomeWidget::NewsEntry> HomeWidget::loadNews() const
{
    const QString path = findResource("resources/news.json");
    QList<NewsEntry> entries;
    if (path.isEmpty())
        return entries;

    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        return entries;
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    f.close();

    if (!doc.isArray())
        return entries;
    for (const QJsonValue& v : doc.array())
    {
        const QJsonObject o = v.toObject();
        entries.append({
            o["title"].toString(), o["date"].toString(),
            o["summary"].toString(), o["url"].toString(),
            o["tag"].toString()
        });
    }
    return entries;
}

void HomeWidget::refresh()
{
    Config cfg;
    cfg.load();
    const auto& json = cfg.getConfig();

    // ── Stat tiles ────────────────────────────────────────────────────────
    QString currentVersion;
    if (json.contains("current_version") && json["current_version"].is_string())
        currentVersion = QString::fromStdString(json["current_version"].get<std::string>());
    if (currentVersion.isEmpty())
        currentVersion = "—";
    if (m_currentVersionValue) m_currentVersionValue->setText(currentVersion);

    int installed = 0;
    if (json.contains("installed_versions") && json["installed_versions"].is_array())
        installed = static_cast<int>(json["installed_versions"].size());
    if (m_installedCountValue) m_installedCountValue->setText(QString::number(installed));
    if (m_installedCountSub)
        m_installedCountSub->setText(installed == 0
                                      ? "Browse the Installs tab"
                                      : QString("%1 ready to launch").arg(installed));

    int projectCount = 0;
    if (json.contains("projects") && json["projects"].is_array())
        projectCount = static_cast<int>(json["projects"].size());
    if (m_projectsCountValue) m_projectsCountValue->setText(QString::number(projectCount));

    // ── News list ────────────────────────────────────────────────────────
    if (m_newsList)
    {
        QLayoutItem* child;
        while ((child = m_newsList->layout()->takeAt(0)) != nullptr)
        {
            if (child->widget()) child->widget()->deleteLater();
            delete child;
        }
        const auto entries = loadNews();
        const int n = std::min(4, static_cast<int>(entries.size()));
        for (int i = 0; i < n; ++i)
            m_newsList->layout()->addWidget(buildNewsRow(entries[i], /*featured=*/i == 0));
    }

    // ── Recent projects ──────────────────────────────────────────────────
    if (m_recentList)
    {
        QLayoutItem* child;
        while ((child = m_recentList->layout()->takeAt(0)) != nullptr)
        {
            if (child->widget()) child->widget()->deleteLater();
            delete child;
        }

        if (json.contains("projects") && json["projects"].is_array())
        {
            int shown = 0;
            for (const auto& p : json["projects"])
            {
                if (shown >= 3) break;
                if (!p.is_object()) continue;
                QString name = p.contains("name") && p["name"].is_string()
                               ? QString::fromStdString(p["name"].get<std::string>())
                               : QStringLiteral("Untitled");
                QString updated = p.contains("updated_at") && p["updated_at"].is_string()
                                  ? QString::fromStdString(p["updated_at"].get<std::string>())
                                  : QStringLiteral("recently");
                QString engine = currentVersion.isEmpty() ? QStringLiteral("—") : currentVersion;
                m_recentList->layout()->addWidget(buildMiniProjectRow(name, engine, updated));
                ++shown;
            }
        }

        m_recentList->layout()->addWidget(buildNewProjectRow());
    }
}

bool HomeWidget::eventFilter(QObject* watched, QEvent* event)
{
    if (event->type() == QEvent::MouseButtonRelease)
    {
        if (auto* w = qobject_cast<QWidget*>(watched))
        {
            if (w->objectName() == "new-project-row" || w->objectName() == "link-projects")
            {
                emit navigateRequested("Projects");
                return true;
            }
        }
    }
    return QWidget::eventFilter(watched, event);
}
