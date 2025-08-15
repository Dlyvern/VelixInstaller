#ifndef MAIN_WIDGET_HPP
#define MAIN_WIDGET_HPP

#include <QWidget>


class MainWidget : public QWidget
{
    Q_OBJECT
public:
    explicit MainWidget(QWidget* widget = nullptr);

    ~MainWidget() override = default;
};

#endif //MAIN_WIDGET_HPP