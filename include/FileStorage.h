#ifndef FILESTORAGE_H
#define FILESTORAGE_H

#include <QString>
#include <QByteArray>
#include <QVariantMap>

/**
 * @brief 文件存储管理类
 * 
 * 负责管理小说文本、面板图片、角色参考图、导出文件等的存储。
 * 当前使用本地文件存储，后续可扩展支持云存储。
 */
class FileStorage
{
public:
    static FileStorage* instance();
    
    void init(const QString& basePath = "/data/comic");
    bool isStorageAvailable() const;
    
    // ========== 小说文本存储 ==========
    QString saveNovelText(const QString& novelId, const QString& text);
    QString loadNovelText(const QString& novelId, const QString& path = QString());
    bool deleteNovelText(const QString& path);
    
    // ========== 面板图片存储 ==========
    QString savePanelImage(const QString& panelId, const QByteArray& imageData, const QString& mode = "preview");
    QByteArray loadPanelImage(const QString& path);
    bool deletePanelImage(const QString& panelId, const QString& mode = "preview");
    QString getPanelImagePath(const QString& panelId, const QString& mode = "preview");
    
    // ========== 角色参考图存储 ==========
    QString saveCharacterReference(const QString& characterId, const QByteArray& imageData);
    bool deleteCharacterReference(const QString& characterId);
    
    // ========== 场景参考图存储 ==========
    QString saveSceneReference(const QString& sceneId, const QByteArray& imageData);
    bool deleteSceneReference(const QString& sceneId);
    
    // ========== 通用参考图存储（从本地文件） ==========
    QString saveReferenceImage(const QString& sourceFilePath, const QString& novelId);
    bool deleteReferenceImage(const QString& relativePath);
    
    // ========== 路径转换 ==========
    QString getFullPath(const QString& relativePath) const;
    
    // ========== 导出文件存储 ==========
    QString saveExportFile(const QString& exportId, const QByteArray& data, const QString& format = "pdf");
    QByteArray loadExportFile(const QString& path);
    QString getExportFilePath(const QString& exportId, const QString& format = "pdf");
    bool deleteExportFile(const QString& path);
    
    QString lastError() const;

private:
    FileStorage();
    ~FileStorage();
    FileStorage(const FileStorage&) = delete;
    FileStorage& operator=(const FileStorage&) = delete;
    
    QString buildFullPath(const QString& relativePath) const;
    bool ensureDirectoryExists(const QString& path);
    void setLastError(const QString& error);
    
    QString saveBinaryFile(const QString& relativePath, const QByteArray& data, const QString& logType);
    QByteArray loadBinaryFile(const QString& path, const QString& logType);
    bool deleteFile(const QString& path, const QString& logType);
    
    QString formatToExtension(const QString& format) const;
    
    static FileStorage* m_instance;
    QString m_basePath;
    QString m_lastError;
};

#endif // FILESTORAGE_H
