#ifndef DOCUMENTATION_WIDGET_HPP
#define DOCUMENTATION_WIDGET_HPP

#include <QWidget>
#include <QTextBrowser>
#include <QListWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QSplitter>
#include <QString>
#include <QStringList>
#include <QScrollArea>
#include <QLabel>
#include <QVBoxLayout>
#include <QNetworkAccessManager>
#include <QEvent>
#include <QMouseEvent>

class DocumentationWidget : public QWidget
{
    Q_OBJECT
public:
    explicit DocumentationWidget(QWidget* parent = nullptr);
    ~DocumentationWidget() override = default;

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    struct DocPage
    {
        QString title;
        QString filename;   // empty = section header
    };

    struct NewsEntry
    {
        QString title;
        QString date;
        QString summary;
        QString url;
        QString tag;        // e.g. "release", "blog", "update"
    };

    void buildNav();
    void loadPage(const QString& filename);
    QString resolveDocsDir() const;
    QString applyDarkCss(const QString& html) const;

    void buildNewsFeed();
    void loadNewsFromFile();
    void loadNewsFromNetwork();
    void populateNewsCards(const QList<NewsEntry>& entries);
    QWidget* createNewsCard(const NewsEntry& entry);

    QListWidget*  m_nav{nullptr};
    QTextBrowser* m_browser{nullptr};
    QLineEdit*    m_searchBar{nullptr};
    QPushButton*  m_backBtn{nullptr};
    QPushButton*  m_fwdBtn{nullptr};
    QPushButton*  m_bookmarkBtn{nullptr};

    // News feed
    QWidget*      m_newsPanel{nullptr};
    QVBoxLayout*  m_newsLayout{nullptr};
    QScrollArea*  m_newsScroll{nullptr};
    QLabel*       m_newsHeader{nullptr};
    QNetworkAccessManager* m_netManager{nullptr};

    QList<DocPage> m_pages;
    QString        m_currentFile;
    QStringList    m_bookmarks;  // filenames of bookmarked pages

    void loadBookmarks();
    void saveBookmarks();
    void rebuildNavWithBookmarks();
    void toggleBookmark();
    void updateBookmarkButton();

private slots:
    void onNavItemClicked(QListWidgetItem* item);
    void onSearchChanged(const QString& text);
    void onBackClicked();
    void onForwardClicked();
    void onNewsFetchFinished(QNetworkReply* reply);
};

#endif // DOCUMENTATION_WIDGET_HPP
