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

class DocumentationWidget : public QWidget
{
    Q_OBJECT
public:
    explicit DocumentationWidget(QWidget* parent = nullptr);
    ~DocumentationWidget() override = default;

private:
    struct DocPage
    {
        QString title;
        QString filename;   // empty = section header
    };

    void buildNav();
    void loadPage(const QString& filename);
    QString resolveDocsDir() const;
    QString applyDarkCss(const QString& html) const;

    QListWidget*  m_nav{nullptr};
    QTextBrowser* m_browser{nullptr};
    QLineEdit*    m_searchBar{nullptr};
    QPushButton*  m_backBtn{nullptr};
    QPushButton*  m_fwdBtn{nullptr};

    QList<DocPage> m_pages;
    QString        m_currentFile;

private slots:
    void onNavItemClicked(QListWidgetItem* item);
    void onSearchChanged(const QString& text);
    void onBackClicked();
    void onForwardClicked();
};

#endif // DOCUMENTATION_WIDGET_HPP
