#include <QtTest/QtTest>
#include "Storyboard.h"

class TestStoryboard : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testDefaultConstructor();
    void testSettersAndGetters();
    void testId();
    void testNovelId();
    void testChapterNumber();
    void testTotalPages();
    void testPanelCount();
    void testVersion();
    void testAllFieldsTogether();
};

void TestStoryboard::initTestCase()
{
    qDebug() << "========================================";
    qDebug() << "  Storyboard 模型单元测试开始";
    qDebug() << "========================================";
}

void TestStoryboard::cleanupTestCase()
{
    qDebug() << "========================================";
    qDebug() << "  Storyboard 模型单元测试结束";
    qDebug() << "========================================";
}

void TestStoryboard::testDefaultConstructor()
{
    Storyboard storyboard;
    
    QVERIFY(storyboard.id().isEmpty());
    QVERIFY(storyboard.novelId().isEmpty());
    QCOMPARE(storyboard.chapterNumber(), 1);
    QCOMPARE(storyboard.totalPages(), 0);
    QCOMPARE(storyboard.panelCount(), 0);
    QCOMPARE(storyboard.version(), 1);
}

void TestStoryboard::testSettersAndGetters()
{
    Storyboard storyboard;
    
    storyboard.setId("sb-001");
    QCOMPARE(storyboard.id(), QString("sb-001"));
    
    storyboard.setNovelId("novel-001");
    QCOMPARE(storyboard.novelId(), QString("novel-001"));
    
    storyboard.setChapterNumber(5);
    QCOMPARE(storyboard.chapterNumber(), 5);
    
    storyboard.setTotalPages(20);
    QCOMPARE(storyboard.totalPages(), 20);
    
    storyboard.setPanelCount(100);
    QCOMPARE(storyboard.panelCount(), 100);
    
    storyboard.setVersion(3);
    QCOMPARE(storyboard.version(), 3);
}

void TestStoryboard::testId()
{
    Storyboard storyboard;
    
    storyboard.setId("storyboard-test-id");
    QCOMPARE(storyboard.id(), QString("storyboard-test-id"));
    
    storyboard.setId("");
    QVERIFY(storyboard.id().isEmpty());
}

void TestStoryboard::testNovelId()
{
    Storyboard storyboard;
    
    storyboard.setNovelId("novel-test-id");
    QCOMPARE(storyboard.novelId(), QString("novel-test-id"));
    
    storyboard.setNovelId("");
    QVERIFY(storyboard.novelId().isEmpty());
}

void TestStoryboard::testChapterNumber()
{
    Storyboard storyboard;
    
    storyboard.setChapterNumber(1);
    QCOMPARE(storyboard.chapterNumber(), 1);
    
    storyboard.setChapterNumber(100);
    QCOMPARE(storyboard.chapterNumber(), 100);
    
    storyboard.setChapterNumber(0);
    QCOMPARE(storyboard.chapterNumber(), 0);
}

void TestStoryboard::testTotalPages()
{
    Storyboard storyboard;
    
    storyboard.setTotalPages(10);
    QCOMPARE(storyboard.totalPages(), 10);
    
    storyboard.setTotalPages(50);
    QCOMPARE(storyboard.totalPages(), 50);
    
    storyboard.setTotalPages(0);
    QCOMPARE(storyboard.totalPages(), 0);
}

void TestStoryboard::testPanelCount()
{
    Storyboard storyboard;
    
    storyboard.setPanelCount(25);
    QCOMPARE(storyboard.panelCount(), 25);
    
    storyboard.setPanelCount(200);
    QCOMPARE(storyboard.panelCount(), 200);
    
    storyboard.setPanelCount(0);
    QCOMPARE(storyboard.panelCount(), 0);
}

void TestStoryboard::testVersion()
{
    Storyboard storyboard;
    
    storyboard.setVersion(1);
    QCOMPARE(storyboard.version(), 1);
    
    storyboard.setVersion(10);
    QCOMPARE(storyboard.version(), 10);
    
    storyboard.setVersion(0);
    QCOMPARE(storyboard.version(), 0);
}

void TestStoryboard::testAllFieldsTogether()
{
    Storyboard storyboard;
    
    storyboard.setId("sb-integration");
    storyboard.setNovelId("novel-integration");
    storyboard.setChapterNumber(3);
    storyboard.setTotalPages(15);
    storyboard.setPanelCount(75);
    storyboard.setVersion(2);
    
    QCOMPARE(storyboard.id(), QString("sb-integration"));
    QCOMPARE(storyboard.novelId(), QString("novel-integration"));
    QCOMPARE(storyboard.chapterNumber(), 3);
    QCOMPARE(storyboard.totalPages(), 15);
    QCOMPARE(storyboard.panelCount(), 75);
    QCOMPARE(storyboard.version(), 2);
}

QTEST_APPLESS_MAIN(TestStoryboard)
#include "tst_storyboard.moc"
