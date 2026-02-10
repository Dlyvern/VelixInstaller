#ifndef MAIN_WINDOW_HPP
#define MAIN_WINDOW_HPP

#include <QMainWindow>
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

private:
};


#endif //MAIN_WINDOW_HPP