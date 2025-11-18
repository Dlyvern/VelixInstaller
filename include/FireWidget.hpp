#ifndef FIRE_WIDGET_HPP
#define FIRE_WIDGET_HPP

#include <QWidget>
#include <QPainter>
#include <QPainterPath>
#include <QRandomGenerator>

class FireWidget : public QWidget 
{
    Q_OBJECT
public:
    explicit FireWidget(QWidget* parent = nullptr);
    
    void setCornerRadius(int radius);
    
protected:
    void paintEvent(QPaintEvent* event) override;
private:
    int m_cornerRadius = 10;
    QColor m_bgColor = QColor(255, 100, 0, 80);
};

#endif //FIRE_WIDGET_HPP