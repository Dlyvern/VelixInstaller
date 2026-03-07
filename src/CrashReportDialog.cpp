#include "CrashReportDialog.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTextEdit>
#include <QLabel>
#include <QPainter>
#include <QPainterPath>
#include <QLinearGradient>
#include <QMouseEvent>
#include <QApplication>
#include <QScreen>

#include "widgets/VelixText.hpp"
#include "FireButton.hpp"

static constexpr int kHeaderHeight = 90;

CrashReportDialog::CrashReportDialog(const QString& crashInfo, int exitCode, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Velix Engine - Crash");
    setFixedSize(680, 500);
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);

    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    // ── Header placeholder (painted in paintEvent) ──────────────────────────
    auto* headerWidget = new QWidget(this);
    headerWidget->setFixedHeight(kHeaderHeight);
    headerWidget->setAttribute(Qt::WA_TransparentForMouseEvents);

    auto* headerLayout = new QVBoxLayout(headerWidget);
    headerLayout->setContentsMargins(28, 18, 28, 12);
    headerLayout->setSpacing(4);

    auto* titleLabel = new VelixText("Velix Engine crashed", headerWidget);
    titleLabel->setPointSize(16);
    titleLabel->setTextColor(Qt::white);

    auto* subtitleLabel = new VelixText(
        QString("The engine process exited unexpectedly (code %1).").arg(exitCode),
        headerWidget
    );
    subtitleLabel->setPointSize(9);
    subtitleLabel->setBold(false);
    subtitleLabel->setTextColor(QColor(255, 180, 170));

    headerLayout->addWidget(titleLabel);
    headerLayout->addWidget(subtitleLabel);
    headerLayout->addStretch(1);

    rootLayout->addWidget(headerWidget);

    // ── Body ─────────────────────────────────────────────────────────────────
    auto* bodyWidget = new QWidget(this);
    auto* bodyLayout = new QVBoxLayout(bodyWidget);
    bodyLayout->setContentsMargins(24, 16, 24, 20);
    bodyLayout->setSpacing(10);

    auto* detailsLabel = new VelixText("Output / Error Details", bodyWidget);
    detailsLabel->setPointSize(9);
    detailsLabel->setTextColor(QColor(160, 160, 160));
    bodyLayout->addWidget(detailsLabel);

    auto* errorView = new QTextEdit(bodyWidget);
    errorView->setReadOnly(true);
    errorView->setPlainText(crashInfo.isEmpty() ? "(No output captured)" : crashInfo);
    errorView->setStyleSheet(
        "QTextEdit {"
        "  background-color: #0d0d0d;"
        "  color: #d8d8d8;"
        "  border: 1px solid #2e2e2e;"
        "  border-radius: 6px;"
        "  font-family: 'Consolas', 'Courier New', monospace;"
        "  font-size: 11px;"
        "  padding: 8px;"
        "}"
        "QScrollBar:vertical {"
        "  background: #1a1a1a;"
        "  width: 8px;"
        "  border-radius: 4px;"
        "}"
        "QScrollBar::handle:vertical {"
        "  background: #444;"
        "  border-radius: 4px;"
        "  min-height: 20px;"
        "}"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }"
    );
    bodyLayout->addWidget(errorView, 1);

    auto* buttonsLayout = new QHBoxLayout();
    buttonsLayout->setSpacing(10);
    buttonsLayout->addStretch(1);

    auto* closeButton = new FireButton("Close", FireButton::Variant::Secondary, bodyWidget);
    closeButton->setFixedWidth(120);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);
    buttonsLayout->addWidget(closeButton);

    bodyLayout->addLayout(buttonsLayout);

    rootLayout->addWidget(bodyWidget, 1);

    // Center on screen
    if (const QScreen* screen = QApplication::primaryScreen())
    {
        const QRect sg = screen->availableGeometry();
        move(sg.center() - rect().center());
    }
}

void CrashReportDialog::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const QRectF bounds = rect().adjusted(1, 1, -1, -1);

    // Outer rounded card
    QPainterPath outerPath;
    outerPath.addRoundedRect(bounds, 10, 10);

    QLinearGradient bg(bounds.topLeft(), bounds.bottomLeft());
    bg.setColorAt(0.0, QColor(20, 20, 20, 252));
    bg.setColorAt(1.0, QColor(13, 13, 13, 252));
    painter.fillPath(outerPath, bg);

    // Red header band
    QPainterPath headerPath;
    headerPath.addRoundedRect(QRectF(bounds.left(), bounds.top(), bounds.width(), kHeaderHeight), 10, 10);
    // Make bottom of header flat by unioning with a rect covering the lower portion
    QPainterPath flatBottom;
    flatBottom.addRect(QRectF(bounds.left(), bounds.top() + 10, bounds.width(), kHeaderHeight - 10));
    headerPath = headerPath.united(flatBottom);

    QLinearGradient headerGrad(0, 0, 0, kHeaderHeight);
    headerGrad.setColorAt(0.0, QColor(165, 18, 18));
    headerGrad.setColorAt(1.0, QColor(110, 8, 8));
    painter.fillPath(headerPath, headerGrad);

    // Subtle orange accent line below header
    painter.setPen(QPen(QColor(220, 60, 20, 140), 1));
    painter.drawLine(
        QPointF(bounds.left() + 1, bounds.top() + kHeaderHeight),
        QPointF(bounds.right() - 1, bounds.top() + kHeaderHeight)
    );

    // Outer border
    painter.setPen(QPen(QColor(70, 70, 70), 1));
    painter.drawPath(outerPath);
}

void CrashReportDialog::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && event->pos().y() < kHeaderHeight)
    {
        m_dragging = true;
        m_dragOffset = event->globalPosition().toPoint() - frameGeometry().topLeft();
    }
    QDialog::mousePressEvent(event);
}

void CrashReportDialog::mouseMoveEvent(QMouseEvent* event)
{
    if (m_dragging)
        move(event->globalPosition().toPoint() - m_dragOffset);
    QDialog::mouseMoveEvent(event);
}
