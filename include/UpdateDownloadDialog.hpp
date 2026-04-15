#ifndef UPDATE_DOWNLOAD_DIALOG_HPP
#define UPDATE_DOWNLOAD_DIALOG_HPP

#include <QDialog>
#include <QTimer>
#include <QPoint>
#include <QMouseEvent>

#include "widgets/VelixText.hpp"
#include "widgets/VelixProgressBar.hpp"
#include "FireButton.hpp"

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

    SpinnerWidget*    m_spinner{nullptr};
    VelixProgressBar* m_progressBar{nullptr};
    VelixText*        m_speedLabel{nullptr};
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
