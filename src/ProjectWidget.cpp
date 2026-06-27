#include "ProjectWidget.hpp"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPainter>
#include <QPainterPath>
#include <QLinearGradient>
#include <QRadialGradient>
#include <QFontMetrics>
#include <QDir>
#include <QDirIterator>

#include "widgets/VelixText.hpp"
#include "FireButton.hpp"
#include "Theme.hpp"

namespace
{
constexpr int kCardHeight = 150;
constexpr int kThumbnailWidth = 200;
constexpr int kThumbnailHeight = 112;
}

ProjectWidget::ProjectWidget(const project::ProjectData& projectData, QWidget* parent) : QWidget(parent), m_projectData(projectData)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setFixedHeight(kCardHeight);
    setAttribute(Qt::WA_Hover, true);

    m_logo.load("./resources/VelixFire.png");
    loadThumbnail();
    calculateDiskUsage();

    auto* mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(14, 14, 14, 14);
    mainLayout->setSpacing(14);

    // Reserve space for the painted thumbnail on the left.
    mainLayout->addSpacing(kThumbnailWidth + 4);

    auto* labelsLayout = new QVBoxLayout();
    labelsLayout->setSpacing(4);
    labelsLayout->setContentsMargins(0, 0, 0, 0);

    m_projectNameLabel = new VelixText(QString::fromStdString(projectData.name), this);
    m_projectNameLabel->setFont(theme::uiFont(12, true));
    m_projectNameLabel->setTextColor(theme::text);

    m_projectPathLabel = new VelixText(QString::fromStdString(projectData.path), this);
    m_projectPathLabel->setFont(theme::monoFont(8));
    m_projectPathLabel->setTextColor(theme::text3);

    labelsLayout->addWidget(m_projectNameLabel);
    labelsLayout->addWidget(m_projectPathLabel);
    labelsLayout->addStretch(1);

    mainLayout->addLayout(labelsLayout, 1);

    auto* buttonsLayout = new QVBoxLayout();
    buttonsLayout->setSpacing(6);
    buttonsLayout->setContentsMargins(0, 0, 0, 0);

    m_openButton = new FireButton("Open", FireButton::Variant::Primary, this);
    m_openButton->setFixedWidth(110);
    connect(m_openButton, &QPushButton::clicked, this, [this]
    {
        emit openRequested(QString::fromStdString(m_projectData.path));
    });

    m_editButton = new FireButton("Edit", FireButton::Variant::Secondary, this);
    m_editButton->setFixedWidth(110);
    connect(m_editButton, &QPushButton::clicked, this, [this]
    {
        emit editRequested(QString::fromStdString(m_projectData.path),
                           QString::fromStdString(m_projectData.name));
    });

    m_removeButton = new FireButton("Remove", FireButton::Variant::Secondary, this);
    m_removeButton->setFixedWidth(110);
    connect(m_removeButton, &QPushButton::clicked, this, [this]
    {
        emit removeRequested(QString::fromStdString(m_projectData.projectFilePath));
    });

    buttonsLayout->addWidget(m_openButton);
    buttonsLayout->addWidget(m_editButton);
    buttonsLayout->addWidget(m_removeButton);
    buttonsLayout->addStretch(1);

    mainLayout->addLayout(buttonsLayout, 0);
}

