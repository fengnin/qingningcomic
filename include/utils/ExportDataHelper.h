#ifndef EXPORTDATAHELPER_H
#define EXPORTDATAHELPER_H

#include <QList>
#include <QVariantMap>
#include <QString>

class ExportResult;

namespace ExportDataHelper {

ExportResult exportResultFromMap(const QVariantMap& data);
void fillExportResultFromMap(ExportResult& result, const QVariantMap& data);
QVariantMap buildExportUpdateData(const QString& status, qint64 fileSize = 0);

}

#endif // EXPORTDATAHELPER_H
