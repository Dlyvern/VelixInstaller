#ifndef PROJECTS_WIDGET_HPP
#define PROJECTS_WIDGET_HPP

#include <QWidget>
#include <QVBoxLayout>
#include <QVector>
#include <QSet>

#include "Config.hpp"
#include "ProjectWidget.hpp"

class ProjectsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ProjectsWidget(QWidget* parent = nullptr);

    ~ProjectsWidget() override = default;

protected:
    void paintEvent(QPaintEvent* event) override;

private slots:
    void onCreateProjectRequested();
    void onOpenProjectRequested();
    void onOpenProjectPath(const QString& projectPath);

private:
    QString resolveExecutableFromInstallPath(const QString& installPath) const;
    QString defaultProjectsRoot() const;
    bool createProject(const QString& parentDir, const QString& projectName, project::ProjectData& outProject);
    bool loadProjectFile(const QString& projectFilePath, project::ProjectData& outProject) const;
    bool upsertProjectInConfig(const project::ProjectData& projectData);
    void addProjectCard(const project::ProjectData& projectData);
    void loadProjectsFromConfig();

    Config m_config;
    QWidget* m_projectsContentWidget{nullptr};
    QVBoxLayout* m_projectsLayout{nullptr};
    QVector<ProjectWidget*> m_projectWidgets;
    QSet<QString> m_knownProjectPaths;
};

#endif //PROJECTS_WIDGET_HPP