void ProjectWidget::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    // ── Card surface ─────────────────────────────────────────────────────
    const QRectF cardRect = rect().adjusted(0.5, 0.5, -0.5, -0.5);
    QPainterPath cardPath;
    cardPath.addRoundedRect(cardRect, theme::radiusCard, theme::radiusCard);

    painter.fillPath(cardPath, m_isHovered ? theme::surface1Hover : theme::surface1);
    painter.setPen(QPen(m_isHovered ? theme::withAlpha(theme::accent, 96) : theme::border, 1));
    painter.drawPath(cardPath);

    // ── Thumbnail ────────────────────────────────────────────────────────
    const QRectF thumbRect(14, 14, kThumbnailWidth, kThumbnailHeight);
    QPainterPath thumbPath;
    thumbPath.addRoundedRect(thumbRect, theme::radiusField, theme::radiusField);

    if (!m_thumbnail.isNull())
    {
        painter.save();
        painter.setClipPath(thumbPath);

        const QSizeF scaled = m_thumbnail.size().scaled(
            thumbRect.size().toSize(), Qt::KeepAspectRatioByExpanding);
        const QRectF dstRect(
            thumbRect.left() + (thumbRect.width()  - scaled.width())  / 2.0,
            thumbRect.top()  + (thumbRect.height() - scaled.height()) / 2.0,
            scaled.width(), scaled.height());
        painter.drawPixmap(dstRect, m_thumbnail, m_thumbnail.rect());

        // Soft ember glow bottom-right (matches design Thumb primitive).
        QRadialGradient hoverGlow(thumbRect.right(), thumbRect.bottom(),
                                   thumbRect.width() * 0.7);
        hoverGlow.setColorAt(0.0, theme::withAlpha(theme::accent, m_isHovered ? 70 : 38));
        hoverGlow.setColorAt(1.0, Qt::transparent);
        painter.fillRect(thumbRect, hoverGlow);

        painter.restore();
    }
    else
    {
        // ── Fallback: striped placeholder, deterministic hue per project ─
        QLinearGradient thumbGradient(thumbRect.topLeft(), thumbRect.bottomRight());
        thumbGradient.setColorAt(0.0, theme::surface2);
        thumbGradient.setColorAt(1.0, theme::surface0);
        painter.fillPath(thumbPath, thumbGradient);

        painter.save();
        painter.setClipPath(thumbPath);
        QRadialGradient accent(thumbRect.right(), thumbRect.bottom(),
                               thumbRect.width() * 0.85);
        accent.setColorAt(0.0, theme::withAlpha(theme::accent, 56));
        accent.setColorAt(1.0, Qt::transparent);
        painter.fillRect(thumbRect, accent);
        painter.restore();

        if (!m_logo.isNull())
        {
            const QSize logoSize(46, 46);
            const QPoint logoTopLeft(
                static_cast<int>(thumbRect.left()) + 12,
                static_cast<int>(thumbRect.top()) + 10
            );
            painter.drawPixmap(QRect(logoTopLeft, logoSize), m_logo);
        }
    }

    painter.setPen(QPen(theme::border, 1));
    painter.drawPath(thumbPath);

    // Mono category label on thumbnail (top-left), matches Thumb in primitives.
    {
        QFont label = theme::monoFont(7);
        painter.setFont(label);
        painter.setPen(QColor(255, 255, 255, 140));
        painter.drawText(QRectF(thumbRect.left() + 10, thumbRect.top() + 8,
                                thumbRect.width() - 20, 14),
                         Qt::AlignLeft | Qt::AlignVCenter,
                         QString::fromStdString(m_projectData.name).left(14).toUpper());
    }

    // ── Disk usage badge (mono, on thumbnail bottom-right) ───────────────
    if (!m_diskUsage.isEmpty())
    {
        QFont badgeFont = theme::monoFont(7, true);
        painter.setFont(badgeFont);

        const QFontMetrics fm(badgeFont);
        const int textW = fm.horizontalAdvance(m_diskUsage) + 12;
        const int badgeH = 16;
        const QRectF badgeRect(
            thumbRect.right() - textW - 6,
            thumbRect.bottom() - badgeH - 6,
            textW, badgeH);

        QPainterPath badgePath;
        badgePath.addRoundedRect(badgeRect, 4, 4);
        painter.fillPath(badgePath, QColor(0, 0, 0, 160));
        painter.setPen(QColor(220, 220, 220));
        painter.drawText(badgeRect, Qt::AlignCenter, m_diskUsage);
    }
}

void ProjectWidget::enterEvent(QEnterEvent* event)
{
    m_isHovered = true;
    update();
    QWidget::enterEvent(event);
}

void ProjectWidget::leaveEvent(QEvent* event)
{
    m_isHovered = false;
    update();
    QWidget::leaveEvent(event);
}

const project::ProjectData& ProjectWidget::getProjectData() const
{
    return m_projectData;
}

QString ProjectWidget::thumbnailPath(const std::string& projectDir)
{
    return QDir(QString::fromStdString(projectDir)).filePath(".velix_thumbnail.png");
}

void ProjectWidget::loadThumbnail()
{
    const QString path = thumbnailPath(m_projectData.path);
    QPixmap px;
    if (px.load(path) && !px.isNull())
        m_thumbnail = px;
}

void ProjectWidget::reloadThumbnail()
{
    loadThumbnail();
    update();
}

void ProjectWidget::calculateDiskUsage()
{
    qint64 totalBytes = 0;
    QDirIterator it(QString::fromStdString(m_projectData.path),
                    QDir::Files | QDir::NoDotAndDotDot,
                    QDirIterator::Subdirectories);
    while (it.hasNext())
    {
        it.next();
        totalBytes += it.fileInfo().size();
    }

    if (totalBytes < 1024)
        m_diskUsage = QString::number(totalBytes) + " B";
    else if (totalBytes < 1024 * 1024)
        m_diskUsage = QString::number(totalBytes / 1024.0, 'f', 1) + " KB";
    else if (totalBytes < 1024LL * 1024 * 1024)
        m_diskUsage = QString::number(totalBytes / (1024.0 * 1024.0), 'f', 1) + " MB";
    else
        m_diskUsage = QString::number(totalBytes / (1024.0 * 1024.0 * 1024.0), 'f', 2) + " GB";
}

ProjectWidget::~ProjectWidget() = default;
