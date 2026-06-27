#include "MainWindow.hpp"
#include <QHBoxLayout>
#include <QResizeEvent>
#include <QRadialGradient>

#include "Separator.hpp"
#include "Config.hpp"
#include "Theme.hpp"

MainWindow::MainWindow(QWidget* widget) : QMainWindow(widget)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_NoSystemBackground);

    auto mainWidget = new QWidget(this);
    auto mainLayout = new QHBoxLayout(mainWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    this->setCentralWidget(mainWidget);

    m_mainWidget = new MainWidget(mainWidget);
    m_leftWidget = new LeftWidget(mainWidget);

    m_leftWidget->setFixedWidth(220);

    mainLayout->addWidget(m_leftWidget);
    mainLayout->addWidget(m_mainWidget);
    mainLayout->setStretch(1, 1);

    connect(m_leftWidget, &LeftWidget::tabWidgetChanged, m_mainWidget, &MainWidget::changeWidget);

    connect(m_mainWidget, &MainWidget::updateAvailable, this, [this]
    {
        m_leftWidget->setUpdateBadge(true);
        ToastNotification::show("New VelixInstaller update available!", ToastType::Info, this);
    });

    setMinimumSize(900, 620);

    Config splashCfg;
    splashCfg.load();
    const auto& splashJson = splashCfg.getConfig();
    const bool splashEnabled = !splashJson.contains("splash_enabled") || splashJson["splash_enabled"].get<bool>();

    if (splashEnabled)
    {
        m_splash = new SplashOverlay(mainWidget);
        m_splash->raise();
        m_splash->show();
    }
}

void MainWindow::resizeEvent(QResizeEvent* event)
{
    QMainWindow::resizeEvent(event);
    if (m_splash)
        m_splash->setGeometry(centralWidget()->rect());
}

void MainWindow::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Deep near-black base (matches design --bg-0 / page background).
    painter.fillRect(rect(), QColor(0x0A, 0x0A, 0x0C));

    // Subtle vertical surface gradient — adds quiet depth without noise.
    QLinearGradient surface(0, 0, 0, height());
    surface.setColorAt(0.0, theme::withAlpha(theme::bg1, 200));
    surface.setColorAt(1.0, theme::withAlpha(theme::bg0, 220));
    painter.fillRect(rect(), surface);

    // Two faint ember glows (deep prototype --ember-a / --ember-b),
    // bottom-left + top-right, very low alpha.
    QRadialGradient emberA(width() * 0.20, height() + 80, width() * 0.55);
    emberA.setColorAt(0.0, theme::withAlpha(theme::accent, 38));
    emberA.setColorAt(1.0, Qt::transparent);
    painter.fillRect(rect(), emberA);

    QRadialGradient emberB(width() * 0.92, -120, width() * 0.45);
    emberB.setColorAt(0.0, theme::withAlpha(theme::accentBright, 22));
    emberB.setColorAt(1.0, Qt::transparent);
    painter.fillRect(rect(), emberB);
}
