#include "MainWindow.hpp"
#include <QHBoxLayout>

#include "Separator.hpp"

MainWindow::MainWindow(QWidget* widget) : QMainWindow(widget)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_NoSystemBackground);

    auto mainWidget = new QWidget(this);
    auto mainLayout = new QHBoxLayout(mainWidget);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    mainLayout->setSpacing(8);

    this->setCentralWidget(mainWidget);

    m_mainWidget = new MainWidget(mainWidget);
    m_leftWidget = new LeftWidget(mainWidget);

    m_leftWidget->setFixedWidth(220);
 
    mainLayout->addWidget(m_leftWidget);

    auto separatorWidget = new Separator();
    separatorWidget->setFixedWidth(2);

    mainLayout->addWidget(separatorWidget);

    mainLayout->addWidget(m_mainWidget);
    mainLayout->setStretch(2, 1);

    connect(m_leftWidget, &LeftWidget::tabWidgetChanged, m_mainWidget, &MainWidget::changeWidget);

    setMinimumSize(900, 620);
}

void MainWindow::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    painter.fillRect(rect(), QColor(18, 18, 18));
    
    QLinearGradient overlay(0, 0, 0, height());
    overlay.setColorAt(0, QColor(34, 34, 34, 75));
    overlay.setColorAt(1, QColor(14, 14, 14, 90));
    painter.fillRect(rect(), overlay);
    
    for (int y = 0; y < height(); y += 4)
    {
        for (int x = 0; x < width(); x += 4)
        {
            const int alpha = ((x + y) % 16 == 0) ? 10 : 4;
            painter.fillRect(x, y, 2, 2, QColor(30, 30, 30, alpha));
        }
    }
    
    QRadialGradient accent(width() / 2, height() + 140, width() * 0.7);
    accent.setColorAt(0, QColor(70, 28, 0, 14));
    accent.setColorAt(1, Qt::transparent);
    painter.fillRect(rect(), accent);
}
