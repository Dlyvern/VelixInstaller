#ifndef FIRE_BUTTON_HPP
#define FIRE_BUTTON_HPP

#include <QPushButton>
#include <QPainter>
#include <QPainterPath>
#include <QRandomGenerator>
#include <QEnterEvent>
#include <QMouseEvent>

class FireButton : public QPushButton
{
    Q_OBJECT
public:
    enum class Variant
    {
        Primary,
        Secondary
    };

    explicit FireButton(const QString& text, QWidget* parent = nullptr);
    explicit FireButton(const QString& text, Variant variant, QWidget* parent = nullptr);

    void setVariant(Variant variant);

protected:
    void paintEvent(QPaintEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    int m_cornerRadius = 8;
    Variant m_variant{Variant::Primary};
    bool m_isHovered{false};
    bool m_isPressed{false};
};



#endif //FIRE_BUTTON_HPP
