#ifndef CHANGEREQUEST_H
#define CHANGEREQUEST_H

#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QList>
#include <QMetaType>
#include <QVariant>

struct ChangeRequestDsl;
struct ChangeRequestOp;

namespace ChangeRequestNormalization {
inline QString normalizeScope(const QString& value);
inline QString normalizeType(const QString& value);
inline QString normalizeAction(const QString& value);
inline bool isSupportedScope(const QString& value);
inline bool isSupportedType(const QString& value);
inline bool isSupportedAction(const QString& value);
inline void normalize(ChangeRequestDsl& dsl);
}

/**
 * @brief 改稿操作参数
 */
struct ChangeRequestOp {
    QString action;
    QJsonObject params;
    
    ChangeRequestOp() = default;
    ChangeRequestOp(const QString& a, const QJsonObject& p = QJsonObject())
        : action(a), params(p) {}
    
    static ChangeRequestOp fromJson(const QJsonObject& json) {
        ChangeRequestOp op;
        op.action = ChangeRequestNormalization::normalizeAction(json.value("action").toString(
            json.value("operation").toString(
                json.value("name").toString())));

        if (json.contains("params") && json.value("params").isObject()) {
            op.params = json.value("params").toObject();
        } else if (json.contains("parameters") && json.value("parameters").isObject()) {
            op.params = json.value("parameters").toObject();
        } else if (json.contains("args") && json.value("args").isObject()) {
            op.params = json.value("args").toObject();
        } else {
            op.params = QJsonObject();
        }
        return op;
    }
    
    QJsonObject toJson() const {
        QJsonObject json;
        json["action"] = action;
        json["params"] = params;
        return json;
    }
};

/**
 * @brief 改稿 DSL (Domain Specific Language)
 * 
 * 由 Qwen 解析自然语言后生成的结构化指令
 */
struct ChangeRequestDsl {
    QString scope;          // global, character, panel, page
    QString type;           // art, dialogue, layout, style
    QString targetId;       // 目标 ID (面板 ID、角色 ID 等)
    QList<ChangeRequestOp> ops;  // 操作列表
    QString reason;         // 修改原因
    QString priority;       // low, medium, high, urgent
    
    ChangeRequestDsl() : priority("medium") {}
    
    bool isValid() const {
        const QString normalizedScope = ChangeRequestNormalization::normalizeScope(scope);
        const QString normalizedType = ChangeRequestNormalization::normalizeType(type);
        if (!ChangeRequestNormalization::isSupportedScope(normalizedScope)) {
            return false;
        }
        if (!ChangeRequestNormalization::isSupportedType(normalizedType)) {
            return false;
        }
        if (targetId.trimmed().isEmpty()) {
            return false;
        }
        if (ops.isEmpty()) {
            return false;
        }
        for (const ChangeRequestOp& op : ops) {
            if (!ChangeRequestNormalization::isSupportedAction(op.action)) {
                return false;
            }
        }
        return true;
    }
    
    static ChangeRequestDsl fromJson(const QJsonObject& json) {
        ChangeRequestDsl dsl;
        const QJsonObject source = json.contains("dsl") && json.value("dsl").isObject()
            ? json.value("dsl").toObject()
            : (json.contains("change_request") && json.value("change_request").isObject()
                   ? json.value("change_request").toObject()
                   : json);

        dsl.scope = source.value("scope").toString(
            source.value("scopeType").toString(
                source.value("changeScope").toString(
                    source.value("scope_type").toString())));
        dsl.type = source.value("type").toString(
            source.value("changeType").toString(
                source.value("operationType").toString(
                    source.value("typeName").toString())));
        dsl.targetId = source.value("targetId").toString(
            source.value("target_id").toString(
                source.value("target").toString()));
        dsl.reason = source.value("reason").toString(
            source.value("explanation").toString());
        dsl.priority = source.value("priority").toString("medium");
        
        QJsonArray opsArray;
        if (source.contains("ops") && source.value("ops").isArray()) {
            opsArray = source.value("ops").toArray();
        } else if (source.contains("operations") && source.value("operations").isArray()) {
            opsArray = source.value("operations").toArray();
        } else if (source.contains("actions") && source.value("actions").isArray()) {
            opsArray = source.value("actions").toArray();
        }

        for (const auto& op : opsArray) {
            dsl.ops.append(ChangeRequestOp::fromJson(op.toObject()));
        }

        ChangeRequestNormalization::normalize(dsl);
        
        return dsl;
    }
    
