#include "ProjectsWidget.hpp"

#include <QVBoxLayout>
#include "widgets/VelixText.hpp"

#include "FireWidget.hpp"

ProjectsWidget::ProjectsWidget(QWidget* parent) : QWidget(parent)
{
    auto mainLayout  = new QVBoxLayout(this);

    FireWidget* container = new FireWidget(this);
    container->setCornerRadius(15);

    mainLayout->addWidget(container);

    auto textLabel = new VelixText{"Your projects", this};

    mainLayout->addWidget(textLabel, 0, Qt::AlignLeft);

    mainLayout->addStretch(10);
}
