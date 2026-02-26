#ifndef LEFT_WIDGET_HPP
#define LEFT_WIDGET_HPP

#include "TabWidget.hpp"

#include <QVector>
#include <QVBoxLayout>

class LeftWidget : public QWidget
{
    Q_OBJECT
public:
    explicit LeftWidget(QWidget* parent = nullptr);

    ~LeftWidget() override = default;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QVector<TabWidget*> m_tabs;

    void addTab(const QString& tabName, const QString& iconPath, QWidget* parent = nullptr);

    QVBoxLayout* m_mainLayout{nullptr};

signals:
    void tabWidgetChanged(const QString& tabName);
};


#endif //LEFT_WIDGET_HPP
