#include <QtTest/QtTest>
#include "Job.h"

class TestJob : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testDefaultConstructor();
    void testSettersAndGetters();
    void testStatusToString();
    void testStringToStatus();
    void testStatusRoundTrip();
    void testStatusToStringInvalid();
    void testStringToStatusInvalid();
};

void TestJob::initTestCase()
{
    qDebug() << "========================================";
    qDebug() << "  Job 模型单元测试开始";
    qDebug() << "========================================";
}

void TestJob::cleanupTestCase()
{
    qDebug() << "========================================";
    qDebug() << "  Job 模型单元测试结束";
    qDebug() << "========================================";
}

void TestJob::testDefaultConstructor()
{
    Job job;
    
    QVERIFY(job.id().isEmpty());
    QVERIFY(job.type().isEmpty());
    QCOMPARE(job.status(), JobStatus::Pending);
    QVERIFY(job.novelId().isEmpty());
    QVERIFY(job.storyboardId().isEmpty());
    QCOMPARE(job.total(), 0);
    QCOMPARE(job.completed(), 0);
    QCOMPARE(job.failed(), 0);
    QVERIFY(job.createdAt().isValid());
    QVERIFY(job.updatedAt().isValid());
}

void TestJob::testSettersAndGetters()
{
    Job job;
    
    job.setId("job-001");
    QCOMPARE(job.id(), QString("job-001"));
    
    job.setType("generate_storyboard");
    QCOMPARE(job.type(), QString("generate_storyboard"));
    
    job.setStatus(JobStatus::Running);
    QCOMPARE(job.status(), JobStatus::Running);
    
    job.setNovelId("novel-001");
    QCOMPARE(job.novelId(), QString("novel-001"));
    
    job.setStoryboardId("storyboard-001");
    QCOMPARE(job.storyboardId(), QString("storyboard-001"));
    
    job.setTotal(100);
    QCOMPARE(job.total(), 100);
    
    job.setCompleted(50);
    QCOMPARE(job.completed(), 50);
    
    job.setFailed(5);
    QCOMPARE(job.failed(), 5);
    
    QDateTime dt = QDateTime::fromString("2024-01-15T10:30:00", Qt::ISODate);
    job.setCreatedAt(dt);
    QCOMPARE(job.createdAt(), dt);
    
    job.setUpdatedAt(dt);
    QCOMPARE(job.updatedAt(), dt);
}

void TestJob::testStatusToString()
{
    QCOMPARE(Job::statusToString(JobStatus::Pending), QString("pending"));
    QCOMPARE(Job::statusToString(JobStatus::Running), QString("running"));
    QCOMPARE(Job::statusToString(JobStatus::Completed), QString("completed"));
    QCOMPARE(Job::statusToString(JobStatus::Failed), QString("failed"));
}

void TestJob::testStringToStatus()
{
    QCOMPARE(Job::stringToStatus("pending"), JobStatus::Pending);
    QCOMPARE(Job::stringToStatus("running"), JobStatus::Running);
    QCOMPARE(Job::stringToStatus("completed"), JobStatus::Completed);
    QCOMPARE(Job::stringToStatus("failed"), JobStatus::Failed);
}

void TestJob::testStatusRoundTrip()
{
    JobStatus statuses[] = {JobStatus::Pending, JobStatus::Running, JobStatus::Completed, JobStatus::Failed};
    
    for (JobStatus status : statuses) {
        QString str = Job::statusToString(status);
        JobStatus restored = Job::stringToStatus(str);
        QCOMPARE(restored, status);
    }
}

void TestJob::testStatusToStringInvalid()
{
    JobStatus invalidStatus = static_cast<JobStatus>(999);
    QString result = Job::statusToString(invalidStatus);
    QCOMPARE(result, QString("unknown"));
}

void TestJob::testStringToStatusInvalid()
{
    QCOMPARE(Job::stringToStatus("invalid"), JobStatus::Pending);
    QCOMPARE(Job::stringToStatus(""), JobStatus::Pending);
    QCOMPARE(Job::stringToStatus("PENDING"), JobStatus::Pending);
}

QTEST_APPLESS_MAIN(TestJob)
#include "tst_job.moc"
