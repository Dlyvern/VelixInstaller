#ifndef DOCUMENTATION_WIDGET_HPP
#define DOCUMENTATION_WIDGET_HPP

#include <QWidget>

class DocumentationWidget : public QWidget
{
    Q_OBJECT
public:
    explicit DocumentationWidget(QWidget* parent = nullptr);

    ~DocumentationWidget() override = default;
};

#endif //DOCUMENTATION_WIDGET_HPP