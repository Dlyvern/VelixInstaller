#ifndef VELIX_CONFIRM_DIALOG_HPP
#define VELIX_CONFIRM_DIALOG_HPP

#include <QDialog>
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>

class FireButton;
class VelixText;

class VelixConfirmDialog : public QDialog
{
    Q_OBJECT
public:
    explicit VelixConfirmDialog(const QString& title,
                                const QString& message,
                                const QString& confirmText = "Yes",
                                const QString& cancelText  = "Cancel",
                                QWidget* parent = nullptr);

    // Convenience: returns true if user clicked confirm
    static bool ask(const QString& title, const QString& message,
                    const QString& confirmText = "Yes",
                    const QString& cancelText  = "Cancel",
                    QWidget* parent = nullptr);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    QPoint m_dragOffset;
    bool   m_dragging{false};
};

#endif //VELIX_CONFIRM_DIALOG_HPP
