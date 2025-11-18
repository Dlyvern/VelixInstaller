#ifndef VERSIONS_WIDGET_HPP
#define VERSIONS_WIDGET_HPP

#include <QWidget>
#include <QVBoxLayout>
#include <QVector>

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
    void launchVersion(const QString& tagName);

public slots:
    void addNewVersion(const QString& tagName, const QString& downloadLink, bool isInstalled);

private slots:
    void handleVersionClicked(VersionWidget* widget);

private:
    QVBoxLayout* m_mainLayout{nullptr};
    QVector<VersionWidget*> m_versionWidgets;
    VersionWidget* m_currentSelected{nullptr};
};

#endif //VERSIONS_WIDGET_HPP