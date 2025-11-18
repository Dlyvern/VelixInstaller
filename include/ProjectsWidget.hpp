#ifndef PROJECTS_WIDGET_HPP
#define PROJECTS_WIDGET_HPP

#include <QWidget>
#include <QPushButton>
#include <QPainter>
#include <QPainterPath>
#include <QRandomGenerator>

class ProjectsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ProjectsWidget(QWidget* parent = nullptr);

    ~ProjectsWidget() override = default;
};

#endif //PROJECTS_WIDGET_HPP