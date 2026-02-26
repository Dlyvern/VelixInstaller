#include "ProjectWidget.hpp"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPainter>
#include <QPainterPath>
#include <QLinearGradient>

#include "widgets/VelixText.hpp"
#include "FireButton.hpp"

namespace
{
constexpr int kCardHeight = 148;
constexpr int kThumbnailWidth = 220;
constexpr int kThumbnailHeight = 118;
}

ProjectWidget::ProjectWidget(const project::ProjectData& projectData, QWidget* parent) : QWidget(parent), m_projectData(projectData)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setFixedHeight(kCardHeight);
    setAttribute(Qt::WA_Hover, true);

    m_logo.load("./resources/VelixFire.png");

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

    m_openButton = new FireButton("Open", FireButton::Variant::Secondary, this);
    m_openButton->setFixedWidth(110);
    connect(m_openButton, &QPushButton::clicked, this, [this]
    {
        emit openRequested(QString::fromStdString(m_projectData.path));
    });

    mainLayout->addWidget(m_openButton, 0, Qt::AlignRight | Qt::AlignTop);
}

void ProjectWidget::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

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

    QLinearGradient thumbGradient(thumbRect.topLeft(), thumbRect.bottomRight());
    thumbGradient.setColorAt(0.0, QColor(33, 33, 33));
    thumbGradient.setColorAt(1.0, QColor(19, 19, 19));
    painter.fillPath(thumbPath, thumbGradient);
    painter.setPen(QPen(QColor(90, 90, 90), 1));
    painter.drawPath(thumbPath);

    painter.save();
    painter.setClipPath(thumbPath);
    QRadialGradient accent(thumbRect.left() + thumbRect.width() * 0.35, thumbRect.bottom() + 20, thumbRect.width() * 0.85);
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
    painter.drawText(titleRect, Qt::AlignLeft | Qt::AlignVCenter, QString::fromStdString(m_projectData.name));

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

ProjectWidget::~ProjectWidget() = default;
