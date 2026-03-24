#include "SplashOverlay.hpp"

#include <QPainter>
#include <QPainterPath>
#include <QLinearGradient>
#include <QRadialGradient>
#include <QFont>
#include <QFontMetrics>

SplashOverlay::SplashOverlay(QWidget* parent)
    : QWidget(parent)
    , m_rng(QRandomGenerator::global()->generate())
{
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setAttribute(Qt::WA_DeleteOnClose);

    m_logo.load("./resources/VelixFire.png");

    // Fill parent exactly
    if (parent)
        setGeometry(parent->rect());

    // Opacity effect for fade in/out
    m_opacityEffect = new QGraphicsOpacityEffect(this);
    m_opacityEffect->setOpacity(0.0);
    setGraphicsEffect(m_opacityEffect);

    // Particle timer
    m_particleTimer = new QTimer(this);
    m_particleTimer->setInterval(kTickMs);
    connect(m_particleTimer, &QTimer::timeout, this, &SplashOverlay::tick);

    startFadeIn();
}

void SplashOverlay::startFadeIn()
{
    m_animation = new QPropertyAnimation(m_opacityEffect, "opacity", this);
    m_animation->setDuration(300);
    m_animation->setStartValue(0.0);
    m_animation->setEndValue(1.0);
    m_animation->setEasingCurve(QEasingCurve::OutCubic);

    connect(m_animation, &QPropertyAnimation::finished, this, [this]
    {
        m_holding     = true;
        m_holdTicks   = 0;
        m_progress    = 0.0f;
        m_particleTimer->start();

        QTimer::singleShot(kHoldMs, this, &SplashOverlay::startFadeOut);
    });

    m_animation->start(QAbstractAnimation::DeleteWhenStopped);
}

void SplashOverlay::startFadeOut()
{
    m_fadingOut = true;
    m_particleTimer->stop();

    m_animation = new QPropertyAnimation(m_opacityEffect, "opacity", this);
    m_animation->setDuration(400);
    m_animation->setStartValue(1.0);
    m_animation->setEndValue(0.0);
    m_animation->setEasingCurve(QEasingCurve::InCubic);

    connect(m_animation, &QPropertyAnimation::finished, this, &SplashOverlay::close);

    m_animation->start(QAbstractAnimation::DeleteWhenStopped);
}

void SplashOverlay::spawnParticle()
{
    if (m_particles.size() >= kMaxParticles)
        return;

    // Spawn near center-bottom of the logo (must match drawLogo positioning)
    const int logoLeft = width() / 2 - kLogoSize - 16;
    const int logoTop  = height() / 2 - kLogoSize / 2;
    const float cx = logoLeft + kLogoSize * 0.5f;
    const float cy = logoTop  + kLogoSize * 0.70f;

    Particle p;
    p.x     = cx + (m_rng.bounded(kLogoSize / 2) - kLogoSize / 4.0f) * 0.9f;
    p.y     = cy + m_rng.bounded(16) - 8.0f;
    p.vx    = (m_rng.generateDouble() - 0.5f) * 0.9f;
    p.vy    = -(0.7f + m_rng.generateDouble() * 1.6f);
    p.life  = 1.0f;
    p.decay = 0.018f + static_cast<float>(m_rng.generateDouble()) * 0.018f;
    p.size  = 4.0f  + static_cast<float>(m_rng.generateDouble()) * 4.5f;

    m_particles.append(p);
}

void SplashOverlay::tick()
{
    // Advance particles
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

    // Advance progress bar
    if (m_holding && !m_fadingOut)
    {
        ++m_holdTicks;
        const float total = static_cast<float>(kHoldMs) / kTickMs;
        m_progress = qMin(1.0f, static_cast<float>(m_holdTicks) / total);
    }

    update();
}

void SplashOverlay::drawBackground(QPainter& p)
{
    const QRect r = rect();

    p.fillRect(r, QColor(18, 18, 18));

    QLinearGradient overlay(0, 0, 0, r.height());
    overlay.setColorAt(0, QColor(34, 34, 34, 75));
    overlay.setColorAt(1, QColor(14, 14, 14, 90));
    p.fillRect(r, overlay);

    for (int y = 0; y < r.height(); y += 4)
    {
        for (int x = 0; x < r.width(); x += 4)
        {
            const int alpha = ((x + y) % 16 == 0) ? 10 : 4;
            p.fillRect(x, y, 2, 2, QColor(30, 30, 30, alpha));
        }
    }

    QRadialGradient accent(r.width() / 2, r.height() + 140, r.width() * 0.7);
    accent.setColorAt(0, QColor(70, 28, 0, 18));
    accent.setColorAt(1, Qt::transparent);
    p.fillRect(r, accent);
}

