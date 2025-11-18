#ifndef PROJECTS_HANDLER_WIDGET_HPP
#define PROJECTS_HANDLER_WIDGET_HPP

#include <QWidget>
#include <QVector>

#include "ProjectWidget.hpp"

class ProjectsHandlerWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ProjectsHandlerWidget(QWidget* parent = nullptr);

    ~ProjectsHandlerWidget() override;

public slots:
    void onAddNewProject(const project::ProjectData& projectData);

private:
    ProjectWidget* m_currentProjectWidget{nullptr};
    QVector<ProjectWidget*> m_projectWidgets;
};

#endif //PROJECTS_HANDLER_WIDGET_HPP