#pragma once

#include <QString>
#include <QDateTime>
#include <QVariantMap>
#include <QJsonObject>

// 小说状态枚举
enum class NovelStatus
{
    Created,      // 已创建
    Analyzing,    // 分析中
    Analyzed,     // 待生成
    Completed,    // 已完成
    Error         // 处理失败
};

class Novel
{
public:
    Novel();
    ~Novel() = default;

    // 基础属性
    QString id() const { return m_id; }
    void setId(const QString& id) { m_id = id; }

    QString userId() const { return m_userId; }
    void setUserId(const QString& userId) { m_userId = userId; }

    QString title() const { return m_title; }
    void setTitle(const QString& title) { m_title = title; }

    QString originalText() const { return m_originalText; }
    void setOriginalText(const QString& text) { m_originalText = text; }

    QString originalTextPath() const { return m_originalTextPath; }
    void setOriginalTextPath(const QString& path) { m_originalTextPath = path; }

    NovelStatus status() const { return m_status; }
    void setStatus(NovelStatus status) { m_status = status; }
    QString statusString() const;

    QString storyboardId() const { return m_storyboardId; }
    void setStoryboardId(const QString& id) { m_storyboardId = id; }

    QVariantMap metadata() const { return m_metadata; }
    void setMetadata(const QVariantMap& metadata) { m_metadata = metadata; }

    QDateTime createdAt() const { return m_createdAt; }
    void setCreatedAt(const QDateTime& dt) { m_createdAt = dt; }

    QDateTime updatedAt() const { return m_updatedAt; }
    void setUpdatedAt(const QDateTime& dt) { m_updatedAt = dt; }

    // 状态转换
    static QString statusToString(NovelStatus status);
    static NovelStatus stringToStatus(const QString& str);

    // 数据库转换
    QVariantMap toVariantMap() const;
    static Novel fromVariantMap(const QVariantMap& map);

    // JSON 序列化
    QJsonObject toJson() const;
    static Novel fromJson(const QJsonObject& json);

private:
    QString m_id;
    QString m_userId;
    QString m_title;
    QString m_originalText;
    QString m_originalTextPath;
    NovelStatus m_status;
    QString m_storyboardId;
    QVariantMap m_metadata;
    QDateTime m_createdAt;
    QDateTime m_updatedAt;
    
    // 状态名称查找表
    static const char* STATUS_NAMES[];
};

Q_DECLARE_METATYPE(Novel)
Q_DECLARE_METATYPE(NovelStatus)
