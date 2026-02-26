#ifndef VERSION_WIDGET_HPP
#define VERSION_WIDGET_HPP

#include <QWidget>
#include <QMouseEvent>
#include <QPainter>
#include <QPen>
#include <QBrush>
#include <QLinearGradient>
#include <QPainterPath>
#include <QLabel>
#include "FireButton.hpp"
#include "widgets/VelixText.hpp"

class VersionWidget : public QWidget
{
    Q_OBJECT
public:
    VersionWidget(const QString& tagName, const QString& downloadLink, bool isInstalled, QWidget* parent = nullptr);

    [[nodiscard]] const QString& getTagName() const;
    [[nodiscard]] const QString& getDownloadLink() const;

    void setSelected(bool isSelected);

    [[nodiscard]] bool isSelected() const;

    void setInstalled(bool isInstalled);
    void setDisabled(bool isDisabled);
    void setCurrentVersion(bool isCurrentVersion);

    ~VersionWidget() override = default;
signals:
    void clicked(VersionWidget* widget);
    void installVersion(const QString& tagName, const QString& downloadLink);
    void chooseVersion(const QString& tagName);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    QString m_downloadLink;
    QString m_tagName;

    FireButton* m_button{nullptr};
    QLabel* m_labelIcon{nullptr};
    VelixText* m_nameLabel{nullptr};
    VelixText* m_statusLabel{nullptr};

    bool m_isInstalled{false};
    bool m_isCurrentVersion{false};

    bool m_isDisabled{false};

    bool m_isSelected{false};
    bool m_isHovered{false};

    void refreshButtonAndStatus();
};


#endif //VERSION_WIDGET_HPP
