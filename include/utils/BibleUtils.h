#ifndef BIBLE_UTILS_H
#define BIBLE_UTILS_H

#include <QString>
#include <QStringList>
#include <QSet>
#include <QRegularExpression>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QMap>
#include <QUuid>
#include "models/Character.h"
#include "models/Scene.h"
#include "utils/JsonUtils.h"

namespace BibleUtils {

// 合并两个字符串列表并去重（过滤空值 + trim，对齐原仓库 mergeStringArrays）
inline QStringList mergeStringLists(const QStringList& base, const QStringList& incoming)
{
    QStringList result;
    QSet<QString> seen;
    
    auto addIfValid = [&result, &seen](const QString& item) {
        QString trimmed = item.trimmed();
        if (!trimmed.isEmpty() && !seen.contains(trimmed)) {
            seen.insert(trimmed);
            result.append(trimmed);
        }
    };
    
    for (const QString& item : base) {
        addIfValid(item);
    }
    for (const QString& item : incoming) {
        addIfValid(item);
    }
    return result;
}

// 合并两个 JSON 数组为字符串列表（去重 + trim）
inline QStringList mergeJsonArraysToStringList(const QJsonArray& base, const QJsonArray& incoming)
{
    return mergeStringLists(JsonUtils::jsonArrayToStringList(base), JsonUtils::jsonArrayToStringList(incoming));
}

// 从显示文本中提取字段值，兼容英文 key 和中文标签
inline QString extractDetailValue(const QString& detail, const QStringList& labels)
{
    for (const QString& label : labels) {
        if (label.isEmpty()) {
            continue;
        }

        const QRegularExpression pattern(QStringLiteral("%1\\s*[:：]\\s*([^，,；;、\\n]+)")
                                             .arg(QRegularExpression::escape(label)));
        const QRegularExpressionMatch match = pattern.match(detail);
        if (match.hasMatch()) {
            return match.captured(1).trimmed();
        }
    }
    return QString();
}

// 拆分逗号分隔列表，过滤空值并 trim
inline QStringList splitCommaSeparatedList(const QString& value)
{
    QStringList result;
    for (const QString& item : value.split(QRegularExpression(QStringLiteral("[,，、;；]")), Qt::SkipEmptyParts)) {
        const QString trimmed = item.trimmed();
        if (!trimmed.isEmpty()) {
            result << trimmed;
        }
    }
    return result;
}

// 判断是否应该更新字段（标量类型：只填充空值）
template<typename T>
inline bool shouldUpdateField(const T& existing, const T& newValue)
{
    return existing.isEmpty() && !newValue.isEmpty();
}

template<>
inline bool shouldUpdateField<int>(const int& existing, const int& newValue)
{
    return existing == 0 && newValue > 0;
}

template<>
inline bool shouldUpdateField<double>(const double& existing, const double& newValue)
{
    return existing == 0.0 && newValue > 0.0;
}

// 递归合并 JSON 对象（对齐原仓库 mergeObjectPreserve）
QJsonObject mergeObjectPreserve(const QJsonObject& base, const QJsonObject& incoming);

// 按 keyField 合并对象数组（对齐原仓库 mergeObjectArray）
QJsonArray mergeObjectArray(const QJsonArray& base, const QJsonArray& incoming, const QString& keyField);

// 合并 visualCharacteristics（对齐原仓库 mergeScenes 中的 visualCharacteristics 处理）
inline QJsonObject mergeVisualCharacteristics(const QJsonObject& base, const QJsonObject& incoming)
{
    return mergeObjectPreserve(base, incoming);
}

// 合并 spatialLayout（对齐原仓库 mergeScenes 中的 spatialLayout 处理）
QJsonObject mergeSpatialLayout(const QJsonObject& base, const QJsonObject& incoming);

// 合并 timeVariations 或 weatherVariations
inline QJsonArray mergeVariations(const QJsonArray& base, const QJsonArray& incoming, const QString& keyField)
{
    return mergeObjectArray(base, incoming, keyField);
}

// ==================== 推断逻辑 ====================

namespace Inference {

bool containsAny(const QString& text, const QStringList& tokens);
bool isIndoorSceneType(const QString& type);
QString inferSceneType(const QString& name, const QString& description);
QString inferSetting(const QString& name, const QString& description);
QString inferTimeOfDay(const QString& name, const QString& description);
QString inferWeather(const QString& name, const QString& description);
QString inferBuilding(const QString& setting);
QString inferColorScheme(const QString& timeOfDay, const QString& weather);
QString inferAtmosphere(const QString& timeOfDay, const QString& type);
QString inferLayout(const QString& type);
QString inferSpaceSize(const QString& name, const QString& description);
QString inferNarrativeRole(const QString& name, const QString& description);
int inferAge(const QString& role, const QString& name);

// ==================== 翻译函数（英文转中文） ====================

namespace Translation {

template<typename Map>
inline QString lookup(const Map& map, const QString& key)
{
    auto it = map.find(key);
    return (it != map.end()) ? it.value() : key;
}

QString translateSceneType(const QString& type);
QString translateTimeOfDay(const QString& timeOfDay);
QString translateWeather(const QString& weather);
QString translateNarrativeRole(const QString& role);

inline QString preferZh(const QString& zhValue, const QString& enValue, QString(*translator)(const QString&))
{
    return zhValue.isEmpty() ? translator(enValue) : zhValue;
}

}

using namespace Translation;

} // namespace Inference

// ==================== 场景文本过滤 ====================

namespace SceneTextFilter {

bool isDynamicSceneText(const QString& text);
QStringList filterStaticSceneList(const QStringList& items);
QString filterStaticSceneText(const QString& text);

inline QJsonArray toJsonArray(const QStringList& items)
{
    return QJsonArray::fromStringList(items);
}

} // namespace SceneTextFilter

// ==================== 圣经更新策略 ====================

namespace BibleUpdateStrategy {

QJsonObject resolveAppearanceObject(const QJsonObject& characterObj);
QJsonObject resolveSceneDetailsObject(const QJsonObject& sceneObj);
bool hasEmptyAppearanceFields(const CharacterAppearance& app);
bool hasNewAppearanceData(const QJsonObject& appObj);
bool hasCharacterAppearanceChanges(const CharacterAppearance& existing, const QJsonObject& appObj);
int sourcePriority(const QString& source);
bool isLockedSource(const QString& source);
bool isManualSource(const QString& source);
bool isExplicitSource(const QString& source);
bool isInferredSource(const QString& source);
int fieldSourcePolicyVersion();
QString fieldSource(const QJsonObject& sources, const QString& field);
void setFieldSource(QJsonObject& sources, const QString& field, const QString& source);
QString normalizedSource(const QString& source, const QString& fallback);
bool canReplaceSource(const QString& existingSource, const QString& incomingSource);
void setUpdatedSource(QJsonObject& sources, const QString& field, const QString& incomingSource, const QString& fallback);
void downgradeLegacyFieldSource(QJsonObject& sources, const QString& field);
bool shouldUpdateFromSource(const QString& existingSource, const QString& incomingSource);
bool canAdoptFieldValue(bool hasExistingValue, const QString& existingSource, const QString& incomingSource);
bool canAdoptFieldValue(const QString& existingValue, const QString& existingSource, const QString& incomingSource);
bool canAdoptFieldValue(int existingValue, const QString& existingSource, const QString& incomingSource);
bool canAdoptFieldValue(const QStringList& existingValues, const QString& existingSource, const QString& incomingSource);
bool hasDirectTextEvidence(const QString& sourceText, const QString& value);
bool hasDirectTextEvidence(const QString& sourceText, const QStringList& values);
QString sourceFromTextEvidence(const QString& sourceText, const QString& value, const QString& inferredValue = QString());
QString sourceFromTextEvidence(const QString& sourceText, const QStringList& values, const QStringList& inferredValues = QStringList());
bool hasCharacterPersonalityChanges(const QStringList& existing, const QJsonArray& incomingArray);
bool hasEmptyDetailsFields(const SceneDetails& det);
bool hasNewDetailsData(const QJsonObject& detailsObj);
bool hasIterationUpdates(const SceneDetails& existing, const QJsonObject& detailsObj);
void updateCharacterAppearance(CharacterAppearance& app, const QJsonObject& appObj);
void updateSceneDetails(SceneDetails& det, const QJsonObject& detailsObj);

} // namespace BibleUpdateStrategy

// ==================== 圣经上下文常量 ====================

namespace BibleContextConstants {

const QStringList& getDynamicTokens();
const QStringList& getLocationHints();
const QStringList& getLocationSuffixes();

} // namespace BibleContextConstants

// 公共工具函数
namespace Utils {

inline QString generateId()
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

} // namespace Utils

} // namespace BibleUtils

#endif
