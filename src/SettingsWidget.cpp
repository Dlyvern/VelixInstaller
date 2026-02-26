#include "SettingsWidget.hpp"

#include <QHBoxLayout>
#include <QPainter>
#include <QPainterPath>
#include <QFrame>
#include <QPalette>
#include <QFileInfo>
#include <QAbstractItemView>

SettingsWidget::SettingsWidget(QWidget* parent) : QWidget(parent)
{
    setAttribute(Qt::WA_StyledBackground, true);

    m_config.load();

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    mainLayout->setSpacing(12);

    auto* titleLabel = new VelixText("Settings", this);
    titleLabel->setPointSize(14);
    titleLabel->setTextColor(Qt::white);
    mainLayout->addWidget(titleLabel, 0, Qt::AlignLeft);

    auto* panelFrame = new QFrame(this);
    panelFrame->setFrameShape(QFrame::NoFrame);
    panelFrame->setAutoFillBackground(true);
    QPalette panelPalette = panelFrame->palette();
    panelPalette.setColor(QPalette::Window, QColor(26, 26, 26, 235));
    panelFrame->setPalette(panelPalette);

    auto* panelLayout = new QVBoxLayout(panelFrame);
    panelLayout->setContentsMargins(14, 14, 14, 14);
    panelLayout->setSpacing(8);

    auto* rowLayout = new QHBoxLayout();
    rowLayout->setSpacing(10);

    auto* fieldLabel = new VelixText("Default Velix version", panelFrame);
    fieldLabel->setPointSize(10);
    fieldLabel->setTextColor(QColor(224, 224, 224));
    rowLayout->addWidget(fieldLabel);

    m_versionCombo = new QComboBox(panelFrame);
    m_versionCombo->setMinimumWidth(340);
    m_versionCombo->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    m_versionCombo->setAutoFillBackground(true);

    QPalette comboPalette = m_versionCombo->palette();
    comboPalette.setColor(QPalette::Base, QColor(42, 42, 42));
    comboPalette.setColor(QPalette::Button, QColor(52, 52, 52));
    comboPalette.setColor(QPalette::Text, QColor(235, 235, 235));
    comboPalette.setColor(QPalette::ButtonText, QColor(235, 235, 235));
    comboPalette.setColor(QPalette::WindowText, QColor(235, 235, 235));
    m_versionCombo->setPalette(comboPalette);
    if (m_versionCombo->view())
        m_versionCombo->view()->setPalette(comboPalette);

    connect(m_versionCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, &SettingsWidget::onVersionSelectionChanged);

    rowLayout->addWidget(m_versionCombo, 1);

    panelLayout->addLayout(rowLayout);

    m_statusLabel = new VelixText(panelFrame);
    m_statusLabel->setPointSize(9);
    m_statusLabel->setBold(false);
    m_statusLabel->setTextColor(QColor(168, 168, 168));
    panelLayout->addWidget(m_statusLabel, 0, Qt::AlignLeft);

    mainLayout->addWidget(panelFrame);
    mainLayout->addStretch(1);

    reloadInstalledVersions();
}

void SettingsWidget::reloadInstalledVersions()
{
    m_config.load();

    m_isReloading = true;
    m_versionCombo->clear();

    const auto& config = m_config.getConfig();
    const std::string currentVersion = config.contains("current_version") && config["current_version"].is_string()
        ? config["current_version"].get<std::string>()
        : std::string{};

    int selectedIndex = -1;

    if (config.contains("installed_versions") && config["installed_versions"].is_array())
    {
        int index = 0;
        for (const auto& item : config["installed_versions"])
        {
            if (!item.is_object() || !item.contains("version") || !item.contains("path"))
                continue;

            if (!item["version"].is_string() || !item["path"].is_string())
                continue;

            const QString version = QString::fromStdString(item["version"].get<std::string>());
            const QString path = QString::fromStdString(item["path"].get<std::string>());
            const bool exists = QFileInfo::exists(path);

            QString label = QString("%1  |  %2").arg(version, path);
            if (!exists)
                label += " (missing)";

            m_versionCombo->addItem(label, version);

            if (version.toStdString() == currentVersion)
                selectedIndex = index;

            ++index;
        }
    }

    if (m_versionCombo->count() == 0)
    {
        m_statusLabel->setText("No installed Velix versions found.");
        m_isReloading = false;
        return;
    }

    if (selectedIndex < 0)
        selectedIndex = 0;

    m_versionCombo->setCurrentIndex(selectedIndex);
    m_isReloading = false;

    onVersionSelectionChanged(selectedIndex);
}

void SettingsWidget::onVersionSelectionChanged(int index)
{
    if (m_isReloading || index < 0)
        return;

    const QString selectedVersion = m_versionCombo->itemData(index).toString();
    if (selectedVersion.isEmpty())
        return;

    auto& config = m_config.mutableConfig();
    config["current_version"] = selectedVersion.toStdString();

    if (m_config.save())
        m_statusLabel->setText(QString("Default version for opening projects: %1").arg(selectedVersion));
    else
        m_statusLabel->setText("Failed to save default version to config.");
}

void SettingsWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const QRectF bounds = rect().adjusted(1, 1, -1, -1);
    QPainterPath path;
    path.addRoundedRect(bounds, 12, 12);

    QLinearGradient gradient(bounds.topLeft(), bounds.bottomLeft());
    gradient.setColorAt(0.0, QColor(20, 20, 20, 240));
    gradient.setColorAt(1.0, QColor(12, 12, 12, 240));
    painter.fillPath(path, gradient);

    painter.setPen(QPen(QColor(62, 62, 62), 1));
    painter.drawPath(path);
}
