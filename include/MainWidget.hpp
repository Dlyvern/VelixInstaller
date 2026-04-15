#ifndef MAIN_WIDGET_HPP
#define MAIN_WIDGET_HPP

#include <QWidget>
#include <QStackedWidget>

#include "InstallWidget.hpp"
#include "ProjectsWidget.hpp"
#include "SettingsWidget.hpp"
#include "DocumentationWidget.hpp"
#include "UpdateWidget.hpp"
#include "AppUpdateChecker.hpp"

class MainWidget : public QWidget
{
    Q_OBJECT
public:
    explicit MainWidget(QWidget* widget = nullptr);

    ~MainWidget() override = default;

public slots:
    void changeWidget(const QString& widgetName);

signals:
    void updateAvailable();

private:
    ProjectsWidget*      m_projectWidget{nullptr};
    InstallWidget*       m_installWidget{nullptr};
    SettingsWidget*      m_settingsWidget{nullptr};
    DocumentationWidget* m_documentationWidget{nullptr};
    UpdateWidget*        m_updateWidget{nullptr};
    AppUpdateChecker*    m_updateChecker{nullptr};

    QStackedWidget* m_stackedWidget{nullptr};
};

#endif //MAIN_WIDGET_HPP