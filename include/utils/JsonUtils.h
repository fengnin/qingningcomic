#ifndef JSON_UTILS_H
#define JSON_UTILS_H

#include <QString>
#include <QStringList>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QVariantMap>
#include <QDateTime>
#include <QDebug>
#include "Character.h"
#include "Storyboard.h"
#include "Panel.h"
#include "Job.h"

namespace JsonUtils {

namespace detail {

inline QJsonDocument parseJsonDocument(const QByteArray& data)
{
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    if (error.error != QJsonParseError::NoError) {
        qWarning() << "JSON parse error:" << error.errorString() << "at offset" << error.offset;
    }
    return doc;
}

inline QJsonDocument variantToJsonDocument(const QVariant& var)
{
    if (var.isNull() || !var.isValid()) {
        return QJsonDocument();
    }
    return parseJsonDocument(var.toByteArray());
}

}

template<typename T>
inline T get(const QJsonObject& obj, const QString& key, const T& defaultValue = T())
{
    if (!obj.contains(key)) {
        return defaultValue;
    }
    return obj.value(key).toVariant().value<T>();
}

template<>
inline QString get<QString>(const QJsonObject& obj, const QString& key, const QString& defaultValue)
{
    return obj.contains(key) ? obj.value(key).toString() : defaultValue;
}

template<>
inline int get<int>(const QJsonObject& obj, const QString& key, const int& defaultValue)
{
    return obj.contains(key) ? obj.value(key).toInt() : defaultValue;
}

template<>
inline double get<double>(const QJsonObject& obj, const QString& key, const double& defaultValue)
{
    return obj.contains(key) ? obj.value(key).toDouble() : defaultValue;
}

template<>
inline bool get<bool>(const QJsonObject& obj, const QString& key, const bool& defaultValue)
{
    return obj.contains(key) ? obj.value(key).toBool() : defaultValue;
}

inline QJsonObject getObject(const QJsonObject& obj, const QString& key)
{
    return obj.contains(key) && obj[key].isObject() ? obj[key].toObject() : QJsonObject();
}

inline QJsonArray getArray(const QJsonObject& obj, const QString& key)
{
    return obj.contains(key) && obj[key].isArray() ? obj[key].toArray() : QJsonArray();
}

inline QStringList toStringList(const QJsonArray& arr)
{
    QStringList result;
    result.reserve(arr.size());
    for (const QJsonValue& v : arr) {
        result << v.toString();
    }
    return result;
}

inline QStringList toStringList(const QJsonObject& obj, const QString& key)
{
    return toStringList(getArray(obj, key));
}

inline QJsonArray toJsonArray(const QStringList& list)
{
    return QJsonArray::fromStringList(list);
}

inline QString toString(const QJsonObject& obj)
{
    return QString::fromUtf8(QJsonDocument(obj).toJson(QJsonDocument::Compact));
}

inline QString toString(const QJsonArray& arr)
{
    return QString::fromUtf8(QJsonDocument(arr).toJson(QJsonDocument::Compact));
}

inline QJsonObject toObject(const QString& str)
{
    QJsonDocument doc = detail::parseJsonDocument(str.toUtf8());
    return doc.isObject() ? doc.object() : QJsonObject();
}

inline QJsonArray toArray(const QString& str)
{
    QJsonDocument doc = detail::parseJsonDocument(str.toUtf8());
    return doc.isArray() ? doc.array() : QJsonArray();
}

inline QJsonObject toObject(const QVariant& var)
{
    QJsonDocument doc = detail::variantToJsonDocument(var);
    return doc.isObject() ? doc.object() : QJsonObject();
}

inline QJsonArray toArray(const QVariant& var)
{
    QJsonDocument doc = detail::variantToJsonDocument(var);
    return doc.isArray() ? doc.array() : QJsonArray();
}

inline QStringList toStringList(const QVariant& var)
{
    return toStringList(toArray(var));
}

inline QString dateTimeToString(const QDateTime& dt)
{
    return dt.isValid() ? dt.toString(Qt::ISODate) : QString();
}

inline QDateTime toDateTime(const QString& str)
{
    if (str.isEmpty()) {
        return QDateTime();
    }
    QDateTime dt = QDateTime::fromString(str, Qt::ISODate);
    if (!dt.isValid()) {
        qWarning() << "Invalid datetime format:" << str;
    }
    return dt;
}

inline QDateTime getDateTime(const QJsonObject& obj, const QString& key)
{
    return toDateTime(get<QString>(obj, key));
}

inline void set(QJsonObject& obj, const QString& key, const QDateTime& dt)
{
    if (dt.isValid()) {
        obj[key] = dt.toString(Qt::ISODate);
    }
}

inline QVariantMap toVariantMap(const QString& str)
{
    QJsonDocument doc = detail::parseJsonDocument(str.toUtf8());
    return doc.isObject() ? doc.object().toVariantMap() : QVariantMap();
}

inline QString toString(const QVariantMap& map)
{
    return QString::fromUtf8(QJsonDocument::fromVariant(map).toJson(QJsonDocument::Compact));
}

inline QVariantMap toVariantMap(const QVariant& var)
{
    return toObject(var).toVariantMap();
}

QJsonObject toJson(const Character& character);
Character toCharacter(const QJsonObject& obj);

QJsonObject toJson(const CharacterAppearance& appearance);
CharacterAppearance toAppearance(const QJsonObject& obj);

QJsonObject toJson(const Storyboard& storyboard);
Storyboard toStoryboard(const QJsonObject& obj);

QJsonObject toJson(const Panel& panel);
Panel toPanel(const QJsonObject& obj);

QJsonObject toJson(const Job& job);
Job toJob(const QJsonObject& obj);

inline QString getString(const QJsonObject& obj, const QString& key, const QString& defaultValue = QString())
{
    return get<QString>(obj, key, defaultValue);
}

inline int getInt(const QJsonObject& obj, const QString& key, int defaultValue = 0)
{
    return get<int>(obj, key, defaultValue);
}

inline bool getBool(const QJsonObject& obj, const QString& key, bool defaultValue = false)
{
    return get<bool>(obj, key, defaultValue);
}

inline double getDouble(const QJsonObject& obj, const QString& key, double defaultValue = 0.0)
{
    return get<double>(obj, key, defaultValue);
}

inline QJsonObject variantToJson(const QVariant& var)
{
    return toObject(var);
}

inline QJsonArray variantToJsonArray(const QVariant& var)
{
    return toArray(var);
}

inline QStringList variantToStringList(const QVariant& var)
{
    return toStringList(var);
}

inline QString variantMapToJsonString(const QVariantMap& map)
{
    return toString(map);
}

inline QVariantMap jsonStringToVariantMap(const QString& str)
{
    return toVariantMap(str);
}

inline QVariantMap parseJsonField(const QVariant& variant)
{
    return toVariantMap(variant);
}

inline QString jsonToString(const QJsonObject& obj)
{
    return toString(obj);
}

inline QString jsonToString(const QJsonArray& arr)
{
    return toString(arr);
}

inline QJsonObject stringToJson(const QString& str)
{
    return toObject(str);
}

inline QJsonArray stringToJsonArray(const QString& str)
{
    return toArray(str);
}

inline QString getStringField(const QJsonObject& obj, const QString& key, const QString& defaultValue = QString())
{
    return get<QString>(obj, key, defaultValue);
}

inline int getIntField(const QJsonObject& obj, const QString& key, int defaultValue = 0)
{
    return get<int>(obj, key, defaultValue);
}

inline bool getBoolField(const QJsonObject& obj, const QString& key, bool defaultValue = false)
{
    return get<bool>(obj, key, defaultValue);
}

inline double getDoubleField(const QJsonObject& obj, const QString& key, double defaultValue = 0.0)
{
    return get<double>(obj, key, defaultValue);
}

inline QJsonObject getObjectField(const QJsonObject& obj, const QString& key)
{
    return getObject(obj, key);
}

inline QJsonArray getArrayField(const QJsonObject& obj, const QString& key)
{
    return getArray(obj, key);
}

inline QDateTime getDateTimeField(const QJsonObject& obj, const QString& key)
{
    return getDateTime(obj, key);
}

template<typename T>
inline T getValue(const QJsonObject& obj, const QString& key, const T& defaultValue = T())
{
    return get<T>(obj, key, defaultValue);
}

inline void setDateTimeField(QJsonObject& obj, const QString& key, const QDateTime& dt)
{
    set(obj, key, dt);
}

inline QStringList jsonArrayToStringList(const QJsonArray& arr)
{
    return toStringList(arr);
}

inline QJsonArray stringListToJsonArray(const QStringList& list)
{
    return toJsonArray(list);
}

}

#endif // JSON_UTILS_H
