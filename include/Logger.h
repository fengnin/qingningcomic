#ifndef LOGGER_H
#define LOGGER_H

#include <QObject>
#include <QFile>
#include <QTextStream>
#include <QMutex>
#include <QDateTime>
#include <QString>

class Logger
{
public:
    enum Level {
        Debug = 0,
        Info = 1,
        Warning = 2,
        Error = 3
    };

    static Logger* instance();
    
    bool init(const QString& logFilePath = QString());
    void close();
    
    void log(Level level, const QString& category, const QString& message);
    
    void setMinLevel(Level level);
    void setConsoleOutput(bool enabled);
    void setMaxFileSize(qint64 maxSize);
    void setMaxBackupFiles(int maxFiles);
    
    Level minLevel() const { return m_minLevel; }
    bool consoleOutput() const { return m_consoleOutput; }
    QString logFilePath() const;
    bool isInitialized() const { return m_initialized; }

private:
    Logger();
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    
    static QString levelToString(Level level);
    static QString formatMessage(const QString& timestamp, const QString& level, 
                                  const QString& category, const QString& message);
    
    void writeToConsole(Level level, const QString& message);
    void writeToFile(const QString& message);
    void writeSeparator(const QString& action);
    void checkFileSizeAndRotate();
    void rotateLogFile();
    QString getBackupFilePath(int index) const;
    
    static Logger* m_instance;
    static constexpr qint64 DEFAULT_MAX_FILE_SIZE = 10 * 1024 * 1024;
    static constexpr int DEFAULT_MAX_BACKUP_FILES = 5;
    
    QFile m_logFile;
    QTextStream m_logStream;
    mutable QMutex m_mutex;
    bool m_initialized;
    Level m_minLevel;
    bool m_consoleOutput;
    qint64 m_maxFileSize;
    int m_maxBackupFiles;
};

#define LOG_DEBUG(cat, msg)   Logger::instance()->log(Logger::Debug, cat, msg)
#define LOG_INFO(cat, msg)    Logger::instance()->log(Logger::Info, cat, msg)
#define LOG_WARNING(cat, msg) Logger::instance()->log(Logger::Warning, cat, msg)
#define LOG_ERROR(cat, msg)   Logger::instance()->log(Logger::Error, cat, msg)

#endif // LOGGER_H
