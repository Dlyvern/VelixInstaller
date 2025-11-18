#include "VersionWidget.hpp"
#include <QHBoxLayout>

#include <QPixmap>
#include <QIcon>

VersionWidget::VersionWidget(const QString& tagName, const QString& downloadLink, bool isInstalled, QWidget* parent) :
QWidget(parent), m_downloadLink(downloadLink), m_tagName(tagName)
{
    auto mainLayout = new QHBoxLayout(this);

    m_labelIcon = new QLabel(this);

    mainLayout->addWidget(m_labelIcon);

    auto textLabel = new QLabel(tagName, this);
    textLabel->setStyleSheet( 
    "color: #D3D3D3;"
    "font-weight: bold;"
    "font-size: 18pt;"
    );

    QPalette palette = textLabel->palette();
    palette.setColor(QPalette::WindowText, Qt::white);
    textLabel->setPalette(palette);

    m_statusLabel = new QLabel(this);
    m_statusLabel->setStyleSheet( 
    "color: #D3D3D3;"
    "font-weight: bold;");

    auto labelsWidget = new QWidget(this);
    auto labelsLayout = new QVBoxLayout(labelsWidget);

    labelsLayout->addWidget(textLabel);
    labelsLayout->addWidget(m_statusLabel);

    mainLayout->addWidget(labelsWidget);

    m_button = new FireButton("", this);

    connect(m_button, &QPushButton::clicked, this, [this, tagName, downloadLink, isInstalled]
    {
        if (m_isInstalled)
            emit launchVersion(m_tagName);
        else
            emit installVersion(m_tagName, m_downloadLink);
    });

    mainLayout->addWidget(m_button);

    mainLayout->setContentsMargins(10, 5, 10, 5);
    
    setBackgroundRole(QPalette::Base);
    setAutoFillBackground(true);

    setInstalled(isInstalled);
}

void VersionWidget::setDisabled(bool isDisabled)
{
    m_isDisabled = isDisabled;
    m_button->setEnabled(!m_isDisabled);

    if (m_isDisabled)
        m_statusLabel->setStyleSheet("color: #888888; font-weight: bold;");
    else
        m_statusLabel->setStyleSheet("color: #D3D3D3; font-weight: bold;");

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

    m_statusLabel->setText(m_isInstalled ? "Installed" : "Not installed");
    m_button->setText(m_isInstalled ? "Launch" : "Install");
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
        QLinearGradient gradient(rect.topLeft(), rect.topRight());
        gradient.setColorAt(0, QColor(255, 128, 8));
        gradient.setColorAt(1, QColor(255, 200, 55));
        
        painter.fillPath(path, QBrush(gradient));
    } 
    
    else if (m_isHovered)
        painter.fillPath(path, QColor(62, 62, 62));
    else
        painter.fillPath(path, QColor(46, 46, 46));
    
    painter.setPen(QPen(QColor(68, 68, 68), 1));
    painter.drawPath(path);
}

const QString& VersionWidget::getDownloadLink() const
{
    return m_downloadLink;
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