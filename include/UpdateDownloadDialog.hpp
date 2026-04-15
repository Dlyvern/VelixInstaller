#ifndef UPDATE_DOWNLOAD_DIALOG_HPP
#define UPDATE_DOWNLOAD_DIALOG_HPP

#include <QDialog>
#include <QTimer>
#include <QPoint>
#include <QMouseEvent>
#include <QPixmap>
#include <QVector>
#include <QPropertyAnimation>

#include "widgets/VelixText.hpp"
#include "widgets/VelixProgressBar.hpp"
#include "FireButton.hpp"

// ── Slideshow ─────────────────────────────────────────────────────────────────
class SlideshowWidget : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(qreal blendAlpha READ blendAlpha WRITE setBlendAlpha)
public:
    explicit SlideshowWidget(QWidget* parent = nullptr);

    void loadFromDir(const QString& dir);
    bool hasImages() const { return !m_images.isEmpty(); }

    qreal blendAlpha() const        { return m_blendAlpha; }
    void  setBlendAlpha(qreal v)    { m_blendAlpha = v; update(); }

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void advance();

    QVector<QPixmap>    m_images;
    int                 m_current{0};
    int                 m_next{0};
    qreal               m_blendAlpha{1.0};

    QTimer*             m_slideTimer{nullptr};
    QPropertyAnimation* m_fadeAnim{nullptr};
};

// ── Spinning arc loader ───────────────────────────────────────────────────────
class SpinnerWidget : public QWidget
{
    Q_OBJECT
public:
    explicit SpinnerWidget(int size = 80, QWidget* parent = nullptr);

    void start();
    void stop();
    void setFinished(bool ok);      // switches to a checkmark or X

protected:
    void paintEvent(QPaintEvent*) override;

private:
    QTimer* m_timer{nullptr};
    int     m_angle{0};
    bool    m_finished{false};
    bool    m_success{false};
};

// ── Download dialog ───────────────────────────────────────────────────────────
class UpdateDownloadDialog : public QDialog
{
    Q_OBJECT
public:
    explicit UpdateDownloadDialog(const QString& version,
                                  const QString& changelog,
                                  QWidget*        parent = nullptr);

public slots:
    void onProgress(qint64 received, qint64 total);
    void onSpeed(double kbps);
    void onFinished();
    void onError(const QString& error);

protected:
    void closeEvent(QCloseEvent* event) override;       // hide instead of close
    void mousePressEvent(QMouseEvent* event) override;  // drag support
    void mouseMoveEvent(QMouseEvent* event) override;

private:
    void nextHint();

    SlideshowWidget*  m_slideshow{nullptr};
    VelixProgressBar* m_progressBar{nullptr};
    VelixText*        m_statusLabel{nullptr};
    VelixText*        m_hintLabel{nullptr};
    FireButton*       m_minimizeBtn{nullptr};

    QTimer* m_hintTimer{nullptr};
    int     m_hintIndex{0};
    bool    m_done{false};

    QPoint  m_dragStart;

    static const QStringList s_hints;
};

#endif // UPDATE_DOWNLOAD_DIALOG_HPP
