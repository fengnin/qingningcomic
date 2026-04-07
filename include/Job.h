
#pragma once

#include <QString>
#include <QDateTime>
#include <QMetaType>

// 任务状态枚举
enum class JobStatus
{
    Pending,
    Running,
    Completed,
    Failed
};

class Job
{
    Q_GADGET
    Q_PROPERTY(QString id READ id WRITE setId)
    Q_PROPERTY(QString type READ type WRITE setType)
    Q_PROPERTY(JobStatus status READ status WRITE setStatus)
    Q_PROPERTY(QString novelId READ novelId WRITE setNovelId)
    Q_PROPERTY(QString storyboardId READ storyboardId WRITE setStoryboardId)
    Q_PROPERTY(int total READ total WRITE setTotal)
    Q_PROPERTY(int completed READ completed WRITE setCompleted)
    Q_PROPERTY(int failed READ failed WRITE setFailed)
    Q_PROPERTY(QDateTime createdAt READ createdAt WRITE setCreatedAt)
    Q_PROPERTY(QDateTime updatedAt READ updatedAt WRITE setUpdatedAt)

public:
    Job();
    ~Job() = default;

    QString id() const { return m_id; }
    void setId(const QString& id) { m_id = id; }

    QString type() const { return m_type; }
    void setType(const QString& type) { m_type = type; }

    JobStatus status() const { return m_status; }
    void setStatus(JobStatus status) { m_status = status; }

    QString novelId() const { return m_novelId; }
    void setNovelId(const QString& novelId) { m_novelId = novelId; }

    QString storyboardId() const { return m_storyboardId; }
    void setStoryboardId(const QString& storyboardId) { m_storyboardId = storyboardId; }

    int total() const { return m_total; }
    void setTotal(int total) { m_total = total; }

    int completed() const { return m_completed; }
    void setCompleted(int completed) { m_completed = completed; }

    int failed() const { return m_failed; }
    void setFailed(int failed) { m_failed = failed; }

    QDateTime createdAt() const { return m_createdAt; }
    void setCreatedAt(const QDateTime& dt) { m_createdAt = dt; }

    QDateTime updatedAt() const { return m_updatedAt; }
    void setUpdatedAt(const QDateTime& dt) { m_updatedAt = dt; }

    // 状态转换
    static QString statusToString(JobStatus status);
    static JobStatus stringToStatus(const QString& statusStr);

private:
    QString m_id;
    QString m_type;
    JobStatus m_status;
    QString m_novelId;
    QString m_storyboardId;
    int m_total;
    int m_completed;
    int m_failed;
    QDateTime m_createdAt;
    QDateTime m_updatedAt;
    
    // 状态名称查找表
    static const char* STATUS_NAMES[];
};

Q_DECLARE_METATYPE(Job)
Q_DECLARE_METATYPE(JobStatus)
