#ifndef SETTINGS_WIDGET_HPP
#define SETTINGS_WIDGET_HPP

#include <QWidget>
#include <QComboBox>
#include <QVBoxLayout>

#include "Config.hpp"
#include "widgets/VelixText.hpp"

class SettingsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit SettingsWidget(QWidget* parent = nullptr);

    ~SettingsWidget() override = default;

public slots:
    void reloadInstalledVersions();

protected:
    void paintEvent(QPaintEvent* event) override;

private slots:
    void onVersionSelectionChanged(int index);

private:
    Config m_config;
    QComboBox* m_versionCombo{nullptr};
    VelixText* m_statusLabel{nullptr};
    bool m_isReloading{false};
};

#endif //SETTINGS_WIDGET_HPP
