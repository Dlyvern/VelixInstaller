#include "FireLogoWidget.hpp"

#include <QPainter>
#include <QPainterPath>
#include <QRadialGradient>
#include <cmath>

static constexpr int   kLogoSize    = 56;  // logo image draw size
static constexpr int   kWidgetW     = 72;
static constexpr int   kWidgetH     = 88;  // extra height above logo for particles
static constexpr int   kMaxParticles = 30;
static constexpr int   kSpawnPerTick = 2;
static constexpr int   kTickMs      = 30;  // ~33 fps

FireLogoWidget::FireLogoWidget(QWidget* parent)
    : QWidget(parent)
    , m_rng(QRandomGenerator::global()->generate())
{
    m_logo.load("./resources/VelixFire.png");

    setFixedSize(kWidgetW, kWidgetH);
    setAttribute(Qt::WA_TransparentForMouseEvents);

    m_timer = new QTimer(this);
    m_timer->setInterval(kTickMs);
    connect(m_timer, &QTimer::timeout, this, &FireLogoWidget::tick);
    m_timer->start();
}

void FireLogoWidget::spawnParticle()
{
    if (m_particles.size() >= kMaxParticles)
        return;

    // Spawn near the center-bottom of the logo area
    const int logoTop = kWidgetH - kLogoSize - 4;
    const float cx = kWidgetW * 0.5f;
    const float cy = logoTop + kLogoSize * 0.65f;  // lower 2/3 of logo

    Particle p;
    p.x     = cx + (m_rng.bounded(kLogoSize / 2) - kLogoSize / 4.0f) * 0.8f;
    p.y     = cy + m_rng.bounded(12) - 6.0f;
    p.vx    = (m_rng.generateDouble() - 0.5f) * 0.7f;
    p.vy    = -(0.6f + m_rng.generateDouble() * 1.2f);
    p.life  = 1.0f;
    p.decay = 0.025f + static_cast<float>(m_rng.generateDouble()) * 0.02f;
    p.size  = 2.5f + static_cast<float>(m_rng.generateDouble()) * 2.0f;

    m_particles.append(p);
}

void FireLogoWidget::tick()
{
    for (int i = m_particles.size() - 1; i >= 0; --i)
    {
        auto& p = m_particles[i];
        p.x    += p.vx;
        p.y    += p.vy;
        p.vy   *= 0.97f;            // slight drag
        p.vx   *= 0.98f;
        p.life -= p.decay;

        if (p.life <= 0.0f)
            m_particles.removeAt(i);
    }

    for (int i = 0; i < kSpawnPerTick; ++i)
        spawnParticle();

    update();
}

void FireLogoWidget::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // ── Particles ────────────────────────────────────────────────────────────
    for (const auto& p : m_particles)
    {
        const float t = p.life;   // 1 = fresh, 0 = dead

        // Color: bright yellow-white at birth → orange → red → fades out
        int r, g, b, a;
        if (t > 0.7f)
        {
            const float f = (t - 0.7f) / 0.3f;
            r = 255;
            g = static_cast<int>(200 + f * 55);  // 200 → 255
            b = static_cast<int>(f * 80);         // 0 → 80  (slight white)
            a = static_cast<int>(220 * t);
        }
        else if (t > 0.35f)
        {
            const float f = (t - 0.35f) / 0.35f;
            r = 255;
            g = static_cast<int>(40 + f * 160);  // 40 → 200 orange
            b = 0;
            a = static_cast<int>(200 * t);
        }
        else
        {
            r = static_cast<int>(150 + 105 * t / 0.35f);
            g = static_cast<int>(20  +  20 * t / 0.35f);
            b = 0;
            a = static_cast<int>(180 * t);
        }

        const float sz = p.size * t * 0.9f + p.size * 0.1f;

        QRadialGradient grad(p.x, p.y, sz);
        grad.setColorAt(0.0, QColor(r, g, b, a));
        grad.setColorAt(0.5, QColor(r, qMax(0, g - 40), 0, a / 2));
        grad.setColorAt(1.0, Qt::transparent);

        painter.setPen(Qt::NoPen);
        painter.setBrush(grad);
        painter.drawEllipse(QRectF(p.x - sz, p.y - sz, sz * 2.0f, sz * 2.0f));
    }

    // ── Logo ─────────────────────────────────────────────────────────────────
    if (!m_logo.isNull())
    {
        const int logoLeft = (kWidgetW - kLogoSize) / 2;
        const int logoTop  = kWidgetH - kLogoSize - 4;
        painter.drawPixmap(QRect(logoLeft, logoTop, kLogoSize, kLogoSize), m_logo);
    }
}
