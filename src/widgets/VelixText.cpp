#include "widgets/VelixText.hpp"

VelixText::VelixText(QWidget* parent) : QLabel(parent)
{
    this->setStyleSheet( 
        "color: #D3D3D3;"
        "font-weight: bold;");
}

VelixText::VelixText(const QString& text, QWidget* parent) : QLabel(text, parent)
{
    this->setStyleSheet( 
        "color: #D3D3D3;"
        "font-weight: bold;");
}

VelixText::~VelixText() = default;
