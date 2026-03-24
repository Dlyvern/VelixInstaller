#ifndef SPLASH_OVERLAY_HPP
#define SPLASH_OVERLAY_HPP

#include <QWidget>
#include <QPixmap>
#include <QTimer>
#include <QVector>
#include <QRandomGenerator>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>

class SplashOverlay : public QWidget
{
    Q_OBJECT
public:
    explicit SplashOverlay(QWidget* parent = nullptr);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    struct Particle
    {
        float x, y, vx, vy, life, decay, size;
    };

    void spawnParticle();
    void tick();
    void startFadeIn();
    void startFadeOut();

    void drawBackground(QPainter& p);
    void drawParticles(QPainter& p);
    void drawLogo(QPainter& p);
    void drawTitles(QPainter& p);
    void drawProgressBar(QPainter& p);
    void drawVersionLabel(QPainter& p);

    QPixmap                 m_logo;
    QVector<Particle>       m_particles;
    QRandomGenerator        m_rng;
    QTimer*                 m_particleTimer{nullptr};
    QGraphicsOpacityEffect* m_opacityEffect{nullptr};
    QPropertyAnimation*     m_animation{nullptr};

    float m_progress{0.0f};   // 0 → 1 during hold phase
    int   m_holdTicks{0};     // counts ticks since hold began
    bool  m_holding{false};
    bool  m_fadingOut{false};

    static constexpr int kLogoSize      = 96;
    static constexpr int kMaxParticles  = 80;
    static constexpr int kSpawnPerTick  = 3;
    static constexpr int kTickMs        = 30;
    static constexpr int kHoldMs        = 1900;
};

#endif // SPLASH_OVERLAY_HPP
