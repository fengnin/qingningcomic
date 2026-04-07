#include <QtTest/QtTest>
#include "QwenClient.h"
#include "api/QwenJsonRepair.h"
#include "api/QwenStoryboardMerger.h"
#include <QJsonObject>
#include <QJsonArray>

class TestQwenClientFailure : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    
    void testParseWithEmptyContent();
    void testParseWithInvalidJson();
    void testParseWithTruncatedJson();
    void testParseWithMissingColon();
    void testMergeWithEmptyList();
    void testMergeWithInvalidStoryboards();
    void testStripMarkdownCodeBlock();
    void testExtractCompleteArrayFromEmptyJson();
    void testExtractCompleteArrayFromInvalidJson();
    void testProcessChunksAllFail();
    void testProcessChunksPartialFail();

private:
    QwenClient* m_client = nullptr;
};

void TestQwenClientFailure::initTestCase()
{
    qDebug() << "========================================";
    qDebug() << "  QwenClient 失败路径测试开始";
    qDebug() << "========================================";
    
    QwenClient::Config config;
    config.apiKey = "test-api-key";
    config.model = "test-model";
    config.endpoint = "https://test.example.com";
    config.maxRetries = 1;
    
    m_client = new QwenClient(config);
}

void TestQwenClientFailure::cleanupTestCase()
{
    delete m_client;
    
    qDebug() << "========================================";
    qDebug() << "  QwenClient 失败路径测试结束";
    qDebug() << "========================================";
}

void TestQwenClientFailure::testParseWithEmptyContent()
{
    QJsonObject result = QwenJsonRepair::parseWithRepair("");
    QVERIFY(result.isEmpty());
}

void TestQwenClientFailure::testParseWithInvalidJson()
{
    QJsonObject result = QwenJsonRepair::parseWithRepair("这不是有效的JSON");
    QVERIFY(result.isEmpty());
}

void TestQwenClientFailure::testParseWithTruncatedJson()
{
    QString truncated = R"({"panels": [{"scene": "测试场景", "characters": [{"name": "李明"})";
    QJsonObject result = QwenJsonRepair::parseWithRepair(truncated);
    
    QVERIFY(!result.isEmpty());
    QVERIFY(result.contains("panels"));
}

void TestQwenClientFailure::testParseWithMissingColon()
{
    QString missingColon = R"({"panels" [{"scene": "测试场景"}]})";
    QJsonObject result = QwenJsonRepair::parseWithRepair(missingColon);
    
    QVERIFY(!result.isEmpty() || result.contains("panels"));
}

void TestQwenClientFailure::testMergeWithEmptyList()
{
    QJsonObject result = QwenStoryboardMerger::merge(QList<QJsonObject>());
    QVERIFY(result.isEmpty());
}

void TestQwenClientFailure::testMergeWithInvalidStoryboards()
{
    QList<QJsonObject> storyboards;
    storyboards.append(QJsonObject());
    storyboards.append(QJsonObject());
    
    QJsonObject result = QwenStoryboardMerger::merge(storyboards);
    QVERIFY(result.isEmpty() || !result.contains("panels"));
}

void TestQwenClientFailure::testStripMarkdownCodeBlock()
{
    QString withCodeBlock = "```json\n{\"key\": \"value\"}\n```";
    QString stripped = QwenJsonRepair::stripMarkdownCodeBlock(withCodeBlock);
    QVERIFY(!stripped.contains("```"));
    QVERIFY(stripped.contains("\"key\""));
}

void TestQwenClientFailure::testExtractCompleteArrayFromEmptyJson()
{
    QJsonArray result = QwenJsonRepair::extractCompleteArray("", "panels", 0);
    QVERIFY(result.isEmpty());
}

void TestQwenClientFailure::testExtractCompleteArrayFromInvalidJson()
{
    QString invalidJson = R"({"panels": "not an array"})";
    QJsonArray result = QwenJsonRepair::extractCompleteArray(invalidJson, "panels", 100);
    QVERIFY(result.isEmpty());
}

void TestQwenClientFailure::testProcessChunksAllFail()
{
    QStringList chunks;
    chunks << "chunk1" << "chunk2" << "chunk3";
    
    QJsonObject schema;
    schema["type"] = "object";
    
    QList<QJsonObject> results = m_client->processChunksParallel(
        chunks, schema, true, QJsonArray(), QJsonArray(), 1);
    
    QVERIFY(results.isEmpty());
}

void TestQwenClientFailure::testProcessChunksPartialFail()
{
    QSKIP("需要模拟网络请求");
}

QTEST_MAIN(TestQwenClientFailure)
#include "tst_qwenclient_failure.moc"
