#pragma once

#include <QString>
#include <QByteArray>

/**
 * @brief 文件管理器工具类
 * 
 * 提供文件读写、目录操作等通用功能。
 */
class FileManager
{
public:
    static QString readTextFile(const QString& filePath);
    static bool writeTextFile(const QString& filePath, const QString& content);
    static QByteArray readBinaryFile(const QString& filePath);
    static bool writeBinaryFile(const QString& filePath, const QByteArray& data);
    
    static bool fileExists(const QString& filePath);
    static bool createDirectory(const QString& dirPath);
    static QString getFileName(const QString& filePath);
    static QString getFileExtension(const QString& filePath);
    
    /**
     * @brief 确保文件所在目录存在
     * @param filePath 文件完整路径
     * @return 是否成功创建或目录已存在
     */
    static bool ensureDirectoryExists(const QString& filePath);
    
    /**
     * @brief 获取文件所在目录路径
     * @param filePath 文件完整路径
     * @return 目录路径
     */
    static QString getDirectoryPath(const QString& filePath);

private:
    FileManager() = delete;
    ~FileManager() = delete;
};
