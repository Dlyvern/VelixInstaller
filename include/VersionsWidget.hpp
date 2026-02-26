#ifndef VERSIONS_WIDGET_HPP
#define VERSIONS_WIDGET_HPP

#include <QWidget>
#include <QVBoxLayout>
#include <QVector>
#include <QHash>

#include "VersionWidget.hpp"

class VersionsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit VersionsWidget(QWidget* parent = nullptr);

    ~VersionsWidget() override = default;

    VersionWidget* getCurrentVersionWidget();

signals:
    void installVersion(const QString& tagName, const QString& downloadLink);
    void chooseVersion(const QString& tagName);

public slots:
    void addNewVersion(const QString& tagName, const QString& downloadLink, bool isInstalled);
    void setCurrentVersionTag(const QString& tagName);
    void setVersionInstalled(const QString& tagName, bool isInstalled);

private slots:
    void handleVersionClicked(VersionWidget* widget);

private:
    QVBoxLayout* m_mainLayout{nullptr};
    QVector<VersionWidget*> m_versionWidgets;
    QHash<QString, VersionWidget*> m_versionsByTag;
    VersionWidget* m_currentSelected{nullptr};
};

#endif //VERSIONS_WIDGET_HPP
