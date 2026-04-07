#include <QtTest/QtTest>
#include "Task.h"

class TestTask : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testTaskDataTypeString();
    void testTaskDataStatusString();
    void testTaskDataProgress();
    void testTaskDataProgressZero();
    void testTaskDataProgressPartial();
    void testTaskDataCanRetry();
    void testTaskDataCanRetryExceeded();
    void testTaskHelperTypeFromString();
    void testTaskHelperTypeFromStringInvalid();
    void testTaskHelperStatusFromString();
    void testTaskHelperStatusFromStringInvalid();
    void testTaskTypeRoundTrip();
    void testTaskStatusRoundTrip();
    void testTaskDataDefaultValues();
};

void TestTask::initTestCase()
{
    qDebug() << "========================================";
    qDebug() << "  Task 模型单元测试开始";
    qDebug() << "========================================";
}

void TestTask::cleanupTestCase()
{
    qDebug() << "========================================";
    qDebug() << "  Task 模型单元测试结束";
    qDebug() << "========================================";
}

void TestTask::testTaskDataTypeString()
{
    TaskData data;
    
    data.type = TaskType::GenerateStoryboard;
    QCOMPARE(data.typeString(), QString("generate_storyboard"));
    
    data.type = TaskType::GeneratePanels;
    QCOMPARE(data.typeString(), QString("generate_panels"));
    
    data.type = TaskType::ExportPdf;
    QCOMPARE(data.typeString(), QString("export_pdf"));
}

void TestTask::testTaskDataStatusString()
{
    TaskData data;
    
    data.status = TaskStatus::Pending;
    QCOMPARE(data.statusString(), QString("pending"));
    
    data.status = TaskStatus::Running;
    QCOMPARE(data.statusString(), QString("running"));
    
    data.status = TaskStatus::Completed;
    QCOMPARE(data.statusString(), QString("completed"));
    
    data.status = TaskStatus::Failed;
    QCOMPARE(data.statusString(), QString("failed"));
    
    data.status = TaskStatus::Cancelled;
    QCOMPARE(data.statusString(), QString("cancelled"));
}

void TestTask::testTaskDataProgress()
{
    TaskData data;
    data.total = 100;
    data.completed = 50;
    
    QCOMPARE(data.progress(), 50);
    
    data.completed = 75;
    QCOMPARE(data.progress(), 75);
    
    data.completed = 100;
    QCOMPARE(data.progress(), 100);
}

void TestTask::testTaskDataProgressZero()
{
    TaskData data;
    data.total = 0;
    data.completed = 0;
    
    QCOMPARE(data.progress(), 0);
}

void TestTask::testTaskDataProgressPartial()
{
    TaskData data;
    data.total = 3;
    data.completed = 1;
    
    QCOMPARE(data.progress(), 33);
    
    data.completed = 2;
    QCOMPARE(data.progress(), 66);
}

void TestTask::testTaskDataCanRetry()
{
    TaskData data;
    data.retryCount = 0;
    data.maxRetries = 3;
    
    QVERIFY(data.canRetry());
    
    data.retryCount = 1;
    QVERIFY(data.canRetry());
    
    data.retryCount = 2;
    QVERIFY(data.canRetry());
}

void TestTask::testTaskDataCanRetryExceeded()
{
    TaskData data;
    data.retryCount = 3;
    data.maxRetries = 3;
    
    QVERIFY(!data.canRetry());
    
    data.retryCount = 5;
    QVERIFY(!data.canRetry());
}

void TestTask::testTaskHelperTypeFromString()
{
    QCOMPARE(TaskHelper::typeFromString("generate_storyboard"), TaskType::GenerateStoryboard);
    QCOMPARE(TaskHelper::typeFromString("generate_panels"), TaskType::GeneratePanels);
    QCOMPARE(TaskHelper::typeFromString("export_pdf"), TaskType::ExportPdf);
}

void TestTask::testTaskHelperTypeFromStringInvalid()
{
    QCOMPARE(TaskHelper::typeFromString("invalid"), TaskType::GenerateStoryboard);
    QCOMPARE(TaskHelper::typeFromString(""), TaskType::GenerateStoryboard);
    QCOMPARE(TaskHelper::typeFromString("unknown_type"), TaskType::GenerateStoryboard);
}

void TestTask::testTaskHelperStatusFromString()
{
    QCOMPARE(TaskHelper::statusFromString("pending"), TaskStatus::Pending);
    QCOMPARE(TaskHelper::statusFromString("running"), TaskStatus::Running);
    QCOMPARE(TaskHelper::statusFromString("completed"), TaskStatus::Completed);
    QCOMPARE(TaskHelper::statusFromString("failed"), TaskStatus::Failed);
    QCOMPARE(TaskHelper::statusFromString("cancelled"), TaskStatus::Cancelled);
}

void TestTask::testTaskHelperStatusFromStringInvalid()
{
    QCOMPARE(TaskHelper::statusFromString("invalid"), TaskStatus::Pending);
    QCOMPARE(TaskHelper::statusFromString(""), TaskStatus::Pending);
    QCOMPARE(TaskHelper::statusFromString("unknown"), TaskStatus::Pending);
}

void TestTask::testTaskTypeRoundTrip()
{
    TaskType types[] = {TaskType::GenerateStoryboard, TaskType::GeneratePanels, TaskType::ExportPdf};
    
    for (TaskType type : types) {
        TaskData data;
        data.type = type;
        QString str = data.typeString();
        TaskType restored = TaskHelper::typeFromString(str);
        QCOMPARE(restored, type);
    }
}

void TestTask::testTaskStatusRoundTrip()
{
    TaskStatus statuses[] = {TaskStatus::Pending, TaskStatus::Running, TaskStatus::Completed, TaskStatus::Failed, TaskStatus::Cancelled};
    
    for (TaskStatus status : statuses) {
        TaskData data;
        data.status = status;
        QString str = data.statusString();
        TaskStatus restored = TaskHelper::statusFromString(str);
        QCOMPARE(restored, status);
    }
}

void TestTask::testTaskDataDefaultValues()
{
    TaskData data;
    
    QVERIFY(data.id.isEmpty());
    QCOMPARE(data.type, TaskType::GenerateStoryboard);
    QCOMPARE(data.status, TaskStatus::Pending);
    QVERIFY(data.novelId.isEmpty());
    QVERIFY(data.storyboardId.isEmpty());
    QCOMPARE(data.chapterNumber, 1);
    QVERIFY(data.text.isEmpty());
    QCOMPARE(data.total, 0);
    QCOMPARE(data.completed, 0);
    QVERIFY(data.errorMessage.isEmpty());
    QCOMPARE(data.retryCount, 0);
    QCOMPARE(data.maxRetries, 3);
}

QTEST_APPLESS_MAIN(TestTask)
#include "tst_task.moc"
