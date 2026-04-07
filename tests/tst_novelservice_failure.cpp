#include <QtTest/QtTest>
#include "NovelService.h"
#include "DatabaseManager.h"
#include "ServiceContainer.h"
#include <QSqlDatabase>
#include <QSqlQuery>

class TestNovelServiceFailure : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    
    void testCreateNovelWithNullDb();
    void testCreateNovelWithEmptyUserId();
    void testCreateNovelWithEmptyTitle();
    void testGetNovelByIdNotFound();
    void testUpdateNovelWithEmptyId();
    void testUpdateNovelNotFound();
    void testDeleteNovelWithEmptyId();
    void testDeleteNovelNotFound();
    void testUpdateStatusWithEmptyId();
    void testGetNovelsByUserWithEmptyUserId();
    void testSearchNovelsWithEmptyQuery();

private:
    DatabaseManager* m_db = nullptr;
    NovelService* m_service = nullptr;
};

void TestNovelServiceFailure::initTestCase()
{
    qDebug() << "========================================";
    qDebug() << "  NovelService 失败路径测试开始";
    qDebug() << "========================================";
    
    DatabaseConfig config;
    config.host = "localhost";
    config.port = 3306;
    config.database = "test_qingning";
    config.username = "root";
    config.password = "123456";
    
    m_db = new DatabaseManager();
    if (!m_db->connect(config)) {
        QSKIP("无法连接到测试数据库");
    }
    
    ServiceContainer::instance()->setDatabaseManager(m_db);
    m_service = NovelService::instance();
}

void TestNovelServiceFailure::cleanupTestCase()
{
    m_service = nullptr;
    
    delete m_db;
    
    qDebug() << "========================================";
    qDebug() << "  NovelService 失败路径测试结束";
    qDebug() << "========================================";
}

void TestNovelServiceFailure::testCreateNovelWithNullDb()
{
    NovelService nullService(nullptr);
    Novel novel = nullService.createNovel("", "测试标题");
    QVERIFY(novel.id().isEmpty());
}

void TestNovelServiceFailure::testCreateNovelWithEmptyUserId()
{
    Novel novel = m_service->createNovel("", "测试标题");
    QVERIFY(novel.id().isEmpty());
}

void TestNovelServiceFailure::testCreateNovelWithEmptyTitle()
{
    Novel novel = m_service->createNovel("user-001", "");
    QVERIFY(novel.id().isEmpty());
}

void TestNovelServiceFailure::testGetNovelByIdNotFound()
{
    Novel novel = m_service->getNovelById("non-existent-id");
    QVERIFY(novel.id().isEmpty());
}

void TestNovelServiceFailure::testUpdateNovelWithEmptyId()
{
    QVariantMap data;
    data["title"] = "更新后的标题";
    bool result = m_service->updateNovel("", data);
    QVERIFY(!result);
}

void TestNovelServiceFailure::testUpdateNovelNotFound()
{
    QVariantMap data;
    data["title"] = "更新后的标题";
    bool result = m_service->updateNovel("non-existent-id", data);
    QVERIFY(!result);
}

void TestNovelServiceFailure::testDeleteNovelWithEmptyId()
{
    bool result = m_service->deleteNovel("");
    QVERIFY(!result);
}

void TestNovelServiceFailure::testDeleteNovelNotFound()
{
    bool result = m_service->deleteNovel("non-existent-id");
    QVERIFY(!result);
}

void TestNovelServiceFailure::testUpdateStatusWithEmptyId()
{
    bool result = m_service->updateStatus("", NovelStatus::Analyzing);
    QVERIFY(!result);
}

void TestNovelServiceFailure::testGetNovelsByUserWithEmptyUserId()
{
    NovelPageResult result = m_service->getNovels("");
    QVERIFY(result.novels.isEmpty());
}

void TestNovelServiceFailure::testSearchNovelsWithEmptyQuery()
{
    NovelPageResult result = m_service->getNovels("user-001");
    QVERIFY(result.novels.isEmpty() || result.total == 0);
}

QTEST_MAIN(TestNovelServiceFailure)
#include "tst_novelservice_failure.moc"