    QJsonObject toJson() const {
        QJsonObject json;
        json["scope"] = scope;
        json["type"] = type;
        if (!targetId.isEmpty()) {
            json["targetId"] = targetId;
        }
        json["reason"] = reason;
        json["priority"] = priority;
        
        QJsonArray opsArray;
        for (const auto& op : ops) {
            opsArray.append(op.toJson());
        }
        json["ops"] = opsArray;
        
        return json;
    }
};

/**
 * @brief 改稿操作执行结果
 */
struct ChangeRequestOpResult {
    QString action;
    QString status;         // completed, failed
    QJsonObject output;     // 执行结果
    QString errorMessage;
    
    ChangeRequestOpResult() : status("pending") {}
    
    QJsonObject toJson() const {
        QJsonObject json;
        json["action"] = action;
        json["status"] = status;
        if (!output.isEmpty()) {
            json["output"] = output;
        }
        if (!errorMessage.isEmpty()) {
            json["error"] = errorMessage;
        }
        return json;
    }
};

/**
 * @brief 改稿请求状态
 */
enum class ChangeRequestStatus {
    Queued,      // 已排队
    Parsing,     // 正在解析
    Pending,     // 等待执行
    InProgress,  // 执行中
    Completed,   // 已完成
    Failed       // 失败
};

/**
 * @brief 改稿请求数据模型
 * 
 * 存储用户的自然语言改稿请求及其执行状态
 */
class ChangeRequest
{
    Q_GADGET
    Q_PROPERTY(QString id READ id WRITE setId)
    Q_PROPERTY(QString novelId READ novelId WRITE setNovelId)
    Q_PROPERTY(QString storyboardId READ storyboardId WRITE setStoryboardId)
    Q_PROPERTY(QString naturalLanguage READ naturalLanguage WRITE setNaturalLanguage)
    Q_PROPERTY(QString status READ status WRITE setStatus)
    Q_PROPERTY(QString userId READ userId WRITE setUserId)

public:
    ChangeRequest();
    ~ChangeRequest() = default;

    QString id() const { return m_id; }
    void setId(const QString& id) { m_id = id; }

    QString novelId() const { return m_novelId; }
    void setNovelId(const QString& novelId) { m_novelId = novelId; }

    QString storyboardId() const { return m_storyboardId; }
    void setStoryboardId(const QString& storyboardId) { m_storyboardId = storyboardId; }

    QString naturalLanguage() const { return m_naturalLanguage; }
    void setNaturalLanguage(const QString& text) { m_naturalLanguage = text; }

    QString status() const { return m_status; }
    void setStatus(const QString& status) { m_status = status; }

    QString userId() const { return m_userId; }
    void setUserId(const QString& userId) { m_userId = userId; }

    QString jobId() const { return m_jobId; }
    void setJobId(const QString& jobId) { m_jobId = jobId; }

    QString createdAt() const { return m_createdAt; }
    void setCreatedAt(const QString& createdAt) { m_createdAt = createdAt; }

    QString updatedAt() const { return m_updatedAt; }
    void setUpdatedAt(const QString& updatedAt) { m_updatedAt = updatedAt; }

    QString errorMessage() const { return m_errorMessage; }
    void setErrorMessage(const QString& error) { m_errorMessage = error; }

    ChangeRequestDsl dsl() const { return m_dsl; }
    void setDsl(const ChangeRequestDsl& dsl) { m_dsl = dsl; }

    QJsonObject context() const { return m_context; }
    void setContext(const QJsonObject& context) { m_context = context; }

    QList<ChangeRequestOpResult> results() const { return m_results; }
    void setResults(const QList<ChangeRequestOpResult>& results) { m_results = results; }
    void addResult(const ChangeRequestOpResult& result) { m_results.append(result); }

    bool isValid() const { return !m_id.isEmpty() && !m_novelId.isEmpty(); }
    