void SplashOverlay::drawParticles(QPainter& p)
{
    for (const auto& part : m_particles)
    {
        const float t = part.life;

        int r, g, b, a;
        if (t > 0.7f)
        {
            const float f = (t - 0.7f) / 0.3f;
            r = 255;
            g = static_cast<int>(200 + f * 55);
            b = static_cast<int>(f * 80);
            a = static_cast<int>(220 * t);
        }
        else if (t > 0.35f)
        {
            const float f = (t - 0.35f) / 0.35f;
            r = 255;
            g = static_cast<int>(40 + f * 160);
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

        const float sz = part.size * t * 0.9f + part.size * 0.1f;

        QRadialGradient grad(part.x, part.y, sz);
        grad.setColorAt(0.0, QColor(r, g, b, a));
        grad.setColorAt(0.5, QColor(r, qMax(0, g - 40), 0, a / 2));
        grad.setColorAt(1.0, Qt::transparent);

        p.setPen(Qt::NoPen);
        p.setBrush(grad);
        p.drawEllipse(QRectF(part.x - sz, part.y - sz, sz * 2.0f, sz * 2.0f));
    }
}

void SplashOverlay::drawLogo(QPainter& p)
{
    if (m_logo.isNull())
        return;

    // Center block: logo on the left, titles on the right
    // Logo is centered vertically in the widget
    const int logoLeft = width() / 2 - kLogoSize - 16;
    const int logoTop  = height() / 2 - kLogoSize / 2;
    p.drawPixmap(QRect(logoLeft, logoTop, kLogoSize, kLogoSize), m_logo);
}

void SplashOverlay::drawTitles(QPainter& p)
{
    const int titleX  = width() / 2 + 16;
    const int centerY = height() / 2;

    QFont titleFont("Arial", 22, QFont::Bold);
    titleFont.setLetterSpacing(QFont::AbsoluteSpacing, 5);
    p.setFont(titleFont);
    p.setPen(QColor(255, 140, 30));

    QFontMetrics titleFm(titleFont);
    const QString titleStr = "VELIX";
    const int titleH = titleFm.height();
    p.drawText(titleX, centerY - 4, titleStr);

    QFont subFont("Arial", 10, QFont::Normal);
    subFont.setLetterSpacing(QFont::AbsoluteSpacing, 3);
    p.setFont(subFont);
    p.setPen(QColor(130, 130, 130));
    p.drawText(titleX, centerY - 4 + titleH + 4, "ENGINE PROJECT HUB");
}

void SplashOverlay::drawProgressBar(QPainter& p)
{
    const int barH     = 3;
    const int barY     = height() - 40;
    const int barLeft  = 48;
    const int barRight = width() - 48;
    const int barW     = barRight - barLeft;

    // Track
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(50, 50, 50));
    p.drawRoundedRect(QRect(barLeft, barY, barW, barH), 1, 1);

    if (m_progress <= 0.0f)
        return;

    const int filled = static_cast<int>(barW * m_progress);

    // Filled bar — orange gradient
    QLinearGradient barGrad(barLeft, 0, barLeft + filled, 0);
    barGrad.setColorAt(0.0, QColor(180, 80, 0));
    barGrad.setColorAt(1.0, QColor(255, 140, 30));
    p.setBrush(barGrad);
    p.drawRoundedRect(QRect(barLeft, barY, filled, barH), 1, 1);

    // Glow dot at leading edge
    if (filled > 2)
    {
        const float dotX = barLeft + filled;
        const float dotY = barY + barH * 0.5f;
        QRadialGradient glow(dotX, dotY, 6);
        glow.setColorAt(0.0, QColor(255, 180, 60, 220));
        glow.setColorAt(1.0, Qt::transparent);
        p.setBrush(glow);
        p.drawEllipse(QRectF(dotX - 6, dotY - 6, 12, 12));
    }
}

void SplashOverlay::drawVersionLabel(QPainter& p)
{
    QFont vFont("Arial", 8);
    p.setFont(vFont);
    p.setPen(QColor(70, 70, 70));

    const QFontMetrics fm(vFont);
    const QString ver = "v1.0.0";
    const int tw = fm.horizontalAdvance(ver);
    p.drawText(width() - tw - 16, height() - 14, ver);
}

void SplashOverlay::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    drawBackground(painter);
    drawParticles(painter);
    drawLogo(painter);
    drawTitles(painter);
    drawProgressBar(painter);
    drawVersionLabel(painter);
}
