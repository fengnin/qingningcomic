#include <QtTest/QtTest>
#include "Novel.h"

class TestNovel : public QObject
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
    void testStatusString();
    void testToJson();
    void testFromJson();
    void testJsonRoundTrip();
    void testToVariantMap();
    void testFromVariantMap();
    void testVariantMapRoundTrip();
    void testChineseTitle();
};

void TestNovel::initTestCase()
{
    qDebug() << "========================================";
    qDebug() << "  Novel 模型单元测试开始";
    qDebug() << "========================================";
}

void TestNovel::cleanupTestCase()
{
    qDebug() << "========================================";
    qDebug() << "  Novel 模型单元测试结束";
    qDebug() << "========================================";
}

void TestNovel::testDefaultConstructor()
{
    Novel novel;
    
    QVERIFY(novel.id().isEmpty());
    QVERIFY(novel.userId().isEmpty());
    QVERIFY(novel.title().isEmpty());
    QCOMPARE(novel.status(), NovelStatus::Created);
    QVERIFY(novel.storyboardId().isEmpty());
    QVERIFY(novel.createdAt().isValid());
    QVERIFY(novel.updatedAt().isValid());
}

void TestNovel::testSettersAndGetters()
{
    Novel novel;
    
    novel.setId("novel-001");
    QCOMPARE(novel.id(), QString("novel-001"));
    
    novel.setUserId("user-001");
    QCOMPARE(novel.userId(), QString("user-001"));
    
    novel.setTitle(QString::fromUtf8("测试小说"));
    QCOMPARE(novel.title(), QString::fromUtf8("测试小说"));
    
    novel.setOriginalText("这是一段测试文本");
    QCOMPARE(novel.originalText(), QString("这是一段测试文本"));
    
    novel.setOriginalTextPath("/path/to/novel.txt");
    QCOMPARE(novel.originalTextPath(), QString("/path/to/novel.txt"));
    
    novel.setStatus(NovelStatus::Analyzing);
    QCOMPARE(novel.status(), NovelStatus::Analyzing);
    
    novel.setStoryboardId("storyboard-001");
    QCOMPARE(novel.storyboardId(), QString("storyboard-001"));
    
    QDateTime dt = QDateTime::fromString("2024-01-15T10:30:00", Qt::ISODate);
    novel.setCreatedAt(dt);
    QCOMPARE(novel.createdAt(), dt);
}

void TestNovel::testStatusToString()
{
    QCOMPARE(Novel::statusToString(NovelStatus::Created), QString("created"));
    QCOMPARE(Novel::statusToString(NovelStatus::Analyzing), QString("analyzing"));
    QCOMPARE(Novel::statusToString(NovelStatus::Analyzed), QString("analyzed"));
    QCOMPARE(Novel::statusToString(NovelStatus::Generating), QString("generating"));
    QCOMPARE(Novel::statusToString(NovelStatus::Completed), QString("completed"));
    QCOMPARE(Novel::statusToString(NovelStatus::Error), QString("error"));
}

void TestNovel::testStringToStatus()
{
    QCOMPARE(Novel::stringToStatus("created"), NovelStatus::Created);
    QCOMPARE(Novel::stringToStatus("analyzing"), NovelStatus::Analyzing);
    QCOMPARE(Novel::stringToStatus("analyzed"), NovelStatus::Analyzed);
    QCOMPARE(Novel::stringToStatus("generating"), NovelStatus::Generating);
    QCOMPARE(Novel::stringToStatus("completed"), NovelStatus::Completed);
    QCOMPARE(Novel::stringToStatus("error"), NovelStatus::Error);
}

void TestNovel::testStatusRoundTrip()
{
    NovelStatus statuses[] = {
        NovelStatus::Created, NovelStatus::Analyzing, NovelStatus::Analyzed,
        NovelStatus::Generating, NovelStatus::Completed, NovelStatus::Error
    };
    
    for (NovelStatus status : statuses) {
        QString str = Novel::statusToString(status);
        NovelStatus restored = Novel::stringToStatus(str);
        QCOMPARE(restored, status);
    }
}

void TestNovel::testStatusString()
{
    Novel novel;
    novel.setStatus(NovelStatus::Analyzing);
    QCOMPARE(novel.statusString(), QString("analyzing"));
    
    novel.setStatus(NovelStatus::Completed);
    QCOMPARE(novel.statusString(), QString("completed"));
}

