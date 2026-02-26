#include "VersionWidget.hpp"
#include <QHBoxLayout>

#include <QPixmap>
#include <QIcon>

VersionWidget::VersionWidget(const QString& tagName, const QString& downloadLink, bool isInstalled, QWidget* parent) :
QWidget(parent), m_downloadLink(downloadLink), m_tagName(tagName)
{
    auto mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(12, 8, 12, 8);
    mainLayout->setSpacing(10);

    m_labelIcon = new QLabel(this);
    m_labelIcon->setFixedSize(34, 34);
    m_labelIcon->setAlignment(Qt::AlignCenter);

    mainLayout->addWidget(m_labelIcon);

    m_nameLabel = new VelixText(tagName, this);
    m_nameLabel->setPointSize(16);
    m_nameLabel->setTextColor(Qt::white);

    m_statusLabel = new VelixText(this);
    m_statusLabel->setPointSize(10);

    auto labelsWidget = new QWidget(this);
    auto labelsLayout = new QVBoxLayout(labelsWidget);
    labelsLayout->setContentsMargins(0, 0, 0, 0);
    labelsLayout->setSpacing(2);

    labelsLayout->addWidget(m_nameLabel);
    labelsLayout->addWidget(m_statusLabel);

    mainLayout->addWidget(labelsWidget, 1);

    m_button = new FireButton("", FireButton::Variant::Primary, this);
    m_button->setFixedWidth(120);

    connect(m_button, &QPushButton::clicked, this, [this]
    {
        if (m_isInstalled)
            emit chooseVersion(m_tagName);
        else
            emit installVersion(m_tagName, m_downloadLink);
    });

    mainLayout->addWidget(m_button);

    setBackgroundRole(QPalette::Base);
    setAutoFillBackground(true);

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

    m_labelIcon->setPixmap(pixmap.scaled(32, 32, Qt::KeepAspectRatio, Qt::SmoothTransformation));
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
        m_statusLabel->setTextColor(m_isDisabled ? QColor(136, 136, 136) : QColor(211, 211, 211));
        return;
    }

    m_statusLabel->setText(m_isCurrentVersion ? "Installed - Default" : "Installed");
    m_statusLabel->setTextColor(m_isDisabled ? QColor(136, 136, 136) : QColor(211, 211, 211));
    m_button->setVariant(FireButton::Variant::Secondary);

    if (m_isCurrentVersion)
    {
        m_button->setText("Chosen");
        m_button->setEnabled(false);
    }
    else
    {
        m_button->setText("Choose");
        m_button->setEnabled(!m_isDisabled);
    }
}

void VersionWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    QRect rect = this->rect();
    int radius = 5;
    
    QPainterPath path;
    path.addRoundedRect(rect, radius, radius);
    
    if (m_isSelected)
    {
        QLinearGradient gradient(rect.topLeft(), rect.bottomLeft());
        gradient.setColorAt(0, QColor(255, 120, 18));
        gradient.setColorAt(1, QColor(145, 60, 4));
        
        painter.fillPath(path, QBrush(gradient));
    }
    
    else if (m_isHovered)
        painter.fillPath(path, QColor(62, 62, 62, 220));
    else
        painter.fillPath(path, QColor(46, 46, 46, 220));
    
    const QColor borderColor = m_isSelected ? QColor(255, 150, 40, 130) : QColor(68, 68, 68);
    painter.setPen(QPen(borderColor, 1));
    painter.drawPath(path);
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