    bool isQueued() const { return m_status == "queued"; }
    bool isParsing() const { return m_status == "parsing"; }
    bool isPending() const { return m_status == "pending"; }
    bool isInProgress() const { return m_status == "in_progress"; }
    bool isCompleted() const { return m_status == "completed"; }
    bool isFailed() const { return m_status == "failed"; }

    QJsonObject toJson() const;
    static ChangeRequest fromJson(const QJsonObject& json);

    static QString statusToString(ChangeRequestStatus status);
    static ChangeRequestStatus stringToStatus(const QString& status);

private:
    QString m_id;
    QString m_novelId;
    QString m_storyboardId;
    QString m_naturalLanguage;
    QString m_status = "queued";
    QString m_userId;
    QString m_jobId;
    QString m_createdAt;
    QString m_updatedAt;
    QString m_errorMessage;
    ChangeRequestDsl m_dsl;
    QJsonObject m_context;
    QList<ChangeRequestOpResult> m_results;
};

Q_DECLARE_METATYPE(ChangeRequest)
Q_DECLARE_METATYPE(ChangeRequestDsl)
Q_DECLARE_METATYPE(ChangeRequestOp)
Q_DECLARE_METATYPE(ChangeRequestOpResult)

namespace ChangeRequestNormalization {

inline QString normalizeScope(const QString& value)
{
    return value.trimmed().toLower();
}

inline QString normalizeType(const QString& value)
{
    return value.trimmed().toLower();
}

inline QString normalizeAction(const QString& value)
{
    const QString trimmed = value.trimmed();
    const QString lower = trimmed.toLower();

    if (lower == QStringLiteral("setexpression")
        || lower == QStringLiteral("updateexpression")
        || lower == QStringLiteral("set_expression")
        || lower == QStringLiteral("update_expression")
        || lower == QStringLiteral("set-expression")
        || lower == QStringLiteral("update-expression")) {
        return QStringLiteral("setExpression");
    }
    if (lower == QStringLiteral("rewritedialogue")) {
        return QStringLiteral("rewrite_dialogue");
    }
    if (lower == QStringLiteral("setstyle")
        || lower == QStringLiteral("set_style")
        || lower == QStringLiteral("updatestyle")
        || lower == QStringLiteral("style_update")
        || lower == QStringLiteral("set-style")
        || lower == QStringLiteral("update-style")) {
        return QStringLiteral("update_style");
    }

    return lower;
}

inline bool isSupportedScope(const QString& value)
{
    const QString normalized = normalizeScope(value);
    return normalized == QStringLiteral("global")
        || normalized == QStringLiteral("character")
        || normalized == QStringLiteral("panel")
        || normalized == QStringLiteral("page");
}

inline bool isSupportedType(const QString& value)
{
    const QString normalized = normalizeType(value);
    return normalized == QStringLiteral("art")
        || normalized == QStringLiteral("dialogue")
        || normalized == QStringLiteral("layout")
        || normalized == QStringLiteral("style");
}

inline bool isSupportedAction(const QString& value)
{
    const QString normalized = normalizeAction(value);
    return normalized == QStringLiteral("inpaint")
        || normalized == QStringLiteral("outpaint")
        || normalized == QStringLiteral("bg_swap")
        || normalized == QStringLiteral("repose")
        || normalized == QStringLiteral("regen_panel")
        || normalized == QStringLiteral("setExpression")
        || normalized == QStringLiteral("add_effect")
        || normalized == QStringLiteral("resize")
        || normalized == QStringLiteral("update_style")
        || normalized == QStringLiteral("rewrite_dialogue")
        || normalized == QStringLiteral("reorder");
}

inline void normalize(ChangeRequestDsl& dsl)
{
    dsl.scope = normalizeScope(dsl.scope);
    dsl.type = normalizeType(dsl.type);
    dsl.targetId = dsl.targetId.trimmed();
    dsl.reason = dsl.reason.trimmed();
    dsl.priority = dsl.priority.trimmed().toLower();

    if (dsl.priority.isEmpty()) {
        dsl.priority = QStringLiteral("medium");
    }

    for (ChangeRequestOp& op : dsl.ops) {
        op.action = normalizeAction(op.action);
    }
}

} // namespace ChangeRequestNormalization

#endif // CHANGEREQUEST_H
