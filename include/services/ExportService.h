#ifndef EXPORT_SERVICE_H
#define EXPORT_SERVICE_H

#include "services/BaseService.h"
#include <QJsonObject>
#include <QJsonArray>
#include <QVariantMap>

struct ExportResult
{
    QString id;
    QString novelId;
    QString format;
    QString status;
    qint64 fileSize;
    QString fileUrl;
    QString createdAt;
    QString errorMessage;
    
    ExportResult() : status("pending"), fileSize(0) {}
};

class ExportService : public BaseService
{
    Q_OBJECT

public:
    explicit ExportService(DatabaseManager* db, QObject* parent = nullptr);
    virtual ~ExportService() = default;
    
    ExportResult getById(const QString& exportId);
    ExportResult createExport(const QString& novelId, const QString& format);
    bool updateExport(const QString& exportId, const QString& status, qint64 fileSize = 0);
    QString getFilePath(const QString& exportId, const QString& format);
    QList<ExportResult> getExportsByNovel(const QString& novelId);
    QList<ExportResult> getRecentExports(int limit = 10);
    
signals:
    void exportCreated(const ExportResult& result);
    void exportUpdated(const QString& exportId, const QString& status);
    void exportCompleted(const QString& exportId, const QString& fileUrl);
    void exportFailed(const QString& exportId, const QString& error);

private:
    static QString tableName() { return "exports"; }
};

#endif // EXPORT_SERVICE_H
