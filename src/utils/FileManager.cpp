#include "utils/FileManager.h"
#include <QFile>
#include <QFileInfo>
#include <QDir>

QString FileManager::readTextFile(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString();
    }
    QString content = QString::fromUtf8(file.readAll());
    file.close();
    return content;
}

bool FileManager::writeTextFile(const QString& filePath, const QString& content)
{
    if (!ensureDirectoryExists(filePath)) {
        return false;
    }
    
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }
    
    file.write(content.toUtf8());
    file.close();
    return true;
}

QByteArray FileManager::readBinaryFile(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return QByteArray();
    }
    QByteArray content = file.readAll();
    file.close();
    return content;
}

bool FileManager::writeBinaryFile(const QString& filePath, const QByteArray& data)
{
    if (!ensureDirectoryExists(filePath)) {
        return false;
    }
    
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    
    file.write(data);
    file.close();
    return true;
}

bool FileManager::fileExists(const QString& filePath)
{
    return QFile::exists(filePath);
}

bool FileManager::createDirectory(const QString& dirPath)
{
    QDir dir;
    return dir.mkpath(dirPath);
}

QString FileManager::getFileName(const QString& filePath)
{
    return QFileInfo(filePath).fileName();
}

QString FileManager::getFileExtension(const QString& filePath)
{
    return QFileInfo(filePath).suffix();
}

QString FileManager::getDirectoryPath(const QString& filePath)
{
    return QFileInfo(filePath).absolutePath();
}

bool FileManager::ensureDirectoryExists(const QString& filePath)
{
    QString dirPath = getDirectoryPath(filePath);
    QDir dir;
    return dir.mkpath(dirPath);
}
