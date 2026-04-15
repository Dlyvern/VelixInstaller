#include "UpdateDownloadDialog.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainter>
#include <QPainterPath>
#include <QCloseEvent>
#include <QScreen>
#include <QApplication>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QScrollArea>
#include <QTextEdit>
#include <QDebug>

// ── Hints ─────────────────────────────────────────────────────────────────────
const QStringList UpdateDownloadDialog::s_hints = {
    "You can install multiple Velix Engine versions side by side.",
    "Use the Projects tab to open your Velix Engine projects quickly.",
    "Stable builds are recommended for production use.",
    "Dev/unstable builds include the latest features but may be less stable.",
    "Settings let you configure custom installation paths.",
    "Check the Documentation tab for guides and quick-start references.",
    "After the update, VelixInstaller restarts automatically.",
    "Your installed engine versions are preserved across updates.",
};

// ── SpinnerWidget ─────────────────────────────────────────────────────────────
SpinnerWidget::SpinnerWidget(int size, QWidget* parent)
    : QWidget(parent)
{
    setFixedSize(size, size);
    m_timer = new QTimer(this);
    m_timer->setInterval(16); // ~60 fps
    connect(m_timer, &QTimer::timeout, this, [this]{ m_angle = (m_angle + 4) % 360; update(); });
}

void SpinnerWidget::start()  { m_finished = false; m_timer->start(); }
void SpinnerWidget::stop()   { m_timer->stop(); update(); }

void SpinnerWidget::setFinished(bool ok)
{
    m_timer->stop();
    m_finished = true;
    m_success  = ok;
    update();
}

