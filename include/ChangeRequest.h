#ifndef CHANGEREQUEST_H
#define CHANGEREQUEST_H

#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QList>
#include <QMetaType>
#include <QVariant>

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
        op.action = json["action"].toString();
        op.params = json["params"].toObject();
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
        return !scope.isEmpty() && !type.isEmpty() && !ops.isEmpty();
    }
    
    static ChangeRequestDsl fromJson(const QJsonObject& json) {
        ChangeRequestDsl dsl;
        dsl.scope = json["scope"].toString();
        dsl.type = json["type"].toString();
        dsl.targetId = json["targetId"].toString();
        dsl.reason = json["reason"].toString();
        dsl.priority = json["priority"].toString("medium");
        
        QJsonArray opsArray = json["ops"].toArray();
        for (const auto& op : opsArray) {
            dsl.ops.append(ChangeRequestOp::fromJson(op.toObject()));
        }
        
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

#endif // CHANGEREQUEST_H
