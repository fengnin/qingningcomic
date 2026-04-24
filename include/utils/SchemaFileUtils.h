#ifndef SCHEMAFILEUTILS_H
#define SCHEMAFILEUTILS_H

#include <QString>
#include <QJsonObject>
#include <QJsonDocument>
#include <QFile>
#include <QIODevice>
#include <QDir>
#include <QCoreApplication>

namespace SchemaFileUtils {

inline QString resolveSchemaPath(const QString& schemaPath)
{
    if (QDir::isRelativePath(schemaPath)) {
        return QDir(QCoreApplication::applicationDirPath()).filePath(schemaPath);
    }
    return schemaPath;
}

inline QJsonObject readJsonObjectFile(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return QJsonObject();
    }

    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    return doc.isObject() ? doc.object() : QJsonObject();
}

inline QJsonObject readSchemaObject(const QString& schemaPath)
{
    return readJsonObjectFile(resolveSchemaPath(schemaPath));
}

}

#endif // SCHEMAFILEUTILS_H
