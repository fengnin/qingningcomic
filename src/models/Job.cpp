
#include "Job.h"

// 状态名称查找表
const char* Job::STATUS_NAMES[] = {
    "pending",    // JobStatus::Pending = 0
    "running",    // JobStatus::Running = 1
    "completed",  // JobStatus::Completed = 2
    "failed"      // JobStatus::Failed = 3
};

Job::Job()
    : m_status(JobStatus::Pending)
    , m_total(0)
    , m_completed(0)
    , m_failed(0)
    , m_createdAt(QDateTime::currentDateTime())
    , m_updatedAt(QDateTime::currentDateTime())
{
}

QString Job::statusToString(JobStatus status)
{
    int index = static_cast<int>(status);
    return (index >= 0 && index <= 3) ? QString::fromLatin1(STATUS_NAMES[index]) : QStringLiteral("unknown");
}

JobStatus Job::stringToStatus(const QString& statusStr)
{
    for (int i = 0; i <= 3; ++i) {
        if (statusStr == QString::fromLatin1(STATUS_NAMES[i])) {
            return static_cast<JobStatus>(i);
        }
    }
    return JobStatus::Pending;
}
