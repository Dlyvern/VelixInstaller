#ifndef MAIN_WIDGET_HPP
#define MAIN_WIDGET_HPP

#include <QWidget>
#include <QStackedWidget>

#include "InstallWidget.hpp"
#include "ProjectsWidget.hpp"
#include "SettingsWidget.hpp"
#include "DocumentationWidget.hpp"

class MainWidget : public QWidget
{
    Q_OBJECT
public:
    explicit MainWidget(QWidget* widget = nullptr);

    ~MainWidget() override = default;


public slots:
    void changeWidget(const QString& widgetName);

private:
    ProjectsWidget* m_projectWidget{nullptr};
    InstallWidget* m_installWidget{nullptr};
    SettingsWidget* m_settingsWidget{nullptr};
    DocumentationWidget* m_documentationWidget{nullptr};

    QStackedWidget* m_stackedWidget{nullptr};
};

#endif //MAIN_WIDGET_HPP