#include "VersionWidget.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPainterPath>

#include "Theme.hpp"

VersionWidget::VersionWidget(const QString& tagName, const QString& downloadLink, bool isInstalled, QWidget* parent) :
QWidget(parent), m_downloadLink(downloadLink), m_tagName(tagName)
{
    auto mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(14, 12, 14, 12);
    mainLayout->setSpacing(12);

    // Timeline dot rendered in paintEvent — leave space for it.
    mainLayout->addSpacing(6);

    m_labelIcon = new QLabel(this);
    m_labelIcon->setFixedSize(28, 28);
    m_labelIcon->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(m_labelIcon);

    auto labelsWidget = new QWidget(this);
    auto labelsLayout = new QVBoxLayout(labelsWidget);
    labelsLayout->setContentsMargins(0, 0, 0, 0);
    labelsLayout->setSpacing(2);

    m_nameLabel = new VelixText(tagName, this);
    m_nameLabel->setFont(theme::monoFont(13, true));
    m_nameLabel->setTextColor(theme::text);

    m_statusLabel = new VelixText(this);
    m_statusLabel->setFont(theme::uiFont(8));
    m_statusLabel->setBold(false);
    m_statusLabel->setTextColor(theme::text3);

    labelsLayout->addWidget(m_nameLabel);
    labelsLayout->addWidget(m_statusLabel);

    mainLayout->addWidget(labelsWidget, 1);

    m_button = new FireButton("", FireButton::Variant::Primary, this);
    m_button->setFixedWidth(110);

    connect(m_button, &QPushButton::clicked, this, [this]
    {
        if (m_isInstalled)
            emit chooseVersion(m_tagName);
        else
            emit installVersion(m_tagName, m_downloadLink);
    });

    mainLayout->addWidget(m_button);

    m_deleteButton = new FireButton("Remove", FireButton::Variant::Secondary, this);
    m_deleteButton->setFixedWidth(80);
    m_deleteButton->hide();

    connect(m_deleteButton, &QPushButton::clicked, this, [this]
    {
        emit deleteVersion(m_tagName);
    });

    mainLayout->addWidget(m_deleteButton);

    setInstalled(isInstalled);
}

void VersionWidget::setDisabled(bool isDisabled)
{
    m_isDisabled = isDisabled;
    refreshButtonAndStatus();
    update();
}

