#include "Novel.h"
#include <QUuid>
#include "utils/JsonUtils.h"

// 状态名称查找表
const char* Novel::STATUS_NAMES[] = {
    "created",    // NovelStatus::Created = 0
    "analyzing",  // NovelStatus::Analyzing = 1
    "completed",  // NovelStatus::Completed = 2
    "error"       // NovelStatus::Error = 3
};

Novel::Novel()
    : m_status(NovelStatus::Created)
    , m_createdAt(QDateTime::currentDateTime())
    , m_updatedAt(QDateTime::currentDateTime())
{
}

QString Novel::statusString() const
{
    return statusToString(m_status);
}

QString Novel::statusToString(NovelStatus status)
{
    int index = static_cast<int>(status);
    return (index >= 0 && index <= 3) ? QString::fromLatin1(STATUS_NAMES[index]) : QStringLiteral("created");
}

NovelStatus Novel::stringToStatus(const QString& str)
{
    for (int i = 0; i <= 3; ++i) {
        if (str == QString::fromLatin1(STATUS_NAMES[i])) {
            return static_cast<NovelStatus>(i);
        }
    }
    return NovelStatus::Created;
}

QVariantMap Novel::toVariantMap() const
{
    QVariantMap map;
    map[QStringLiteral("id")] = m_id;
    map[QStringLiteral("user_id")] = m_userId;
    map[QStringLiteral("title")] = m_title;
    map[QStringLiteral("original_text")] = m_originalText;
    map[QStringLiteral("original_text_path")] = m_originalTextPath;
    map[QStringLiteral("status")] = statusToString(m_status);
    map[QStringLiteral("storyboard_id")] = m_storyboardId;
    map[QStringLiteral("metadata")] = JsonUtils::variantMapToJsonString(m_metadata);
    map[QStringLiteral("created_at")] = m_createdAt.toString(Qt::ISODate);
    map[QStringLiteral("updated_at")] = m_updatedAt.toString(Qt::ISODate);
    return map;
}

Novel Novel::fromVariantMap(const QVariantMap& map)
{
    Novel novel;
    novel.setId(map[QStringLiteral("id")].toString());
    novel.setUserId(map[QStringLiteral("user_id")].toString());
    novel.setTitle(map[QStringLiteral("title")].toString());
    novel.setOriginalText(map[QStringLiteral("original_text")].toString());
    novel.setOriginalTextPath(map[QStringLiteral("original_text_path")].toString());
    novel.setStatus(stringToStatus(map[QStringLiteral("status")].toString()));
    novel.setStoryboardId(map[QStringLiteral("storyboard_id")].toString());
    novel.setMetadata(JsonUtils::parseJsonField(map[QStringLiteral("metadata")]));
    novel.setCreatedAt(QDateTime::fromString(map[QStringLiteral("created_at")].toString(), Qt::ISODate));
    novel.setUpdatedAt(QDateTime::fromString(map[QStringLiteral("updated_at")].toString(), Qt::ISODate));
    return novel;
}

QJsonObject Novel::toJson() const
{
    QJsonObject json;
    json[QStringLiteral("id")] = m_id;
    json[QStringLiteral("userId")] = m_userId;
    json[QStringLiteral("title")] = m_title;
    json[QStringLiteral("originalText")] = m_originalText;
    json[QStringLiteral("originalTextPath")] = m_originalTextPath;
    json[QStringLiteral("status")] = statusToString(m_status);
    json[QStringLiteral("storyboardId")] = m_storyboardId;
    json[QStringLiteral("metadata")] = QJsonObject::fromVariantMap(m_metadata);
    json[QStringLiteral("createdAt")] = m_createdAt.toString(Qt::ISODate);
    json[QStringLiteral("updatedAt")] = m_updatedAt.toString(Qt::ISODate);
    return json;
}

Novel Novel::fromJson(const QJsonObject& json)
{
    Novel novel;
    novel.setId(json[QStringLiteral("id")].toString());
    novel.setUserId(json[QStringLiteral("userId")].toString());
    novel.setTitle(json[QStringLiteral("title")].toString());
    novel.setOriginalText(json[QStringLiteral("originalText")].toString());
    novel.setOriginalTextPath(json[QStringLiteral("originalTextPath")].toString());
    novel.setStatus(stringToStatus(json[QStringLiteral("status")].toString()));
    novel.setStoryboardId(json[QStringLiteral("storyboardId")].toString());
    novel.setMetadata(json[QStringLiteral("metadata")].toObject().toVariantMap());
    novel.setCreatedAt(QDateTime::fromString(json[QStringLiteral("createdAt")].toString(), Qt::ISODate));
    novel.setUpdatedAt(QDateTime::fromString(json[QStringLiteral("updatedAt")].toString(), Qt::ISODate));
    return novel;
}
