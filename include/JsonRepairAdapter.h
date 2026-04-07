#pragma once

#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QVariant>

namespace JsonRepair {

struct RepairResult {
    QJsonValue value;
    QString error;
    bool success = false;
};

RepairResult repair(const QString& json);
QJsonObject repairToObject(const QString& json);
QJsonArray repairToArray(const QString& json);

}
