#include "data/FileStorage.h"
#include "data/DatabaseManager.h"
#include "utils/FileManager.h"
#include "utils/Logger.h"
#include <QFileInfo>
#include <QFile>
#include <QDir>
#include <QUuid>

namespace {
    constexpr int SHORT_TEXT_THRESHOLD = 10000;

    QString buildPanelRelativePath(const QString& panelId, const QString& mode)
    {
        return QString("panels/%1/%2.png").arg(panelId.left(8), mode);
    }

    QString buildReferenceRelativePath(const QString& folder, const QString& entityId, const QString& fileName)
    {
        return QString("%1/%2/%3").arg(folder, entityId.left(8), fileName);
    }

    QString buildExportRelativePath(const QString& exportId, const QString& extension)
    {
        return QString("exports/%1.%2").arg(exportId, extension);
    }

    QString buildNovelTextRelativePath(const QString& novelId)
    {
        return QString("novels/%1/text.txt").arg(novelId);
    }
}

FileStorage* FileStorage::m_instance = nullptr;
std::once_flag FileStorage::m_instanceOnceFlag;

FileStorage::FileStorage()
    : m_basePath("/data/comic")
{
}

FileStorage::~FileStorage()
{
}

FileStorage* FileStorage::instance()
{
    std::call_once(m_instanceOnceFlag, []() {
        m_instance = new FileStorage();
    });
    return m_instance;
}

void FileStorage::init(const QString& basePath)
{
    QString resolvedBasePath = basePath.trimmed().isEmpty()
        ? QStringLiteral("/data/comic")
        : basePath.trimmed();

    QDir dir(resolvedBasePath);
    m_basePath = dir.absolutePath();
    
    ensureDirectoryExists(m_basePath);
    LOG_INFO("FileStorage", QString("Storage initialized at: %1 (absolute path)").arg(m_basePath));
}

bool FileStorage::isStorageAvailable() const
{
    return QDir(m_basePath).exists() || QDir().mkpath(m_basePath);
}

QString FileStorage::buildFullPath(const QString& relativePath) const
{
    return QString("%1/%2").arg(m_basePath, relativePath);
}

QString FileStorage::getFullPath(const QString& relativePath) const
{
    return buildFullPath(relativePath);
}

bool FileStorage::ensureDirectoryExists(const QString& path)
{
    QDir dir(path);
    if (dir.exists()) {
        return true;
    }
    
    if (dir.mkpath(".")) {
        LOG_DEBUG("FileStorage", QString("Created directory: %1").arg(path));
        return true;
    }
    
    setLastError(QString("Failed to create directory: %1").arg(path));
    return false;
}

void FileStorage::setLastError(const QString& error)
{
    m_lastError = error;
    LOG_ERROR("FileStorage", error);
}

QString FileStorage::saveBinaryFile(const QString& relativePath, const QByteArray& data, const QString& logType)
{
    QString fullPath = buildFullPath(relativePath);
    ensureDirectoryExists(QFileInfo(fullPath).absolutePath());
    
    if (FileManager::writeBinaryFile(fullPath, data)) {
        LOG_INFO("FileStorage", QString("%1 saved: %2").arg(logType, relativePath));
        return relativePath;
    }
    
    setLastError(QString("Failed to save %1").arg(logType.toLower()));
    return QString();
}

QByteArray FileStorage::loadBinaryFile(const QString& path, const QString& logType)
{
    QString fullPath = buildFullPath(path);
    QByteArray data = FileManager::readBinaryFile(fullPath);
    
    if (data.isEmpty()) {
        setLastError(QString("Failed to load %1: %2").arg(logType.toLower(), path));
        return QByteArray();
    }
    
    LOG_DEBUG("FileStorage", QString("%1 loaded: %2").arg(logType, path));
    return data;
}

bool FileStorage::deleteFile(const QString& path, const QString& logType)
{
    QString fullPath = buildFullPath(path);
    bool result = QFile::remove(fullPath);
    
    if (result) {
        LOG_DEBUG("FileStorage", QString("%1 deleted: %2").arg(logType, path));
    }
    
    return result;
}

QString FileStorage::formatToExtension(const QString& format) const
{
    QString ext = format.toLower();
    if (ext == "webtoon") return "png";
    if (ext == "resources") return "zip";
    return ext;
}

// ========== 小说文本存储 ==========

QString FileStorage::saveNovelText(const QString& novelId, const QString& text)
{
    if (text.isEmpty()) {
        return QString();
    }
    
    if (text.length() < SHORT_TEXT_THRESHOLD) {
        LOG_DEBUG("FileStorage", QString("Saving short text to database: %1 chars").arg(text.length()));
        return QString();
    }
    
    QString relativePath = buildNovelTextRelativePath(novelId);
    QString fullPath = buildFullPath(relativePath);
    ensureDirectoryExists(QFileInfo(fullPath).absolutePath());
    
    if (FileManager::writeTextFile(fullPath, text)) {
        LOG_INFO("FileStorage", QString("Novel text saved: %1").arg(relativePath));
        return relativePath;
    }
    
    setLastError("Failed to save novel text");
    return QString();
}

