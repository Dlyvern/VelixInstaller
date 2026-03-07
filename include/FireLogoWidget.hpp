#ifndef FIRE_LOGO_WIDGET_HPP
#define FIRE_LOGO_WIDGET_HPP

#include <QWidget>
#include <QPixmap>
#include <QTimer>
#include <QVector>
#include <QRandomGenerator>

class FireLogoWidget : public QWidget
{
    Q_OBJECT
public:
    explicit FireLogoWidget(QWidget* parent = nullptr);
    ~FireLogoWidget() override = default;

protected:
    void paintEvent(QPaintEvent* event) override;

private slots:
    void tick();

private:
    struct Particle
    {
        float x, y;
        float vx, vy;
        float life;   // 1.0 = fresh, 0.0 = dead
        float decay;
        float size;
    };

    void spawnParticle();

    QPixmap             m_logo;
    QVector<Particle>   m_particles;
    QTimer*             m_timer{nullptr};
    QRandomGenerator    m_rng;
};

#endif //FIRE_LOGO_WIDGET_HPP
