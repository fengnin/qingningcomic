#include "Logger.h"
#include <QDebug>
#include <QDir>
#include <QStandardPaths>

Logger* Logger::m_instance = nullptr;

Logger::Logger()
    : m_initialized(false)
    , m_minLevel(Debug)
    , m_consoleOutput(true)
    , m_maxFileSize(DEFAULT_MAX_FILE_SIZE)
    , m_maxBackupFiles(DEFAULT_MAX_BACKUP_FILES)
{
}

Logger::~Logger()
{
    close();
}

Logger* Logger::instance()
{
    if (!m_instance) {
        m_instance = new Logger();
    }
    return m_instance;
}

QString Logger::levelToString(Level level)
{
    switch (level) {
        case Debug:   return "DEBUG";
        case Info:    return "INFO";
        case Warning: return "WARN";
        case Error:   return "ERROR";
        default:      return "UNKNOWN";
    }
}

QString Logger::formatMessage(const QString& timestamp, const QString& level, 
                               const QString& category, const QString& message)
{
    return QString("[%1] [%2] [%3] %4").arg(timestamp, level, category, message);
}

bool Logger::init(const QString& logFilePath)
{
    QMutexLocker locker(&m_mutex);
    
    if (m_initialized) {
        return true;
    }
    
    QString filePath = logFilePath;
    
    if (filePath.isEmpty()) {
        QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QDir dir(dataPath);
        if (!dir.exists()) {
            dir.mkpath(".");
        }
        filePath = dataPath + "/qingning.log";
    }
    
    m_logFile.setFileName(filePath);
    
    if (!m_logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        qWarning() << "Failed to open log file:" << filePath;
        return false;
    }
    
    m_logStream.setDevice(&m_logFile);
    m_logStream.setCodec("UTF-8");
    m_initialized = true;
    
    writeSeparator("Opened");
    return true;
}

void Logger::log(Level level, const QString& category, const QString& message)
{
    if (level < m_minLevel) {
        return;
    }
    
    QMutexLocker locker(&m_mutex);
    
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    QString formatted = formatMessage(timestamp, levelToString(level), category, message);
    
    writeToConsole(level, formatted);
    writeToFile(formatted);
    
    checkFileSizeAndRotate();
}

void Logger::setMinLevel(Level level)
{
    QMutexLocker locker(&m_mutex);
    m_minLevel = level;
}

void Logger::setConsoleOutput(bool enabled)
{
    QMutexLocker locker(&m_mutex);
    m_consoleOutput = enabled;
}

void Logger::setMaxFileSize(qint64 maxSize)
{
    QMutexLocker locker(&m_mutex);
    m_maxFileSize = maxSize;
}

void Logger::setMaxBackupFiles(int maxFiles)
{
    QMutexLocker locker(&m_mutex);
    m_maxBackupFiles = maxFiles;
}

QString Logger::logFilePath() const
{
    return m_logFile.fileName();
}

void Logger::close()
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_initialized) {
        return;
    }
    
    writeSeparator("Closed");
    
    m_logStream.flush();
    m_logFile.close();
    m_initialized = false;
}

void Logger::writeToConsole(Level level, const QString& message)
{
    if (!m_consoleOutput) {
        return;
    }
    
    switch (level) {
        case Debug:
        case Info:
            qDebug().noquote() << message;
            break;
        case Warning:
            qWarning().noquote() << message;
            break;
        case Error:
            qCritical().noquote() << message;
            break;
    }
}

void Logger::writeToFile(const QString& message)
{
    if (m_initialized && m_logFile.isOpen()) {
        m_logStream << message << "\n";
        m_logStream.flush();
    }
}

void Logger::writeSeparator(const QString& action)
{
    QString message = QString("========== QingningComic %1 at %2 ==========")
        .arg(action)
        .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
    
    writeToConsole(Info, message);
    writeToFile(message);
}

void Logger::checkFileSizeAndRotate()
{
    if (!m_initialized || !m_logFile.isOpen()) {
        return;
    }
    
    if (m_logFile.size() >= m_maxFileSize) {
        rotateLogFile();
    }
}

void Logger::rotateLogFile()
{
    m_logStream.flush();
    m_logFile.close();
    
    QString backupPath = getBackupFilePath(m_maxBackupFiles);
    QFile oldFile(backupPath);
    if (oldFile.exists()) {
        oldFile.remove();
    }
    
    for (int i = m_maxBackupFiles - 1; i >= 1; --i) {
        QString oldPath = getBackupFilePath(i);
        QString newPath = getBackupFilePath(i + 1);
        
        QFile file(oldPath);
        if (file.exists()) {
            file.rename(newPath);
        }
    }
    
    QString firstBackup = getBackupFilePath(1);
    QFile::rename(m_logFile.fileName(), firstBackup);
    
    if (!m_logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        qWarning() << "Failed to reopen log file after rotation";
        m_initialized = false;
        return;
    }
    
    m_logStream.setDevice(&m_logFile);
    m_logStream.setCodec("UTF-8");
    
    writeToFile(QString("========== Log rotated. Previous log: %1 ==========").arg(firstBackup));
}

QString Logger::getBackupFilePath(int index) const
{
    return QString("%1.%2").arg(m_logFile.fileName()).arg(index);
}
