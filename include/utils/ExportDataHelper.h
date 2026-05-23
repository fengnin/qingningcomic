#ifndef EXPORTDATAHELPER_H
#define EXPORTDATAHELPER_H

#include <QVariantMap>

class ExportResult;

namespace ExportDataHelper {

ExportResult exportResultFromMap(const QVariantMap& data);

}

#endif // EXPORTDATAHELPER_H
