#ifndef CRASH_REPORT_DIALOG_HPP
#define CRASH_REPORT_DIALOG_HPP

#include <QDialog>
#include <QTextEdit>
#include <QPainter>
#include <QPainterPath>

class FireButton;
class VelixText;

class CrashReportDialog : public QDialog
{
    Q_OBJECT
public:
    explicit CrashReportDialog(const QString& crashInfo, int exitCode, QWidget* parent = nullptr);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;

private:
    QPoint m_dragOffset;
    bool m_dragging{false};
};

#endif //CRASH_REPORT_DIALOG_HPP
