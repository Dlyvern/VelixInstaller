#ifndef FILE_DOWNLOADER_HPP
#define FILE_DOWNLOADER_HPP

#include <QObject>
#include <QFile>

class FileDownloader : public QObject
{
    Q_OBJECT
public:
    explicit FileDownloader(QObject* parent = nullptr);

    bool init(const QString& path);

public slots:
    void writeChunk(const QByteArray& chunk);

    void finish();

signals:
    void error(const QString& error);
    void saved(const QString& path);

private:
    QFile* m_file{nullptr};
};


#endif //FILE_DOWNLOADER_HPP