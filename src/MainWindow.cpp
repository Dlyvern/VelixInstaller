#include "MainWindow.hpp"
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDebug>
#include <QLabel>
#include <QNetworkAccessManager>
#include <QRandomGenerator>

#include <QHBoxLayout>

#include "Separator.hpp"

void downloadFile(const QUrl &url, const QString &savePath)
{
    QNetworkAccessManager *manager = new QNetworkAccessManager();
    QNetworkRequest request(url);
    QNetworkReply *reply = manager->get(request);

    QObject::connect(reply, &QNetworkReply::finished, [=]() {
        if (reply->error() != QNetworkReply::NoError) {
            qDebug() << "Download error:" << reply->errorString();
        } else {
            QFile file(savePath);
            if (file.open(QIODevice::WriteOnly)) {
                file.write(reply->readAll());
                file.close();
                qDebug() << "File downloaded to:" << savePath;
            }
        }
        reply->deleteLater();
        manager->deleteLater();
    });
}

MainWindow::MainWindow(QWidget* widget) : QMainWindow(widget)
{
    // auto *manager = new QNetworkAccessManager(this);

    // QUrl url("https://github.com/Dlyvern/Velix/releases/download/0.0.0/Velix-linux.zip");
    // QNetworkRequest request(url);
    // QNetworkReply *reply = manager->get(request);

    // QObject::connect(reply, &QNetworkReply::downloadProgress, this,
    //     [](qint64 bytesReceived, qint64 bytesTotal) {
    //         qDebug() << "Progress:" << bytesReceived << "/" << bytesTotal;
    //     }
    // );

    // QObject::connect(reply, &QNetworkReply::finished, this, [reply]() {
    //     if (reply->error() != QNetworkReply::NoError) {
    //         qDebug() << "Download error:" << reply->errorString();
    //     } else {
    //         QFile file("Velix-linux.zip");
    //         if (file.open(QIODevice::WriteOnly)) {
    //             file.write(reply->readAll());
    //             file.close();
    //             qDebug() << "File downloaded to Velix-linux.zip";
    //         }
    //     }
    //     reply->deleteLater();
    // });

    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_NoSystemBackground);

    auto mainWidget = new QWidget(this);
    auto mainLayout = new QHBoxLayout(mainWidget);

    this->setCentralWidget(mainWidget);

    m_mainWidget = new MainWidget(mainWidget);
    m_leftWidget = new LeftWidget(mainWidget);

    m_leftWidget->setFixedWidth(200);
 
    mainLayout->addWidget(m_leftWidget);

    auto separatorWidget = new Separator();
    separatorWidget->setFixedWidth(3);

    mainLayout->addWidget(separatorWidget);

    mainLayout->addWidget(m_mainWidget);

    this->setFixedSize({800, 600});
}


void MainWindow::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // Base color - almost black
    painter.fillRect(rect(), QColor(18, 18, 18));
    
    // Subtle vertical gradient overlay
    QLinearGradient overlay(0, 0, 0, height());
    overlay.setColorAt(0, QColor(30, 30, 30, 80));
    overlay.setColorAt(1, QColor(10, 10, 10, 80));
    painter.fillRect(rect(), overlay);
    
    // Extremely subtle noise texture
    painter.setPen(Qt::NoPen);
    for (int i = 0; i < width(); i += 2) {
        for (int j = 0; j < height(); j += 2) {
            int dark = QRandomGenerator::global()->bounded(10, 20);
            painter.setBrush(QColor(dark, dark, dark, 3));  // Almost imperceptible
            painter.drawRect(i, j, 2, 2);
        }
    }
    
    // Minimal fiery accent
    QRadialGradient accent(width()/2, height() + 100, width() * 0.7);
    accent.setColorAt(0, QColor(50, 20, 0, 10));  // Barely visible
    accent.setColorAt(1, Qt::transparent);
    painter.fillRect(rect(), accent);
}