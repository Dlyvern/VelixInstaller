#ifndef VELIX_PROGRESS_BAR_HPP
#define VELIX_PROGRESS_BAR_HPP

#include <QProgressBar>
#include <QPainter>
#include <QPainterPath>
#include <QLinearGradient>

class VelixProgressBar : public QProgressBar
{
public:
    explicit VelixProgressBar(QWidget* parent = nullptr);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    int m_cornerRadius{8};
};

#endif //VELIX_PROGRESS_BAR_HPP
