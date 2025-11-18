#include "ProjectsHandlerWidget.hpp"

ProjectsHandlerWidget::ProjectsHandlerWidget(QWidget* parent)
{

}

ProjectsHandlerWidget::~ProjectsHandlerWidget() = default;

void ProjectsHandlerWidget::onAddNewProject(const project::ProjectData& projectData)
{
    auto newProject = new ProjectWidget(projectData, this);

    m_projectWidgets.push_back(newProject);
}