void TestNovel::testToJson()
{
    Novel novel;
    novel.setId("novel-001");
    novel.setUserId("user-001");
    novel.setTitle(QString::fromUtf8("测试小说"));
    novel.setStatus(NovelStatus::Analyzing);
    
    QJsonObject json = novel.toJson();
    
    QCOMPARE(json["id"].toString(), QString("novel-001"));
    QCOMPARE(json["userId"].toString(), QString("user-001"));
    QCOMPARE(json["title"].toString(), QString::fromUtf8("测试小说"));
    QCOMPARE(json["status"].toString(), QString("analyzing"));
}

void TestNovel::testFromJson()
{
    QJsonObject json;
    json["id"] = "novel-002";
    json["userId"] = "user-002";
    json["title"] = QString::fromUtf8("JSON小说");
    json["status"] = "completed";
    json["storyboardId"] = "sb-001";
    
    Novel novel = Novel::fromJson(json);
    
    QCOMPARE(novel.id(), QString("novel-002"));
    QCOMPARE(novel.userId(), QString("user-002"));
    QCOMPARE(novel.title(), QString::fromUtf8("JSON小说"));
    QCOMPARE(novel.status(), NovelStatus::Completed);
    QCOMPARE(novel.storyboardId(), QString("sb-001"));
}

void TestNovel::testJsonRoundTrip()
{
    Novel original;
    original.setId("novel-003");
    original.setUserId("user-003");
    original.setTitle(QString::fromUtf8("往返测试小说"));
    original.setOriginalText("测试内容");
    original.setOriginalTextPath("/path/to/file.txt");
    original.setStatus(NovelStatus::Generating);
    original.setStoryboardId("sb-003");
    
    QJsonObject json = original.toJson();
    Novel restored = Novel::fromJson(json);
    
    QCOMPARE(restored.id(), original.id());
    QCOMPARE(restored.userId(), original.userId());
    QCOMPARE(restored.title(), original.title());
    QCOMPARE(restored.originalText(), original.originalText());
    QCOMPARE(restored.originalTextPath(), original.originalTextPath());
    QCOMPARE(restored.status(), original.status());
    QCOMPARE(restored.storyboardId(), original.storyboardId());
}

void TestNovel::testToVariantMap()
{
    Novel novel;
    novel.setId("novel-004");
    novel.setTitle(QString::fromUtf8("变体映射测试"));
    novel.setStatus(NovelStatus::Error);
    
    QVariantMap map = novel.toVariantMap();
    
    QCOMPARE(map["id"].toString(), QString("novel-004"));
    QCOMPARE(map["title"].toString(), QString::fromUtf8("变体映射测试"));
    QCOMPARE(map["status"].toString(), QString("error"));
}

void TestNovel::testFromVariantMap()
{
    QVariantMap map;
    map["id"] = "novel-005";
    map["user_id"] = "user-005";
    map["title"] = QString::fromUtf8("变体小说");
    map["status"] = "analyzed";
    
    Novel novel = Novel::fromVariantMap(map);
    
    QCOMPARE(novel.id(), QString("novel-005"));
    QCOMPARE(novel.userId(), QString("user-005"));
    QCOMPARE(novel.title(), QString::fromUtf8("变体小说"));
    QCOMPARE(novel.status(), NovelStatus::Analyzed);
}

void TestNovel::testVariantMapRoundTrip()
{
    Novel original;
    original.setId("novel-006");
    original.setUserId("user-006");
    original.setTitle(QString::fromUtf8("变体往返测试"));
    original.setStatus(NovelStatus::Completed);
    
    QVariantMap map = original.toVariantMap();
    Novel restored = Novel::fromVariantMap(map);
    
    QCOMPARE(restored.id(), original.id());
    QCOMPARE(restored.userId(), original.userId());
    QCOMPARE(restored.title(), original.title());
    QCOMPARE(restored.status(), original.status());
}

void TestNovel::testChineseTitle()
{
    Novel novel;
    QString chineseTitle = QString::fromUtf8("修仙传奇之无敌天下");
    novel.setTitle(chineseTitle);
    
    QCOMPARE(novel.title(), chineseTitle);
    
    QJsonObject json = novel.toJson();
    QCOMPARE(json["title"].toString(), chineseTitle);
    
    Novel restored = Novel::fromJson(json);
    QCOMPARE(restored.title(), chineseTitle);
}

QTEST_APPLESS_MAIN(TestNovel)
#include "tst_novel.moc"