void VersionWidget::setInstalled(bool isInstalled)
{
    m_isInstalled = isInstalled;

    QString iconPath = m_isInstalled ? "./resources/checked.png" : "./resources/cloud.png";
    QPixmap pixmap(iconPath);

    if (pixmap.isNull())
        qWarning() << "Failed to load texture image!";

    m_labelIcon->setPixmap(pixmap.scaled(26, 26, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    refreshButtonAndStatus();
}

void VersionWidget::setCurrentVersion(bool isCurrentVersion)
{
    if (m_isCurrentVersion == isCurrentVersion)
        return;

    m_isCurrentVersion = isCurrentVersion;
    refreshButtonAndStatus();
}

void VersionWidget::refreshButtonAndStatus()
{
    if (!m_isInstalled)
    {
        m_statusLabel->setText("Not installed");
        m_button->setText("Install");
        m_button->setVariant(FireButton::Variant::Primary);
        m_button->setEnabled(!m_isDisabled);
        m_statusLabel->setTextColor(m_isDisabled ? theme::text3 : theme::text2);
        m_deleteButton->hide();
        return;
    }

    m_statusLabel->setText(m_isCurrentVersion ? "Installed · Default" : "Installed");
    m_statusLabel->setTextColor(m_isDisabled ? theme::text3 : theme::text2);
    m_button->setVariant(FireButton::Variant::Secondary);

    if (m_isCurrentVersion)
    {
        m_button->setText("Current");
        m_button->setEnabled(false);
    }
    else
    {
        m_button->setText("Set default");
        m_button->setEnabled(!m_isDisabled);
    }

    m_deleteButton->setVisible(!m_isDisabled);
}

void VersionWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QRectF cardRect = rect().adjusted(0.5, 0.5, -0.5, -0.5);
    QPainterPath path;
    path.addRoundedRect(cardRect, theme::radiusCard, theme::radiusCard);

    QColor fill;
    QColor stroke;

    if (m_isCurrentVersion || m_isSelected)
    {
        fill   = theme::withAlpha(theme::accent, 28);
        stroke = theme::withAlpha(theme::accent, 110);
    }
    else if (m_isHovered)
    {
        fill   = theme::surface1Hover;
        stroke = theme::borderHover;
    }
    else
    {
        fill   = theme::surface1;
        stroke = theme::border;
    }

    painter.fillPath(path, fill);
    painter.setPen(QPen(stroke, 1));
    painter.drawPath(path);

    // ── Channel pill (top-right) ─────────────────────────────────────────
    QString channel;
    QColor channelColor;
    if (m_tagName.startsWith("dev"))      { channel = "DEV";     channelColor = theme::dev; }
    else if (m_tagName.contains("rc") ||
             m_tagName.contains("beta"))  { channel = "PREVIEW"; channelColor = QColor(0xE0, 0xB2, 0x68); }
    else                                  { channel = "STABLE";  channelColor = theme::accent; }

    QFont pillFont = theme::uiFont(7, true);
    painter.setFont(pillFont);
    const QFontMetrics fm(pillFont);
    const int textW = fm.horizontalAdvance(channel);
    const int padX  = 8;
    const int pillH = 18;
    const QRectF pillRect(cardRect.right() - textW - padX * 2 - 12,
                          cardRect.top() + 12,
                          textW + padX * 2, pillH);
    QPainterPath pill;
    pill.addRoundedRect(pillRect, pillH / 2.0, pillH / 2.0);
    painter.fillPath(pill, theme::withAlpha(channelColor, 36));
    painter.setPen(QPen(theme::withAlpha(channelColor, 90), 1));
    painter.drawPath(pill);
    painter.setPen(channelColor);
    painter.drawText(pillRect, Qt::AlignCenter, channel);

    // ── Timeline dot (left edge of card, vertically centered) ───────────
    {
        const qreal cy = cardRect.center().y();
        const qreal dotR = m_isCurrentVersion ? 6.0 : 5.0;
        const QPointF c(cardRect.left() + 14, cy);

        if (m_isCurrentVersion)
        {
            painter.setBrush(QColor(theme::withAlpha(theme::accent, 60)));
            painter.setPen(Qt::NoPen);
            painter.drawEllipse(c, dotR + 4, dotR + 4);
        }

        QRadialGradient dotGrad(c, dotR);
        dotGrad.setColorAt(0.0, m_isCurrentVersion ? theme::accent : (m_isInstalled ? theme::surface2Hover : theme::surface1));
        dotGrad.setColorAt(1.0, m_isCurrentVersion ? theme::accentDeep : (m_isInstalled ? theme::surface2 : theme::surface1));
        painter.setBrush(dotGrad);
        painter.setPen(QPen(m_isCurrentVersion ? theme::withAlpha(theme::accent, 160) : theme::border, 1.4));
        painter.drawEllipse(c, dotR, dotR);
    }
}

const QString& VersionWidget::getDownloadLink() const
{
    return m_downloadLink;
}

const QString& VersionWidget::getTagName() const
{
    return m_tagName;
}

void VersionWidget::setSelected(bool isSelected)
{
    if(m_isSelected != isSelected)
    {
        m_isSelected = isSelected;
        update();
    }
}

bool VersionWidget::isSelected() const
{
    return m_isSelected;
}

void VersionWidget::enterEvent(QEnterEvent* event)
{
    Q_UNUSED(event);
    m_isHovered = true;
    update();
}

void VersionWidget::leaveEvent(QEvent* event)
{
    Q_UNUSED(event);
    m_isHovered = false;
    update();
}

void VersionWidget::mousePressEvent(QMouseEvent* event)
{
    if (m_isDisabled) return;

    Q_UNUSED(event);
    emit clicked(this);
}
