#include "ToastNotification.hpp"

#include <QHBoxLayout>
#include <QApplication>
#include <QScreen>
#include <QPainter>
#include <QPainterPath>
#include <QLinearGradient>

static constexpr int kToastWidth   = 320;
static constexpr int kToastHeight  = 52;
static constexpr int kAccentWidth  = 4;
static constexpr int kMarginRight  = 18;
static constexpr int kMarginBottom = 18;
static constexpr int kStackGap     = 10;

// ── helpers ──────────────────────────────────────────────────────────────────

static QColor accentForType(ToastType t)
{
    switch (t)
    {
    case ToastType::Success: return QColor(50,  200, 80);
    case ToastType::Warning: return QColor(255, 160, 20);
    case ToastType::Error:   return QColor(220, 50,  50);
    default:                 return QColor(60,  150, 255);
    }
}

static QString iconForType(ToastType t)
{
    switch (t)
    {
    case ToastType::Success: return "✓";
    case ToastType::Warning: return "⚠";
    case ToastType::Error:   return "✕";
    default:                 return "i";
    }
}

// ── static tracking ──────────────────────────────────────────────────────────

QVector<ToastNotification*>& ToastNotification::activeToasts()
{
    static QVector<ToastNotification*> s_toasts;
    return s_toasts;
}

// ── public static factory ────────────────────────────────────────────────────

void ToastNotification::show(const QString& message, ToastType type,
                              QWidget* anchor, int msec)
{
    auto* toast = new ToastNotification(message, type, anchor, msec);
    toast->animateIn();
}

// ── constructor ───────────────────────────────────────────────────────────────

ToastNotification::ToastNotification(const QString& message, ToastType type,
                                     QWidget* anchor, int msec)
    : QWidget(nullptr)
    , m_message(message)
    , m_type(type)
{
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_DeleteOnClose);
    setFixedSize(kToastWidth, kToastHeight);

    m_opacityEffect = new QGraphicsOpacityEffect(this);
    m_opacityEffect->setOpacity(0.0);
    setGraphicsEffect(m_opacityEffect);

    // Position: bottom-right of anchor window (or primary screen)
    activeToasts().append(this);
    repositionStack(anchor);

    // Dismiss timer
    m_dismissTimer = new QTimer(this);
    m_dismissTimer->setSingleShot(true);
    m_dismissTimer->setInterval(msec);
    connect(m_dismissTimer, &QTimer::timeout, this, &ToastNotification::startDismiss);

    connect(this, &QObject::destroyed, this, [this, anchor]()
    {
        activeToasts().removeOne(this);
        repositionStack(anchor);
    });
}

void ToastNotification::repositionStack(QWidget* anchor)
{
    QRect base;
    if (anchor && anchor->window())
        base = anchor->window()->geometry();
    else if (const QScreen* s = QApplication::primaryScreen())
        base = s->availableGeometry();

    const int rightEdge  = base.right()  - kMarginRight;
    const int bottomEdge = base.bottom() - kMarginBottom;

    // Lay out all active toasts from bottom up
    int stackY = bottomEdge - kToastHeight;
    for (int i = activeToasts().size() - 1; i >= 0; --i)
    {
        auto* t = activeToasts()[i];
        t->move(rightEdge - kToastWidth, stackY);
        stackY -= (kToastHeight + kStackGap);
    }
}

// ── animations ────────────────────────────────────────────────────────────────

void ToastNotification::animateIn()
{
    QWidget::show();

    m_animation = new QPropertyAnimation(m_opacityEffect, "opacity", this);
    m_animation->setDuration(220);
    m_animation->setStartValue(0.0);
    m_animation->setEndValue(1.0);
    m_animation->setEasingCurve(QEasingCurve::OutCubic);
    m_animation->start();

    connect(m_animation, &QPropertyAnimation::finished, this, [this]()
    {
        m_dismissTimer->start();
    });
}

void ToastNotification::startDismiss()
{
    m_animation = new QPropertyAnimation(m_opacityEffect, "opacity", this);
    m_animation->setDuration(300);
    m_animation->setStartValue(1.0);
    m_animation->setEndValue(0.0);
    m_animation->setEasingCurve(QEasingCurve::InCubic);
    m_animation->start();

    connect(m_animation, &QPropertyAnimation::finished, this, &QWidget::close);
}

// ── paint ─────────────────────────────────────────────────────────────────────

void ToastNotification::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const QRectF bounds = rect().adjusted(1, 1, -1, -1);
    QPainterPath path;
    path.addRoundedRect(bounds, 8, 8);

    // Background
    QLinearGradient bg(bounds.topLeft(), bounds.bottomLeft());
    bg.setColorAt(0.0, QColor(32, 32, 32, 245));
    bg.setColorAt(1.0, QColor(22, 22, 22, 245));
    painter.fillPath(path, bg);

    // Outer border
    painter.setPen(QPen(QColor(55, 55, 55), 1));
    painter.drawPath(path);

    // Colored left accent bar
    const QColor accent = accentForType(m_type);
    QPainterPath accentPath;
    accentPath.addRoundedRect(QRectF(bounds.left(), bounds.top(), kAccentWidth, bounds.height()), 8, 8);
    QPainterPath accentClip;
    accentClip.addRect(QRectF(bounds.left(), bounds.top(), kAccentWidth, bounds.height()));
    accentPath = accentPath.intersected(accentClip);
    painter.fillPath(accentPath, accent);

    // Icon circle
    const int iconX = static_cast<int>(bounds.left()) + kAccentWidth + 10;
    const int iconCY = height() / 2;
    const QRectF iconCircle(iconX, iconCY - 10, 20, 20);
    painter.setBrush(accent.darker(160));
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(iconCircle);

    painter.setPen(accent);
    QFont iconFont = painter.font();
    iconFont.setBold(true);
    iconFont.setPointSize(9);
    painter.setFont(iconFont);
    painter.drawText(iconCircle, Qt::AlignCenter, iconForType(m_type));

    // Message text
    const QRectF textRect(iconCircle.right() + 10, bounds.top(),
                          bounds.right() - iconCircle.right() - 16, bounds.height());
    painter.setPen(QColor(220, 220, 220));
    QFont msgFont = painter.font();
    msgFont.setBold(false);
    msgFont.setPointSize(10);
    painter.setFont(msgFont);
    painter.drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft | Qt::TextWordWrap, m_message);
}
