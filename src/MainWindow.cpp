#include "MainWindow.hpp"
#include <QDebug>
#include <QLabel>
#include <QRandomGenerator>
#include <QHBoxLayout>

MainWindow::MainWindow(QWidget* widget) : QMainWindow(widget)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_NoSystemBackground);

    auto mainWidget = new QWidget(this);
    auto mainLayout = new QHBoxLayout(mainWidget);

    this->setCentralWidget(mainWidget);
}

void MainWindow::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    painter.fillRect(rect(), QColor(18, 18, 18));
    
    QLinearGradient overlay(0, 0, 0, height());
    overlay.setColorAt(0, QColor(30, 30, 30, 80));
    overlay.setColorAt(1, QColor(10, 10, 10, 80));
    painter.fillRect(rect(), overlay);
    
    painter.setPen(Qt::NoPen);
    for (int i = 0; i < width(); i += 2) {
        for (int j = 0; j < height(); j += 2) {
            int dark = QRandomGenerator::global()->bounded(10, 20);
            painter.setBrush(QColor(dark, dark, dark, 3));
            painter.drawRect(i, j, 2, 2);
        }
    }
    
    QRadialGradient accent(width()/2, height() + 100, width() * 0.7);
    accent.setColorAt(0, QColor(50, 20, 0, 10));
    accent.setColorAt(1, Qt::transparent);
    painter.fillRect(rect(), accent);
}