void SpinnerWidget::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const int   m    = 6;
    const QRectF r(m, m, width() - 2*m, height() - 2*m);
    const qreal  pen = 5.0;

    if (m_finished)
    {
        // Draw circle with check or X
        const QColor col = m_success ? QColor(80, 200, 120) : QColor(220, 70, 70);
        p.setPen(QPen(col, pen, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p.drawEllipse(r);

        p.setPen(QPen(col, pen, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        const QPointF c = r.center();
        const qreal   s = r.width() * 0.18;
        if (m_success)
        {
            p.drawLine(c + QPointF(-s, 0), c + QPointF(-s*0.2, s*0.9));
            p.drawLine(c + QPointF(-s*0.2, s*0.9), c + QPointF(s, -s*0.8));
        }
        else
        {
            p.drawLine(c + QPointF(-s, -s), c + QPointF(s, s));
            p.drawLine(c + QPointF(s, -s),  c + QPointF(-s, s));
        }
        return;
    }

    // Track (dim ring)
    p.setPen(QPen(QColor(55, 55, 55), pen, Qt::SolidLine, Qt::RoundCap));
    p.drawEllipse(r);

    // Spinning arc (orange gradient feel via two arcs)
    QConicalGradient grad(r.center(), -m_angle);
    grad.setColorAt(0.0, QColor(255, 106,   0));
    grad.setColorAt(0.3, QColor(255, 150,  40));
    grad.setColorAt(1.0, QColor(255, 106,   0, 0));

    p.setPen(QPen(QBrush(grad), pen, Qt::SolidLine, Qt::RoundCap));
    p.drawArc(r, (-m_angle) * 16, 270 * 16);
}

// ── UpdateDownloadDialog ──────────────────────────────────────────────────────
UpdateDownloadDialog::UpdateDownloadDialog(const QString& version,
                                           const QString& changelog,
                                           QWidget*        parent)
    : QDialog(parent, Qt::Window | Qt::FramelessWindowHint)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setModal(false);

    // ── Root container (gives us the dark rounded card) ───────────────────
    auto* root = new QWidget(this);
    root->setObjectName("dlgRoot");
    root->setStyleSheet(
        "#dlgRoot {"
        "  background: #1e1e1e;"
        "  border: 1px solid #333;"
        "  border-radius: 16px;"
        "}");

    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->addWidget(root);

    auto* mainLayout = new QVBoxLayout(root);
    mainLayout->setContentsMargins(28, 22, 28, 24);
    mainLayout->setSpacing(18);

    // ── Title bar row ─────────────────────────────────────────────────────
    {
        auto* row = new QHBoxLayout;
        row->setContentsMargins(0,0,0,0);

        auto* title = new VelixText(QString("Downloading  v%1").arg(version.startsWith('v') ? version.mid(1) : version), root);
        title->setPointSize(13);
        title->setTextColor(Qt::white);

        m_minimizeBtn = new FireButton("Minimize", FireButton::Variant::Secondary, root);
        m_minimizeBtn->setFixedSize(90, 30);
        connect(m_minimizeBtn, &QPushButton::clicked, this, &QDialog::hide);

        row->addWidget(title, 1);
        row->addWidget(m_minimizeBtn, 0, Qt::AlignVCenter);
        mainLayout->addLayout(row);
    }

    // ── Spinner ───────────────────────────────────────────────────────────
    m_spinner = new SpinnerWidget(72, root);
    m_spinner->start();
    mainLayout->addWidget(m_spinner, 0, Qt::AlignHCenter);

    // ── Status / speed ────────────────────────────────────────────────────
    m_statusLabel = new VelixText("Starting download\u2026", root);
    m_statusLabel->setPointSize(10);
    m_statusLabel->setTextColor(QColor(180, 180, 180));
    m_statusLabel->setAlignment(Qt::AlignCenter);

    m_speedLabel = new VelixText("", root);
    m_speedLabel->setPointSize(9);
    m_speedLabel->setTextColor(QColor(255, 106, 0));
    m_speedLabel->setAlignment(Qt::AlignCenter);

    mainLayout->addWidget(m_statusLabel);
    mainLayout->addWidget(m_speedLabel);

    // ── Progress bar ──────────────────────────────────────────────────────
    m_progressBar = new VelixProgressBar(root);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setFixedHeight(10);
    mainLayout->addWidget(m_progressBar);

    // ── Divider ───────────────────────────────────────────────────────────
    auto* divider = new QWidget(root);
    divider->setFixedHeight(1);
    divider->setStyleSheet("background: #333;");
    mainLayout->addWidget(divider);

    // ── Hint ─────────────────────────────────────────────────────────────
    {
        auto* hintRow = new QHBoxLayout;
        hintRow->setContentsMargins(0,0,0,0);
        hintRow->setSpacing(8);

        auto* bulb = new VelixText("\U0001F4A1", root);
        bulb->setPointSize(11);
        bulb->setFixedWidth(24);

        m_hintLabel = new VelixText(s_hints.first(), root);
        m_hintLabel->setPointSize(9);
        m_hintLabel->setTextColor(QColor(160, 160, 160));
        m_hintLabel->setWordWrap(true);

        hintRow->addWidget(bulb, 0, Qt::AlignTop);
        hintRow->addWidget(m_hintLabel, 1);
        mainLayout->addLayout(hintRow);
    }

    // ── Changelog ─────────────────────────────────────────────────────────
    if (!changelog.isEmpty())
    {
        auto* clHeader = new VelixText("What\u2019s new:", root);
        clHeader->setPointSize(9);
        clHeader->setTextColor(QColor(200, 200, 200));
        mainLayout->addWidget(clHeader);

        auto* clEdit = new QTextEdit(root);
        clEdit->setReadOnly(true);
        clEdit->setPlainText(changelog);
        clEdit->setFixedHeight(110);
        clEdit->setStyleSheet(
            "QTextEdit {"
            "  background: #141414;"
            "  color: #aaa;"
            "  border: 1px solid #2a2a2a;"
            "  border-radius: 8px;"
            "  padding: 6px;"
            "  font-size: 9pt;"
            "}");
        mainLayout->addWidget(clEdit);
    }

    // ── Hint cycling ─────────────────────────────────────────────────────
    m_hintTimer = new QTimer(this);
    m_hintTimer->setInterval(5000);
    connect(m_hintTimer, &QTimer::timeout, this, &UpdateDownloadDialog::nextHint);
    m_hintTimer->start();

    // ── Size & position ───────────────────────────────────────────────────
    setFixedWidth(480);
    adjustSize();

    if (const QScreen* scr = QApplication::primaryScreen())
    {
        const QRect sg = scr->availableGeometry();
        move(sg.center() - rect().center());
    }
}

void UpdateDownloadDialog::nextHint()
{
    m_hintIndex = (m_hintIndex + 1) % s_hints.size();

    // Fade out → swap text → fade in
    auto* fx  = new QGraphicsOpacityEffect(m_hintLabel);
    m_hintLabel->setGraphicsEffect(fx);

    auto* out = new QPropertyAnimation(fx, "opacity", this);
    out->setDuration(300);
    out->setStartValue(1.0);
    out->setEndValue(0.0);
    connect(out, &QPropertyAnimation::finished, this, [this, fx]
    {
        m_hintLabel->setText(s_hints[m_hintIndex]);
        auto* in = new QPropertyAnimation(fx, "opacity", this);
        in->setDuration(300);
        in->setStartValue(0.0);
        in->setEndValue(1.0);
        in->start(QAbstractAnimation::DeleteWhenStopped);
    });
    out->start(QAbstractAnimation::DeleteWhenStopped);
}

void UpdateDownloadDialog::onProgress(qint64 received, qint64 total)
{
    if (total <= 0) return;
    const int pct = static_cast<int>(received * 100 / total);
    m_progressBar->setValue(pct);

    const double mb = received / 1024.0 / 1024.0;
    const double mbTotal = total / 1024.0 / 1024.0;
    m_statusLabel->setText(QString("%1 MB  /  %2 MB").arg(mb, 0, 'f', 1).arg(mbTotal, 0, 'f', 1));
}

void UpdateDownloadDialog::onSpeed(double kbps)
{
    if (kbps >= 1024.0)
        m_speedLabel->setText(QString("%1 MB/s").arg(kbps / 1024.0, 0, 'f', 2));
    else
        m_speedLabel->setText(QString("%1 KB/s").arg(kbps, 0, 'f', 1));
}

void UpdateDownloadDialog::onFinished()
{
    m_done = true;
    m_hintTimer->stop();
    m_progressBar->setValue(100);
    m_statusLabel->setText("Installing update\u2026");
    m_speedLabel->setText("");
    m_spinner->setFinished(true);
    m_minimizeBtn->setEnabled(false);
}

void UpdateDownloadDialog::onError(const QString& error)
{
    m_done = true;
    m_hintTimer->stop();
    m_statusLabel->setText("Download failed: " + error);
    m_speedLabel->setText("");
    m_spinner->setFinished(false);
    m_minimizeBtn->setText("Close");
    m_minimizeBtn->setEnabled(true);
    disconnect(m_minimizeBtn, nullptr, nullptr, nullptr);
    connect(m_minimizeBtn, &QPushButton::clicked, this, &QDialog::accept);
}

void UpdateDownloadDialog::closeEvent(QCloseEvent* event)
{
    if (!m_done)
    {
        hide();
        event->ignore();
    }
    else
    {
        event->accept();
    }
}

// Drag-to-move (frameless window)
void UpdateDownloadDialog::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
        m_dragStart = event->globalPosition().toPoint() - frameGeometry().topLeft();
}

void UpdateDownloadDialog::mouseMoveEvent(QMouseEvent* event)
{
    if (event->buttons() & Qt::LeftButton)
        move(event->globalPosition().toPoint() - m_dragStart);
}
