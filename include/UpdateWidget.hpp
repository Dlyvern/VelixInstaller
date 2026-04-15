#ifndef UPDATE_WIDGET_HPP
#define UPDATE_WIDGET_HPP

#include <QWidget>
#include <QVBoxLayout>
#include <QTabWidget>
#include <QTextEdit>
#include <QFile>
#include <QPainter>
#include <QPainterPath>
#include <QPalette>

#include "AppUpdateChecker.hpp"
#include "widgets/VelixText.hpp"
#include "widgets/VelixProgressBar.hpp"
#include "FireButton.hpp"

class UpdateWidget : public QWidget
{
    Q_OBJECT
public:
    explicit UpdateWidget(AppUpdateChecker* checker, QWidget* parent = nullptr);
    ~UpdateWidget() override = default;

public slots:
    void onStableUpdateAvailable(const QString& version, const QString& downloadUrl, const QString& changelog);
    void onUnstableUpdateAvailable(const QString& version, const QString& downloadUrl, const QString& changelog);
    void onNoStableUpdate();
    void onNoUnstableUpdate();
    void onCheckFailed();

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    // Per-channel state
    struct Channel
    {
        QString           skipConfigKey;
        QString           version;
        QString           downloadUrl;

        VelixText*        latestLabel{nullptr};
        QTextEdit*        changelogEdit{nullptr};
        FireButton*       downloadBtn{nullptr};
        FireButton*       skipBtn{nullptr};
        VelixProgressBar* progressBar{nullptr};
        VelixText*        speedLabel{nullptr};
        VelixText*        statusLabel{nullptr};
        QWidget*          updatePanel{nullptr};
        QWidget*          upToDatePanel{nullptr};
        QFile*            downloadFile{nullptr};
    };

    QWidget* buildChannelPage(Channel& ch, const QString& label);
    void     startDownload(Channel& ch);
    void     skipVersion(Channel& ch);
    void     applyUpdate();

    AppUpdateChecker* m_checker{nullptr};
    QTabWidget*       m_tabWidget{nullptr};
    Channel*          m_activeChannel{nullptr};

    Channel m_stable;
    Channel m_unstable;
};

#endif // UPDATE_WIDGET_HPP
