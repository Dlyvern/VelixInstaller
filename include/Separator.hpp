#ifndef SEPARATOR_HPP
#define SEPARATOR_HPP

#include <QWidget>
#include <QPainter>
#include <QLinearGradient>

class Separator : public QWidget
{
    Q_OBJECT
public:
    explicit Separator(QWidget* parent = nullptr);

    ~Separator() = default;

protected:
    void paintEvent(QPaintEvent *) override;
};

#endif //SEPARATOR_HPP