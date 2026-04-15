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

    void reloadThumbnail();

    static QString thumbnailPath(const std::string& projectDir);

    ~ProjectWidget() override;

protected:
    void paintEvent(QPaintEvent *) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;

signals:
    void openRequested(const QString& projectPath);
    void editRequested(const QString& projectPath, const QString& projectName);
    void removeRequested(const QString& projectFilePath);

private:
    void loadThumbnail();

    project::ProjectData m_projectData;
    FireButton* m_openButton{nullptr};
    FireButton* m_editButton{nullptr};
    FireButton* m_removeButton{nullptr};
    VelixText* m_projectNameLabel{nullptr};
    VelixText* m_projectPathLabel{nullptr};
    QPixmap m_logo;
    QPixmap m_thumbnail;
    QString m_diskUsage;
    bool m_isHovered{false};

    void calculateDiskUsage();
};


#endif //PROJECT_WIDGET_HPP
