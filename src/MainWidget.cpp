#include "MainWidget.hpp"

#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>

MainWidget::MainWidget(QWidget* widget) : QWidget(widget)
{
    auto mainLayout = new QVBoxLayout(this);
    auto logoLabel = new QLabel{"VelixInstaller", this};

    logoLabel->setStyleSheet( 
        "color: #D3D3D3;"
        "font-weight: bold;"
        "font-size: 18pt;"
    );

    mainLayout->addWidget(logoLabel, 0, Qt::AlignTop | Qt::AlignLeft);

}
