#ifndef TAB_WIDGET_HPP
#define TAB_WIDGET_HPP

#include <QWidget>
#include <QString>
#include <QLabel>
#include <QMouseEvent>
#include <QEnterEvent>

#include "widgets/VelixText.hpp"

class TabWidget : public QWidget
{
    Q_OBJECT
public:
    TabWidget(const QString& tabName, const QString& iconPath, QWidget* parent = nullptr);

    ~TabWidget() override = default;

    void setActive(bool isActive);

protected:
    void paintEvent(QPaintEvent *) override;

    void mousePressEvent(QMouseEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;

    bool m_isActive{false};
    bool m_isHovered{false};

    int m_cornerRadius{10};
private:
    QLabel* m_labelIcon{nullptr};
    VelixText* m_textLabel{nullptr};
    QPixmap m_originalPixMap;

    void updateIconColor();

signals:
    void clicked();
};

#endif //TAB_WIDGET_HPP