QString FileStorage::loadNovelText(const QString& novelId, const QString& path)
{
    if (path.isEmpty()) {
        DatabaseManager* db = DatabaseManager::instance();
        QVariantMap novel = db->selectOne("novels", "id = ?", QVariantList() << novelId);
        return novel.value("original_text").toString();
    }
    
    QString fullPath = buildFullPath(path);
    QString text = FileManager::readTextFile(fullPath);
    
    if (text.isEmpty()) {
        setLastError(QString("Failed to load novel text: %1").arg(path));
        return QString();
    }
    
    LOG_DEBUG("FileStorage", QString("Novel text loaded: %1").arg(path));
    return text;
}

bool FileStorage::deleteNovelText(const QString& path)
{
    if (path.isEmpty()) {
        return true;
    }
    return deleteFile(path, "Novel text");
}

// ========== 面板图片存储 ==========

QString FileStorage::savePanelImage(const QString& panelId, const QByteArray& imageData, const QString& mode)
{
    if (imageData.isEmpty()) {
        return QString();
    }
    
    QString relativePath = buildPanelRelativePath(panelId, mode);
    return saveBinaryFile(relativePath, imageData, "Panel image");
}

QByteArray FileStorage::loadPanelImage(const QString& path)
{
    return loadBinaryFile(path, "Panel image");
}

bool FileStorage::deletePanelImage(const QString& panelId, const QString& mode)
{
    QString relativePath = buildPanelRelativePath(panelId, mode);
    return deleteFile(relativePath, "Panel image");
}

QString FileStorage::getPanelImagePath(const QString& panelId, const QString& mode)
{
    QString relativePath = buildPanelRelativePath(panelId, mode);
    return buildFullPath(relativePath);
}

// ========== 角色参考图存储 ==========

QString FileStorage::saveCharacterReference(const QString& characterId, const QByteArray& imageData)
{
    if (imageData.isEmpty()) {
        return QString();
    }
    
    QString relativePath = buildReferenceRelativePath("characters", characterId, "reference.png");
    return saveBinaryFile(relativePath, imageData, "Character reference");
}

bool FileStorage::deleteCharacterReference(const QString& characterId)
{
    QString relativePath = buildReferenceRelativePath("characters", characterId, "reference.png");
    return deleteFile(relativePath, "Character reference");
}

// ========== 场景参考图存储 ==========

QString FileStorage::saveSceneReference(const QString& sceneId, const QByteArray& imageData)
{
    if (imageData.isEmpty()) {
        return QString();
    }
    
    QString relativePath = buildReferenceRelativePath("scenes", sceneId, "reference.png");
    return saveBinaryFile(relativePath, imageData, "Scene reference");
}

bool FileStorage::deleteSceneReference(const QString& sceneId)
{
    QString relativePath = buildReferenceRelativePath("scenes", sceneId, "reference.png");
    return deleteFile(relativePath, "Scene reference");
}

// ========== 通用参考图存储 ==========

QString FileStorage::saveReferenceImage(const QString& sourceFilePath, const QString& novelId)
{
    if (sourceFilePath.isEmpty() || novelId.isEmpty()) {
        return QString();
    }
    
    QFile sourceFile(sourceFilePath);
    if (!sourceFile.open(QIODevice::ReadOnly)) {
        setLastError("Failed to open source file: " + sourceFilePath);
        return QString();
    }
    
    QByteArray imageData = sourceFile.readAll();
    sourceFile.close();
    
    if (imageData.isEmpty()) {
        setLastError("Source file is empty: " + sourceFilePath);
        return QString();
    }
    
    QString ext = QFileInfo(sourceFilePath).suffix();
    if (ext.isEmpty()) {
        ext = "png";
    }
    
    QString fileName = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QString relativePath = buildReferenceRelativePath("references", novelId, QString("%1.%2").arg(fileName, ext));
    
    QString result = saveBinaryFile(relativePath, imageData, "Reference image");
    if (!result.isEmpty()) {
        LOG_INFO("FileStorage", QString("Reference image saved: %1 -> %2").arg(sourceFilePath, relativePath));
    }
    return result;
}

bool FileStorage::deleteReferenceImage(const QString& relativePath)
{
    if (relativePath.isEmpty()) {
        return false;
    }
    return deleteFile(relativePath, "Reference image");
}

// ========== 导出文件存储 ==========

QString FileStorage::saveExportFile(const QString& exportId, const QByteArray& data, const QString& format)
{
    if (data.isEmpty()) {
        return QString();
    }
    
    QString extension = formatToExtension(format);
    QString relativePath = buildExportRelativePath(exportId, extension);
    return saveBinaryFile(relativePath, data, "Export file");
}

QByteArray FileStorage::loadExportFile(const QString& path)
{
    return loadBinaryFile(path, "Export file");
}

QString FileStorage::getExportFilePath(const QString& exportId, const QString& format)
{
    QString extension = formatToExtension(format);
    QString relativePath = buildExportRelativePath(exportId, extension);
    return buildFullPath(relativePath);
}

bool FileStorage::deleteExportFile(const QString& path)
{
    if (path.isEmpty()) {
        return true;
    }
    return deleteFile(path, "Export file");
}

QString FileStorage::lastError() const
{
    return m_lastError;
}
