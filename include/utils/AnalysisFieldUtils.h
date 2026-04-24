#ifndef ANALYSISFIELDUTILS_H
#define ANALYSISFIELDUTILS_H

#include <QJsonObject>
#include <QString>
#include <QStringList>
#include <initializer_list>

namespace AnalysisFieldUtils {

template<typename T>
inline bool valueChanged(const T& current, const T& original)
{
    return current != original;
}

template<>
inline bool valueChanged<QString>(const QString& current, const QString& original)
{
    return current.trimmed() != original.trimmed();
}

template<>
inline bool valueChanged<int>(const int& current, const int& original)
{
    return current != original;
}

template<>
inline bool valueChanged<QStringList>(const QStringList& current, const QStringList& original)
{
    return current != original;
}

struct FieldMarkSpec
{
    const char* key;
    bool shouldMark;
};

inline void markFieldSources(QJsonObject& sources, std::initializer_list<FieldMarkSpec> fields, const QString& source)
{
    for (const FieldMarkSpec& field : fields) {
        if (field.shouldMark) {
            sources[QString::fromUtf8(field.key)] = source;
        }
    }
}

inline QString describeFieldChange(const QString& key,
                                   const QString& existingValue,
                                   const QString& incomingValue,
                                   const QString& existingSource,
                                   const QString& incomingSource)
{
    return QStringLiteral("%1: %2 [%3] -> %4 [%5]")
        .arg(key,
             existingValue.isEmpty() ? QString::fromUtf8("空") : existingValue,
             existingSource.isEmpty() ? QString::fromUtf8("无来源") : existingSource,
             incomingValue.isEmpty() ? QString::fromUtf8("空") : incomingValue,
             incomingSource.isEmpty() ? QString::fromUtf8("无来源") : incomingSource);
}

} // namespace AnalysisFieldUtils

#endif // ANALYSISFIELDUTILS_H
