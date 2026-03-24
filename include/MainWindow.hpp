#ifndef MAIN_WINDOW_HPP
#define MAIN_WINDOW_HPP

#include <QMainWindow>
#include <QPointer>

#include "MainWidget.hpp"
#include "LeftWidget.hpp"
#include "SplashOverlay.hpp"
#include <QPainter>
#include <QLinearGradient>

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget* widget = nullptr);

    ~MainWindow() = default;

protected:
    void paintEvent(QPaintEvent*) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    MainWidget*            m_mainWidget{nullptr};
    LeftWidget*            m_leftWidget{nullptr};
    QPointer<SplashOverlay> m_splash;
};


#endif //MAIN_WINDOW_HPP