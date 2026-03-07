#ifndef PROJECT_WIDGET_HPP
#define PROJECT_WIDGET_HPP

#include <QWidget>
#include <QPixmap>
#include <QLabel>
#include <QEnterEvent>
#include "ProjectData.hpp"

class FireButton;
class VelixText;

class ProjectWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ProjectWidget(const project::ProjectData& projectData, QWidget* parent = nullptr);

    [[nodiscard]] const project::ProjectData& getProjectData() const;

    ~ProjectWidget() override;

protected:
    void paintEvent(QPaintEvent *) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;

signals:
    void openRequested(const QString& projectPath);
    void removeRequested(const QString& projectFilePath);

private:
    project::ProjectData m_projectData;
    FireButton* m_openButton{nullptr};
    FireButton* m_removeButton{nullptr};
    VelixText* m_projectNameLabel{nullptr};
    VelixText* m_projectPathLabel{nullptr};
    QPixmap m_logo;
    bool m_isHovered{false};
};


#endif //PROJECT_WIDGET_HPP
