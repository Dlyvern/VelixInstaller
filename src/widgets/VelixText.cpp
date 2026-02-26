#include "widgets/VelixText.hpp"
#include <QPalette>
#include <QFont>

VelixText::VelixText(QWidget* parent) : QLabel(parent)
{
    initStyle();
}

VelixText::VelixText(const QString& text, QWidget* parent) : QLabel(text, parent)
{
    initStyle();
}

void VelixText::setTextColor(const QColor& color)
{
    QPalette palette = this->palette();
    palette.setColor(QPalette::WindowText, color);
    this->setPalette(palette);
}

void VelixText::setBold(bool isBold)
{
    QFont currentFont = this->font();
    currentFont.setBold(isBold);
    this->setFont(currentFont);
}

void VelixText::setPointSize(int pointSize)
{
    QFont currentFont = this->font();
    currentFont.setPointSize(pointSize);
    this->setFont(currentFont);
}

void VelixText::initStyle()
{
    setBold(true);
    setTextColor(QColor("#D3D3D3"));
}

VelixText::~VelixText() = default;
