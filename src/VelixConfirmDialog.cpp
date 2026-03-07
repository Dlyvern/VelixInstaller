#include "VelixConfirmDialog.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainter>
#include <QPainterPath>
#include <QLinearGradient>
#include <QApplication>
#include <QScreen>

#include "widgets/VelixText.hpp"
#include "FireButton.hpp"

VelixConfirmDialog::VelixConfirmDialog(const QString& title,
                                       const QString& message,
                                       const QString& confirmText,
                                       const QString& cancelText,
                                       QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(title);
    setFixedSize(440, 190);
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(20, 18, 20, 18);
    root->setSpacing(14);

    // ── Title ──────────────────────────────────────────────────────────────
    auto* titleLabel = new VelixText(title, this);
    titleLabel->setPointSize(12);
    titleLabel->setTextColor(Qt::white);
    root->addWidget(titleLabel);

    // ── Thin divider ───────────────────────────────────────────────────────
    auto* divider = new QWidget(this);
    divider->setFixedHeight(1);
    divider->setStyleSheet("background-color: #2e2e2e;");
    root->addWidget(divider);

    // ── Message ────────────────────────────────────────────────────────────
    auto* msgLabel = new VelixText(message, this);
    msgLabel->setPointSize(10);
    msgLabel->setBold(false);
    msgLabel->setTextColor(QColor(185, 185, 185));
    msgLabel->setWordWrap(true);
    root->addWidget(msgLabel, 1);

    // ── Buttons ────────────────────────────────────────────────────────────
    auto* btnRow = new QHBoxLayout();
    btnRow->setSpacing(10);
    btnRow->addStretch(1);

    auto* cancelBtn = new FireButton(cancelText, FireButton::Variant::Secondary, this);
    cancelBtn->setFixedWidth(105);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);

    auto* confirmBtn = new FireButton(confirmText, FireButton::Variant::Primary, this);
    confirmBtn->setFixedWidth(105);
    connect(confirmBtn, &QPushButton::clicked, this, &QDialog::accept);

    btnRow->addWidget(cancelBtn);
    btnRow->addWidget(confirmBtn);
    root->addLayout(btnRow);

    // Center on parent or screen
    if (parent)
    {
        const QRect pg = parent->geometry();
        move(pg.center() - rect().center());
    }
    else if (const QScreen* screen = QApplication::primaryScreen())
    {
        move(screen->availableGeometry().center() - rect().center());
    }
}

bool VelixConfirmDialog::ask(const QString& title, const QString& message,
                              const QString& confirmText, const QString& cancelText,
                              QWidget* parent)
{
    VelixConfirmDialog dlg(title, message, confirmText, cancelText, parent);
    return dlg.exec() == QDialog::Accepted;
}

void VelixConfirmDialog::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const QRectF bounds = rect().adjusted(1, 1, -1, -1);

    QPainterPath path;
    path.addRoundedRect(bounds, 10, 10);

    QLinearGradient bg(bounds.topLeft(), bounds.bottomLeft());
    bg.setColorAt(0.0, QColor(28, 28, 28, 254));
    bg.setColorAt(1.0, QColor(18, 18, 18, 254));
    painter.fillPath(path, bg);

    // Subtle orange top edge glow
    QPainterPath topEdge;
    topEdge.addRoundedRect(QRectF(bounds.left(), bounds.top(), bounds.width(), 3), 10, 10);
    QPainterPath topClip;
    topClip.addRect(QRectF(bounds.left(), bounds.top(), bounds.width(), 3));
    topEdge = topEdge.intersected(topClip);

    QLinearGradient topGlow(bounds.left(), 0, bounds.right(), 0);
    topGlow.setColorAt(0.0, Qt::transparent);
    topGlow.setColorAt(0.3, QColor(220, 70, 0, 180));
    topGlow.setColorAt(0.7, QColor(220, 70, 0, 180));
    topGlow.setColorAt(1.0, Qt::transparent);
    painter.fillPath(topEdge, topGlow);

    painter.setPen(QPen(QColor(55, 55, 55), 1));
    painter.drawPath(path);
}

void VelixConfirmDialog::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
    {
        m_dragging = true;
        m_dragOffset = event->globalPosition().toPoint() - frameGeometry().topLeft();
    }
    QDialog::mousePressEvent(event);
}

void VelixConfirmDialog::mouseMoveEvent(QMouseEvent* event)
{
    if (m_dragging)
        move(event->globalPosition().toPoint() - m_dragOffset);
    QDialog::mouseMoveEvent(event);
}

void VelixConfirmDialog::mouseReleaseEvent(QMouseEvent* event)
{
    m_dragging = false;
    QDialog::mouseReleaseEvent(event);
}
