#include <QtTest/QtTest>
#include "StoryboardService.h"
#include "DatabaseManager.h"
#include "ServiceContainer.h"
#include <QSqlDatabase>
#include <QSqlQuery>

class TestStoryboardServiceFailure : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    
    void testGetStoryboardByIdNotFound();
    void testGetStoryboardByNovelNotFound();
    void testSaveStoryboardWithEmptyNovelId();
    void testSaveStoryboardWithEmptyPanels();
    void testUpdatePanelWithEmptyId();
    void testUpdatePanelNotFound();
    void testDeletePanelWithEmptyId();
    void testGetPanelsByStoryboardNotFound();
    void testSavePanelImageWithEmptyPanelId();

private:
    DatabaseManager* m_db = nullptr;
    StoryboardService* m_service = nullptr;
};

void TestStoryboardServiceFailure::initTestCase()
{
    qDebug() << "========================================";
    qDebug() << "  StoryboardService 失败路径测试开始";
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
    m_service = StoryboardService::instance();
}

void TestStoryboardServiceFailure::cleanupTestCase()
{
    m_service = nullptr;
    
    delete m_db;
    
    qDebug() << "========================================";
    qDebug() << "  StoryboardService 失败路径测试结束";
    qDebug() << "========================================";
}

void TestStoryboardServiceFailure::testGetStoryboardByIdNotFound()
{
    Storyboard storyboard = m_service->getStoryboardById("non-existent-id");
    QVERIFY(storyboard.id().isEmpty());
}

void TestStoryboardServiceFailure::testGetStoryboardByNovelNotFound()
{
    Storyboard storyboard = m_service->getStoryboardByNovel("non-existent-novel-id", 1);
    QVERIFY(storyboard.id().isEmpty());
}

void TestStoryboardServiceFailure::testSaveStoryboardWithEmptyNovelId()
{
    QJsonObject data;
    data["panels"] = QJsonArray();
    
    bool result = m_service->saveStoryboard("", data, 1);
    QVERIFY(!result);
}

void TestStoryboardServiceFailure::testSaveStoryboardWithEmptyPanels()
{
    QJsonObject data;
    data["panels"] = QJsonArray();
    
    bool result = m_service->saveStoryboard("novel-001", data, 1);
    QVERIFY(!result);
}

void TestStoryboardServiceFailure::testUpdatePanelWithEmptyId()
{
    QJsonObject content;
    content["scene"] = "测试场景";
    
    bool result = m_service->updatePanel("", content);
    QVERIFY(!result);
}

void TestStoryboardServiceFailure::testUpdatePanelNotFound()
{
    QJsonObject content;
    content["scene"] = "测试场景";
    
    bool result = m_service->updatePanel("non-existent-panel-id", content);
    QVERIFY(!result);
}

void TestStoryboardServiceFailure::testDeletePanelWithEmptyId()
{
    bool result = m_service->deletePanel("");
    QVERIFY(!result);
}

void TestStoryboardServiceFailure::testGetPanelsByStoryboardNotFound()
{
    QList<Panel> panels = m_service->getPanelsByStoryboard("non-existent-storyboard-id");
    QVERIFY(panels.isEmpty());
}

void TestStoryboardServiceFailure::testSavePanelImageWithEmptyPanelId()
{
    bool result = m_service->savePanelImage("", "preview", "test-image-key");
    QVERIFY(!result);
}

QTEST_MAIN(TestStoryboardServiceFailure)
#include "tst_storyboardservice_failure.moc"
