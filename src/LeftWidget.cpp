#include "LeftWidget.hpp"
#include <QPainter>
#include <QPainterPath>
#include <QHBoxLayout>
#include <QDebug>

#include "FireLogoWidget.hpp"
#include "widgets/VelixText.hpp"
#include "Theme.hpp"

LeftWidget::LeftWidget(QWidget* parent) : QWidget(parent)
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setSpacing(2);
    m_mainLayout->setContentsMargins(QMargins(12, 14, 12, 14));

    // ── Brand block: flame + "Velix / Installer 1.3.1" ───────────────────
    auto* brand = new QWidget(this);
    auto* brandLayout = new QHBoxLayout(brand);
    brandLayout->setContentsMargins(6, 4, 6, 14);
    brandLayout->setSpacing(10);

    auto* logoWidget = new FireLogoWidget(brand);
    brandLayout->addWidget(logoWidget, 0, Qt::AlignVCenter);

    auto* brandText = new QWidget(brand);
    auto* brandTextLayout = new QVBoxLayout(brandText);
    brandTextLayout->setContentsMargins(0, 0, 0, 0);
    brandTextLayout->setSpacing(1);

    auto* nameLbl = new VelixText("Velix", brandText);
    nameLbl->setPointSize(11);
    nameLbl->setBold(true);
    nameLbl->setTextColor(theme::text);

    auto* subLbl = new VelixText("Installer 1.3.1", brandText);
    subLbl->setPointSize(8);
    subLbl->setBold(false);
    subLbl->setTextColor(theme::text3);
    subLbl->setFont(theme::monoFont(8));

    brandTextLayout->addWidget(nameLbl);
    brandTextLayout->addWidget(subLbl);

    brandLayout->addWidget(brandText, 1, Qt::AlignVCenter);

    m_mainLayout->addWidget(brand);

    addTab("Home",          ":home",                     this);
    addTab("Projects",      "./resources/folder.png",    this);
    addTab("Samples",       "./resources/cloud.png",     this);
    addTab("Documentation", "./resources/document.png",  this);
    addTab("Settings",      "./resources/setting.png",   this);
    addTab("Installs",      "./resources/installs.png",  this);
    addTab("Updates",       "./resources/download.png",  this);

    m_updatesTab = m_tabs.last();
    m_tabs.first()->setActive(true);

    m_mainLayout->addStretch(10);
}

void LeftWidget::setUpdateBadge(bool hasBadge)
{
    if (m_updatesTab)
        m_updatesTab->setHasBadge(hasBadge);
}

void LeftWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Flat sidebar — slightly darker than the page background.
    painter.fillRect(rect(), theme::withAlpha(theme::bg0, 130));

    // 1px right border to separate from main content area.
    painter.setPen(QPen(theme::border, 1));
    painter.drawLine(width() - 1, 0, width() - 1, height());
}

void LeftWidget::addTab(const QString& tabName, const QString& iconPath, QWidget* parent)
{
    auto tab = new TabWidget(tabName, iconPath, parent);

    connect(tab, &TabWidget::clicked, this, [this, tab, tabName]
    {
        for (auto* tb : m_tabs)
            tb->setActive(false);

        tab->setActive(true);

        emit tabWidgetChanged(tabName);
    });

    m_tabs.push_back(tab);
    m_mainLayout->addWidget(tab);
}
