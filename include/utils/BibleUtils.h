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

QStringList mergeStringLists(const QStringList& base, const QStringList& incoming);
QStringList mergeJsonArraysToStringList(const QJsonArray& base, const QJsonArray& incoming);
QString extractDetailValue(const QString& detail, const QStringList& labels);
QStringList splitCommaSeparatedList(const QString& value);

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

QStringList jsonArrayToStringList(const QJsonArray& array);
QJsonObject mergeObjectPreserve(const QJsonObject& base, const QJsonObject& incoming);
QJsonArray mergeObjectArray(const QJsonArray& base, const QJsonArray& incoming, const QString& keyField);
QJsonObject mergeVisualCharacteristics(const QJsonObject& base, const QJsonObject& incoming);
QJsonObject mergeSpatialLayout(const QJsonObject& base, const QJsonObject& incoming);
QJsonArray mergeVariations(const QJsonArray& base, const QJsonArray& incoming, const QString& keyField);

// ==================== 推断逻辑 ====================

namespace Inference {

bool containsAny(const QString& text, const QStringList& tokens);
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

} // namespace Inference

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
QString preferZh(const QString& zhValue, const QString& enValue, QString(*translator)(const QString&));

} // namespace Translation

// ==================== 场景文本过滤 ====================

namespace SceneTextFilter {

bool isDynamicSceneText(const QString& text);
QStringList filterStaticSceneList(const QStringList& items);
QString filterStaticSceneText(const QString& text);
QJsonArray toJsonArray(const QStringList& items);

} // namespace SceneTextFilter

// ==================== 圣经更新策略 ====================

namespace BibleUpdateStrategy {

QJsonObject resolveAppearanceObject(const QJsonObject& characterObj);
QJsonObject resolveSceneDetailsObject(const QJsonObject& sceneObj);
bool hasEmptyAppearanceFields(const CharacterAppearance& app);
bool hasNewAppearanceData(const QJsonObject& appObj);
bool hasCharacterAppearanceChanges(const CharacterAppearance& existing, const QJsonObject& appObj);
bool isExplicitSource(const QString& source);
bool isInferredSource(const QString& source);
bool isManualSource(const QString& source);
bool isLockedSource(const QString& source);
int sourcePriority(const QString& source);
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

inline const QStringList& getDynamicTokens()
{
    static const QStringList tokens = {
        QString::fromUtf8("我"), QString::fromUtf8("你"), QString::fromUtf8("他"),
        QString::fromUtf8("她"), QString::fromUtf8("他们"), QString::fromUtf8("她们"),
        QString::fromUtf8("角色"), QString::fromUtf8("人物"), QString::fromUtf8("主角"),
        QString::fromUtf8("配角"), QString::fromUtf8("说"), QString::fromUtf8("喊"),
        QString::fromUtf8("问"), QString::fromUtf8("看"), QString::fromUtf8("走"),
        QString::fromUtf8("跑"), QString::fromUtf8("站"), QString::fromUtf8("坐"),
        QString::fromUtf8("躺"), QString::fromUtf8("笑"), QString::fromUtf8("哭"),
        QString::fromUtf8("目光"), QString::fromUtf8("视线"), QString::fromUtf8("指尖"),
        QString::fromUtf8("声音"), QString::fromUtf8("对话"), QString::fromUtf8("镜头"),
        QString::fromUtf8("特写"), QString::fromUtf8("近景"), QString::fromUtf8("中景"),
        QString::fromUtf8("远景")
    };
    return tokens;
}

inline const QStringList& getLocationHints()
{
    static const QStringList hints = {
        QString::fromUtf8("房"), QString::fromUtf8("室"), QString::fromUtf8("厅"),
        QString::fromUtf8("厨房"), QString::fromUtf8("客厅"), QString::fromUtf8("卧室"),
        QString::fromUtf8("走廊"), QString::fromUtf8("门"), QString::fromUtf8("院"),
        QString::fromUtf8("楼"), QString::fromUtf8("街"), QString::fromUtf8("路"),
        QString::fromUtf8("巷"), QString::fromUtf8("店"), QString::fromUtf8("摊"),
        QString::fromUtf8("市场"), QString::fromUtf8("广场"), QString::fromUtf8("车站"),
        QString::fromUtf8("码头"), QString::fromUtf8("剧场"), QString::fromUtf8("戏台"),
        QString::fromUtf8("后台"), QString::fromUtf8("仓库"), QString::fromUtf8("教室"),
        QString::fromUtf8("办公室"), QString::fromUtf8("病房"), QString::fromUtf8("阳台"),
        QString::fromUtf8("天台"), QString::fromUtf8("屋顶"), QString::fromUtf8("庭院"),
        QString::fromUtf8("入口"), QString::fromUtf8("门口")
    };
    return hints;
}

inline const QStringList& getLocationSuffixes()
{
    static const QStringList suffixes = {
        QString::fromUtf8("客厅"), QString::fromUtf8("厨房"), QString::fromUtf8("卧室"),
        QString::fromUtf8("书房"), QString::fromUtf8("教室"), QString::fromUtf8("办公室"),
        QString::fromUtf8("病房"), QString::fromUtf8("走廊"), QString::fromUtf8("院门"),
        QString::fromUtf8("门口"), QString::fromUtf8("门前"), QString::fromUtf8("门内"),
        QString::fromUtf8("院子"), QString::fromUtf8("庭院"), QString::fromUtf8("阳台"),
        QString::fromUtf8("天台"), QString::fromUtf8("屋顶"), QString::fromUtf8("楼道"),
        QString::fromUtf8("街道"), QString::fromUtf8("小径"), QString::fromUtf8("小巷"),
        QString::fromUtf8("石板路"), QString::fromUtf8("书市"), QString::fromUtf8("旧书市"),
        QString::fromUtf8("摊位"), QString::fromUtf8("摊前"), QString::fromUtf8("店铺"),
        QString::fromUtf8("市场"), QString::fromUtf8("广场"), QString::fromUtf8("后台"),
        QString::fromUtf8("舞台"), QString::fromUtf8("戏台"), QString::fromUtf8("剧场"),
        QString::fromUtf8("仓库")
    };
    return suffixes;
}

} // namespace BibleContextConstants

// 公共工具函数
namespace Utils {

QString generateId();

} // namespace Utils

} // namespace BibleUtils

#endif
