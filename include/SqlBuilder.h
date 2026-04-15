#ifndef SQLBUILDER_H
#define SQLBUILDER_H

#include <QString>
#include <QStringList>
#include <QVariant>
#include <QVariantMap>
#include <QVariantList>

class SqlBuilder
{
public:
    static QString buildInsert(const QString& table, const QVariantMap& data,
                               QStringList& outColumns, QVariantList& outValues);
    
    static QString buildUpdate(const QString& table, const QVariantMap& data,
                               const QString& whereClause, QVariantList& outValues);
    
    static QString buildSelect(const QString& table, const QString& whereClause = QString(),
                               const QString& orderBy = QString(), int limit = 0);
    
    static QString buildDelete(const QString& table, const QString& whereClause);
    
    static QString buildCount(const QString& table, const QString& whereClause = QString());
    
    static QString buildDirectSql(const QString& sql, const QVariantList& values);
    
    static QString escapeValue(const QVariant& value);

private:
    SqlBuilder() = delete;
};

#endif // SQLBUILDER_H
