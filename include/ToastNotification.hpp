#ifndef TOAST_NOTIFICATION_HPP
#define TOAST_NOTIFICATION_HPP

#include <QWidget>
#include <QTimer>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QPainter>
#include <QPainterPath>

enum class ToastType { Success, Warning, Error, Info };

class ToastNotification : public QWidget
{
    Q_OBJECT
public:
    // Shows a toast anchored to 'anchor' window. Auto-dismisses.
    static void show(const QString& message,
                     ToastType type   = ToastType::Info,
                     QWidget*  anchor = nullptr,
                     int       msec   = 3000);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    explicit ToastNotification(const QString& message, ToastType type,
                               QWidget* anchor, int msec);
    void animateIn();
    void startDismiss();

    QString   m_message;
    ToastType m_type;

    QGraphicsOpacityEffect* m_opacityEffect{nullptr};
    QPropertyAnimation*     m_animation{nullptr};
    QTimer*                 m_dismissTimer{nullptr};

    // Track active toasts per anchor so we can stack them
    static QVector<ToastNotification*>& activeToasts();
    void repositionStack(QWidget* anchor);
};

#endif //TOAST_NOTIFICATION_HPP
