#include "FileDownloader.hpp"
#include <QDebug>

FileDownloader::FileDownloader(QObject* parent) : QObject(parent)
{

}

bool FileDownloader::init(const QString& path)
{
    m_file = new QFile(path, this);

    if (!m_file->open(QIODevice::WriteOnly)) 
    {
        qDebug() << "Failed to open file: " << path;

        emit error("Failed to open file: " + path);

        delete m_file;
        m_file = nullptr;

        return false;
    }

    return true;
}

void FileDownloader::writeChunk(const QByteArray& chunk)
{
    if(m_file)
        m_file->write(chunk);
}

void FileDownloader::finish()
{
    if (m_file) 
    {
        m_file->flush();
        m_file->close();
        emit saved(m_file->fileName());
        m_file->deleteLater();
        m_file = nullptr;
    }
}
