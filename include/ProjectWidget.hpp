#ifndef PROJECT_WIDGET_HPP
#define PROJECT_WIDGET_HPP

#include <QWidget>
#include "ProjectData.hpp"

class ProjectWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ProjectWidget(const project::ProjectData& projectData, QWidget* parent = nullptr);

    [[nodiscard]] const project::ProjectData& getProjectData() const;

    ~ProjectWidget() override;

protected:
    void paintEvent(QPaintEvent *) override;

private:
    project::ProjectData m_projectData;
};


#endif //PROJECT_WIDGET_HPP