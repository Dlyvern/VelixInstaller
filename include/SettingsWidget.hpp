#ifndef SETTINGS_WIDGET_HPP
#define SETTINGS_WIDGET_HPP

#include <QWidget>

class SettingsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit SettingsWidget(QWidget* parent = nullptr);

    ~SettingsWidget() override = default;
};

#endif //SETTINGS_WIDGET_HPP