#ifndef FIRE_BUTTON_HPP
#define FIRE_BUTTON_HPP

#include <QPushButton>
#include <QPainter>
#include <QPainterPath>
#include <QRandomGenerator>

class FireButton : public QPushButton
{
    Q_OBJECT
public:
    explicit FireButton(const QString& text, QWidget* parent = nullptr);
    
protected:
    void paintEvent(QPaintEvent* event) override;
    
private:
    int m_cornerRadius = 8;
};



#endif //FIRE_BUTTON_HPP