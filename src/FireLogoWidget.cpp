#include "FireLogoWidget.hpp"

#include <QPainter>
#include <QPainterPath>
#include <QRadialGradient>
#include <QLinearGradient>
#include <cmath>

#include "Theme.hpp"

// Sized for the sidebar brand block (Velix · Installer 1.3.1).
static constexpr int   kFlameSize    = 28;
static constexpr int   kWidgetW      = 32;
static constexpr int   kWidgetH      = 36;
static constexpr int   kMaxParticles = 10;
static constexpr int   kSpawnPerTick = 1;
static constexpr int   kTickMs       = 50;

FireLogoWidget::FireLogoWidget(QWidget* parent)
    : QWidget(parent)
    , m_rng(QRandomGenerator::global()->generate())
{
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

    const float cx = kWidgetW * 0.5f;
    const float cy = kWidgetH * 0.30f;

    Particle p;
    p.x     = cx + (m_rng.bounded(8) - 4.0f);
    p.y     = cy + m_rng.bounded(6) - 3.0f;
    p.vx    = (m_rng.generateDouble() - 0.5f) * 0.35f;
    p.vy    = -(0.35f + m_rng.generateDouble() * 0.6f);
    p.life  = 1.0f;
    p.decay = 0.030f + static_cast<float>(m_rng.generateDouble()) * 0.020f;
    p.size  = 1.6f + static_cast<float>(m_rng.generateDouble()) * 1.4f;

    m_particles.append(p);
}

void FireLogoWidget::tick()
{
    for (int i = m_particles.size() - 1; i >= 0; --i)
    {
        auto& p = m_particles[i];
        p.x    += p.vx;
        p.y    += p.vy;
        p.vy   *= 0.97f;
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

    // ── Procedural flame body (mirrors the design's <Flame /> primitive) ──
    const float left = (kWidgetW - kFlameSize) * 0.5f;
    const float top  = (kWidgetH - kFlameSize) * 0.5f + 2.0f;

    // Outer petal as a path scaled to the 40x40 viewBox used in the prototype.
    auto petalAt = [left, top](qreal x, qreal y)
    {
        return QPointF(left + x * (kFlameSize / 40.0), top + y * (kFlameSize / 40.0));
    };

    QPainterPath petal;
    petal.moveTo(petalAt(20, 4));
    petal.cubicTo(petalAt(24, 10), petalAt(32, 14), petalAt(32, 24));
    petal.cubicTo(petalAt(32, 32), petalAt(26, 37), petalAt(20, 37));
    petal.cubicTo(petalAt(14, 37), petalAt(8, 32),  petalAt(8, 24));
    petal.cubicTo(petalAt(8, 16),  petalAt(16, 14), petalAt(20, 4));

    QLinearGradient body(petalAt(20, 4), petalAt(20, 37));
    body.setColorAt(0.0, theme::accent);
    body.setColorAt(1.0, theme::accentDeep);
    painter.setOpacity(0.92);
    painter.fillPath(petal, body);
    painter.setOpacity(1.0);

    // Hot core
    const QPointF coreCentre = petalAt(20, 26);
    const qreal coreRx = 6.5 * (kFlameSize / 40.0);
    const qreal coreRy = 9.0 * (kFlameSize / 40.0);
    QRadialGradient core(coreCentre, std::max(coreRx, coreRy));
    core.setColorAt(0.0, theme::accent);
    core.setColorAt(0.6, theme::withAlpha(theme::accent, 217));
    core.setColorAt(1.0, theme::withAlpha(theme::accentDeep, 50));
    painter.setBrush(core);
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(coreCentre, coreRx, coreRy);

    // Subtle inner highlight
    QPainterPath highlight;
    highlight.moveTo(petalAt(16, 12));
    highlight.cubicTo(petalAt(18, 16), petalAt(17, 22), petalAt(14, 24));
    painter.setPen(QPen(QColor(255, 255, 255, 90), 1.2));
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(highlight);

    // ── Tiny ember particles drifting up ─────────────────────────────────
    for (const auto& p : m_particles)
    {
        const float t  = p.life;
        const float sz = p.size * t * 0.9f + p.size * 0.1f;
        QColor c = theme::accentBright;
        c.setAlpha(static_cast<int>(180 * t));
        QRadialGradient grad(p.x, p.y, sz);
        grad.setColorAt(0.0, c);
        grad.setColorAt(1.0, Qt::transparent);
        painter.setPen(Qt::NoPen);
        painter.setBrush(grad);
        painter.drawEllipse(QRectF(p.x - sz, p.y - sz, sz * 2.0f, sz * 2.0f));
    }
}
