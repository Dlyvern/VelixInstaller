#ifndef VELIX_TEXT_HPP
#define VELIX_TEXT_HPP

#include <QLabel>

class VelixText : public QLabel
{
    Q_OBJECT
public:
    explicit VelixText(QWidget* parent = nullptr);
    VelixText(const QString& text, QWidget* parent = nullptr);

    ~VelixText() override;
};

#endif //VELIX_TEXT_HPP