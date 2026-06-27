#ifndef HOME_WIDGET_HPP
#define HOME_WIDGET_HPP

#include <QWidget>
#include <QVBoxLayout>
#include <QList>

class QLabel;
class FireLogoWidget;

class HomeWidget : public QWidget
{
    Q_OBJECT
public:
    explicit HomeWidget(QWidget* parent = nullptr);
    ~HomeWidget() override = default;

    void refresh();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

signals:
    void navigateRequested(const QString& tabName);

private:
    struct NewsEntry { QString title, date, summary, url, tag; };

    void buildHero(QVBoxLayout* parent);
    void buildBody(QVBoxLayout* parent);
    QWidget* buildStatTile(const QString& label, const QString& value,
                           const QString& sub, bool accent);
    QWidget* buildNewsRow(const NewsEntry& n, bool featured);
    QWidget* buildMiniProjectRow(const QString& name, const QString& engine,
                                  const QString& opened);
    QWidget* buildNewProjectRow();

    QList<NewsEntry> loadNews() const;

    QLabel* m_currentVersionValue{nullptr};
    QLabel* m_installedCountValue{nullptr};
    QLabel* m_projectsCountValue{nullptr};
    QLabel* m_currentVersionSub{nullptr};
    QLabel* m_installedCountSub{nullptr};
    QLabel* m_projectsCountSub{nullptr};

    QWidget* m_newsList{nullptr};
    QWidget* m_recentList{nullptr};
};

#endif // HOME_WIDGET_HPP
