#include "ProjectWidget.hpp"

#include <QHBoxLayout>
#include <QPushButton>

#include "widgets/VelixText.hpp"

ProjectWidget::ProjectWidget(const project::ProjectData& projectData, QWidget* parent) : QWidget(parent)
{
    auto mainLayout = new QHBoxLayout(this);

    auto projectNameLabel = new VelixText(QString(projectData.name.c_str()), this);

    mainLayout->addWidget(projectNameLabel);

    auto openButton = new QPushButton("Open", this);

    openButton->setStyleSheet(
        "background: gray"
    );

    mainLayout->addWidget(openButton);
}

void ProjectWidget::paintEvent(QPaintEvent *)
{

}

const project::ProjectData& ProjectWidget::getProjectData() const
{
    return m_projectData;
}

ProjectWidget::~ProjectWidget() = default;