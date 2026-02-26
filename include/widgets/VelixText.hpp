#ifndef VELIX_TEXT_HPP
#define VELIX_TEXT_HPP

#include <QLabel>
#include <QColor>

class VelixText : public QLabel
{
    Q_OBJECT
public:
    explicit VelixText(QWidget* parent = nullptr);
    VelixText(const QString& text, QWidget* parent = nullptr);

    void setTextColor(const QColor& color);
    void setBold(bool isBold);
    void setPointSize(int pointSize);

    ~VelixText() override;

private:
    void initStyle();
};

#endif //VELIX_TEXT_HPP
