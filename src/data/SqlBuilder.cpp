#include "data/SqlBuilder.h"

QString SqlBuilder::buildInsert(const QString& table, const QVariantMap& data,
                                 QStringList& outColumns, QVariantList& outValues)
{
    if (data.isEmpty()) {
        return QString();
    }
    
    QStringList placeholders;
    
    for (auto it = data.begin(); it != data.end(); ++it) {
        outColumns << it.key();
        placeholders << "?";
        outValues << it.value();
    }
    
    return QString("INSERT INTO %1 (%2) VALUES (%3)")
        .arg(table, outColumns.join(", "), placeholders.join(", "));
}

QString SqlBuilder::buildUpdate(const QString& table, const QVariantMap& data,
                                 const QString& whereClause, QVariantList& outValues)
{
    if (data.isEmpty()) {
        return QString();
    }
    
    QStringList setClauses;
    
    for (auto it = data.begin(); it != data.end(); ++it) {
        setClauses << QString("%1 = ?").arg(it.key());
        outValues << it.value();
    }
    
    QString sql = QString("UPDATE %1 SET %2").arg(table, setClauses.join(", "));
    
    if (!whereClause.isEmpty()) {
        sql += " WHERE " + whereClause;
    }
    
    return sql;
}

QString SqlBuilder::buildSelect(const QString& table, const QString& whereClause,
                                 const QString& orderBy, int limit)
{
    QString sql = QString("SELECT * FROM %1").arg(table);
    
    if (!whereClause.isEmpty()) {
        sql += " WHERE " + whereClause;
    }
    
    if (!orderBy.isEmpty()) {
        sql += " ORDER BY " + orderBy;
    }
    
    if (limit > 0) {
        sql += QString(" LIMIT %1").arg(limit);
    }
    
    return sql;
}

QString SqlBuilder::buildDelete(const QString& table, const QString& whereClause)
{
    QString sql = QString("DELETE FROM %1").arg(table);
    
    if (!whereClause.isEmpty()) {
        sql += " WHERE " + whereClause;
    }
    
    return sql;
}

QString SqlBuilder::buildCount(const QString& table, const QString& whereClause)
{
    QString sql = QString("SELECT COUNT(*) FROM %1").arg(table);
    
    if (!whereClause.isEmpty()) {
        sql += " WHERE " + whereClause;
    }
    
    return sql;
}

QString SqlBuilder::buildDirectSql(const QString& sql, const QVariantList& values)
{
    QString result = sql;
    int pos = 0;
    
    for (const QVariant& value : values) {
        int questionPos = result.indexOf("?", pos);
        if (questionPos == -1) break;
        
        QString valueStr = escapeValue(value);
        result = result.replace(questionPos, 1, valueStr);
        pos = questionPos + valueStr.length();
    }
    
    return result;
}

QString SqlBuilder::escapeValue(const QVariant& value)
{
    if (value.isNull()) {
        return "NULL";
    }
    
    if (value.type() == QVariant::String) {
        return QString("'%1'").arg(value.toString().replace("'", "''"));
    }
    
    return value.toString();
}
