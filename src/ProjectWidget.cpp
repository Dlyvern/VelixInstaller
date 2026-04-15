#include "ProjectWidget.hpp"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPainter>
#include <QPainterPath>
#include <QLinearGradient>
#include <QRadialGradient>
#include <QDir>
#include <QDirIterator>

#include "widgets/VelixText.hpp"
#include "FireButton.hpp"

namespace
{
constexpr int kCardHeight = 170;
constexpr int kThumbnailWidth = 220;
constexpr int kThumbnailHeight = 118;
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
    mainLayout->setSpacing(12);

    mainLayout->addSpacing(kThumbnailWidth + 8);

    auto* labelsLayout = new QVBoxLayout();
    labelsLayout->setSpacing(5);
    labelsLayout->setContentsMargins(0, 0, 0, 0);

    m_projectNameLabel = new VelixText(QString::fromStdString(projectData.name), this);
    m_projectNameLabel->setPointSize(13);
    m_projectNameLabel->setTextColor(Qt::white);

    m_projectPathLabel = new VelixText(QString::fromStdString(projectData.path), this);
    m_projectPathLabel->setPointSize(9);
    m_projectPathLabel->setBold(false);
    m_projectPathLabel->setTextColor(QColor(160, 160, 160));

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

    const QRectF cardRect = rect().adjusted(1, 1, -1, -1);
    QPainterPath cardPath;
    cardPath.addRoundedRect(cardRect, 10, 10);

    QLinearGradient cardGradient(cardRect.topLeft(), cardRect.bottomLeft());
    if (m_isHovered)
    {
        cardGradient.setColorAt(0.0, QColor(58, 58, 58, 240));
        cardGradient.setColorAt(1.0, QColor(41, 41, 41, 240));
    }
    else
    {
        cardGradient.setColorAt(0.0, QColor(50, 50, 50, 235));
        cardGradient.setColorAt(1.0, QColor(34, 34, 34, 235));
    }

    painter.fillPath(cardPath, cardGradient);
    painter.setPen(QPen(m_isHovered ? QColor(255, 128, 40, 125) : QColor(72, 72, 72), 1));
    painter.drawPath(cardPath);

    const QRectF thumbRect(14, 14, kThumbnailWidth, kThumbnailHeight);
    QPainterPath thumbPath;
    thumbPath.addRoundedRect(thumbRect, 8, 8);

    if (!m_thumbnail.isNull())
    {
        // ── Screenshot thumbnail (UE5-style) ─────────────────────────────────
        painter.save();
        painter.setClipPath(thumbPath);

        // Scale-to-fill: scale the thumbnail to cover the entire thumb area
        const QSizeF scaled = m_thumbnail.size().scaled(
            thumbRect.size().toSize(), Qt::KeepAspectRatioByExpanding);
        const QRectF srcRect(
            (m_thumbnail.width()  - scaled.width()  * m_thumbnail.width()  / scaled.width())  / 2.0, 0,
            m_thumbnail.width(), m_thumbnail.height());
        const QRectF dstRect(
            thumbRect.left() + (thumbRect.width()  - scaled.width())  / 2.0,
            thumbRect.top()  + (thumbRect.height() - scaled.height()) / 2.0,
            scaled.width(), scaled.height());
        painter.drawPixmap(dstRect, m_thumbnail, srcRect);

        // Dark gradient overlay at bottom for text readability
        QLinearGradient textFade(thumbRect.left(), thumbRect.bottom() - 52,
                                 thumbRect.left(), thumbRect.bottom());
        textFade.setColorAt(0.0, QColor(0, 0, 0, 0));
        textFade.setColorAt(1.0, QColor(0, 0, 0, 190));
        painter.fillRect(thumbRect, textFade);

        // Subtle orange glow at the bottom when hovered
        if (m_isHovered)
        {
            QRadialGradient hoverGlow(thumbRect.center().x(), thumbRect.bottom(),
                                       thumbRect.width() * 0.6);
            hoverGlow.setColorAt(0.0, QColor(255, 95, 0, 35));
            hoverGlow.setColorAt(1.0, Qt::transparent);
            painter.fillRect(thumbRect, hoverGlow);
        }

        painter.restore();

        // Border
        painter.setPen(QPen(m_isHovered ? QColor(255, 128, 40, 160) : QColor(60, 60, 60), 1));
        painter.drawPath(thumbPath);
    }
    else
    {
        // ── Fallback: logo + gradient (original style) ───────────────────────
        QLinearGradient thumbGradient(thumbRect.topLeft(), thumbRect.bottomRight());
        thumbGradient.setColorAt(0.0, QColor(33, 33, 33));
        thumbGradient.setColorAt(1.0, QColor(19, 19, 19));
        painter.fillPath(thumbPath, thumbGradient);
        painter.setPen(QPen(QColor(90, 90, 90), 1));
        painter.drawPath(thumbPath);

        painter.save();
        painter.setClipPath(thumbPath);
        QRadialGradient accent(thumbRect.left() + thumbRect.width() * 0.35,
                               thumbRect.bottom() + 20,
                               thumbRect.width() * 0.85);
        accent.setColorAt(0.0, QColor(255, 95, 0, 55));
        accent.setColorAt(1.0, Qt::transparent);
        painter.fillRect(thumbRect, accent);
        painter.restore();

        if (!m_logo.isNull())
        {
            const QSize logoSize(52, 52);
            const QPoint logoTopLeft(
                static_cast<int>(thumbRect.left()) + 14,
                static_cast<int>(thumbRect.top()) + 12
            );
            painter.drawPixmap(QRect(logoTopLeft, logoSize), m_logo);
        }
    }

    // ── Title & subtitle on the thumbnail ────────────────────────────────────
    painter.setPen(QColor(238, 238, 238));
    QFont titleFont = painter.font();
    titleFont.setBold(true);
    titleFont.setPointSize(11);
    painter.setFont(titleFont);
    const QRect titleRect(
        static_cast<int>(thumbRect.left()) + 12,
        static_cast<int>(thumbRect.bottom()) - 40,
        static_cast<int>(thumbRect.width()) - 24,
        22
    );
    painter.drawText(titleRect, Qt::AlignLeft | Qt::AlignVCenter,
                     QString::fromStdString(m_projectData.name));

    painter.setPen(QColor(170, 170, 170));
    QFont subtitleFont = painter.font();
    subtitleFont.setBold(false);
    subtitleFont.setPointSize(8);
    painter.setFont(subtitleFont);
    painter.drawText(
        QRect(titleRect.left(), titleRect.bottom() - 2, titleRect.width(), 18),
        Qt::AlignLeft | Qt::AlignVCenter,
        "Velix Game Project"
    );

    // ── Disk usage badge (top-right of thumbnail) ────────────────────────────
    if (!m_diskUsage.isEmpty())
    {
        QFont badgeFont = painter.font();
        badgeFont.setPointSize(7);
        badgeFont.setBold(true);
        painter.setFont(badgeFont);

        const QFontMetrics fm(badgeFont);
        const int textW = fm.horizontalAdvance(m_diskUsage) + 10;
        const int badgeH = 16;
        const QRectF badgeRect(
            thumbRect.right() - textW - 6,
            thumbRect.top() + 6,
            textW, badgeH);

        QPainterPath badgePath;
        badgePath.addRoundedRect(badgeRect, 4, 4);
        painter.fillPath(badgePath, QColor(0, 0, 0, 160));
        painter.setPen(Qt::NoPen);
        painter.drawPath(badgePath);

        painter.setPen(QColor(200, 200, 200));
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
