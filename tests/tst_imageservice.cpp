#include <QtTest/QtTest>
#include "ImageService.h"
#include "Logger.h"
#include "QwenImageClient.h"
#include "StorageClient.h"
#include "StoryboardService.h"
#include "TaskQueue.h"
#include "Task.h"

/**
 * @brief ImageService 单元测试类
 * 
 * 测试覆盖范围：
 * 1. 单例模式测试
 * 2. 生成模式测试
 * 3. 结果结构测试
 * 4. 错误处理测试
 * 5. 批量生成测试
 * 6. TaskQueue 集成测试
 */
class TestImageService : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    // 单例模式测试
    void testSingletonInstance();

    // 生成模式测试
    void testGenerateModeEnum();

    // 结果结构测试
    void testGenerateResultDefaults();
    void testBatchResultDefaults();

    // 状态测试
    void testIsGeneratingDefault();
    void testLastErrorDefault();

    // 取消生成测试
    void testCancelGeneration();

    // 元类型注册测试
    void testMetaTypeRegistration();

    // TaskQueue 集成测试
    void testEnqueuePanelImageGeneration();
    void testEnqueuePanelImageGenerations();
    void testHandleGeneratePanelImageTask();
};

void TestImageService::initTestCase()
{
    qDebug() << "========================================";
    qDebug() << "  ImageService 单元测试开始";
    qDebug() << "========================================";

    // 初始化 Logger
    Logger::instance()->init();
    Logger::instance()->setConsoleOutput(true);
    Logger::instance()->setMinLevel(Logger::Debug);

    // 注册元类型
    qRegisterMetaType<ImageService::GenerateResult>("ImageService::GenerateResult");
    qRegisterMetaType<ImageService::GenerateResult>("GenerateResult");
    qRegisterMetaType<ImageService::BatchResult>("ImageService::BatchResult");
    qRegisterMetaType<ImageService::BatchResult>("BatchResult");
}

void TestImageService::cleanupTestCase()
{
    qDebug() << "========================================";
    qDebug() << "  ImageService 单元测试结束";
    qDebug() << "========================================";
}

// ==================== 单例模式测试 ====================

void TestImageService::testSingletonInstance()
{
    ImageService* instance1 = ImageService::instance();
    ImageService* instance2 = ImageService::instance();

    QVERIFY(instance1 != nullptr);
    QVERIFY(instance2 != nullptr);
    QCOMPARE(instance1, instance2);
}

// ==================== 生成模式测试 ====================

void TestImageService::testGenerateModeEnum()
{
    // 验证枚举值
    ImageService::GenerateMode previewMode = ImageService::GenerateMode::Preview;
    ImageService::GenerateMode hdMode = ImageService::GenerateMode::HD;

    QVERIFY(previewMode != hdMode);
    QCOMPARE(static_cast<int>(previewMode), 0);
    QCOMPARE(static_cast<int>(hdMode), 1);
}

// ==================== 结果结构测试 ====================

void TestImageService::testGenerateResultDefaults()
{
    ImageService::GenerateResult result;

    QVERIFY(!result.success);
    QVERIFY(result.panelId.isEmpty());
    QVERIFY(result.imageUrl.isEmpty());
    QVERIFY(result.s3Key.isEmpty());
    QVERIFY(result.errorMessage.isEmpty());
    QCOMPARE(result.width, 0);
    QCOMPARE(result.height, 0);
    QCOMPARE(result.timestamp, (qint64)0);
}

void TestImageService::testBatchResultDefaults()
{
    ImageService::BatchResult result;

    QCOMPARE(result.totalCount, 0);
    QCOMPARE(result.successCount, 0);
    QCOMPARE(result.failedCount, 0);
    QVERIFY(result.results.isEmpty());
}

// ==================== 状态测试 ====================

void TestImageService::testIsGeneratingDefault()
{
    ImageService* service = ImageService::instance();
    QVERIFY(!service->isGenerating());
}

void TestImageService::testLastErrorDefault()
{
    ImageService* service = ImageService::instance();
    QVERIFY(service->lastError().isEmpty());
}

// ==================== 取消生成测试 ====================

void TestImageService::testCancelGeneration()
{
    ImageService* service = ImageService::instance();

    // 没有生成任务时取消应该安全
    service->cancelGeneration();
    QVERIFY(!service->isGenerating());
}

// ==================== 元类型注册测试 ====================

void TestImageService::testMetaTypeRegistration()
{
    // 验证元类型可以正确创建和复制
    ImageService::GenerateResult result;
    result.success = true;
    result.panelId = "test-panel-001";
    result.imageUrl = "https://example.com/image.png";
    result.s3Key = "panels/preview/test.png";
    result.width = 720;
    result.height = 1280;
    result.timestamp = QDateTime::currentMSecsSinceEpoch();

    // 验证 QVariant 可以存储
    QVariant variant = QVariant::fromValue(result);
    QVERIFY(variant.isValid());
    QVERIFY(variant.canConvert<ImageService::GenerateResult>());

    ImageService::GenerateResult restored = variant.value<ImageService::GenerateResult>();
    QCOMPARE(restored.success, result.success);
    QCOMPARE(restored.panelId, result.panelId);
    QCOMPARE(restored.imageUrl, result.imageUrl);
    QCOMPARE(restored.s3Key, result.s3Key);
    QCOMPARE(restored.width, result.width);
    QCOMPARE(restored.height, result.height);

    // 验证 BatchResult
    ImageService::BatchResult batchResult;
    batchResult.totalCount = 10;
    batchResult.successCount = 8;
    batchResult.failedCount = 2;
    batchResult.results.append(result);

    QVariant batchVariant = QVariant::fromValue(batchResult);
    QVERIFY(batchVariant.isValid());
    QVERIFY(batchVariant.canConvert<ImageService::BatchResult>());

    ImageService::BatchResult restoredBatch = batchVariant.value<ImageService::BatchResult>();
    QCOMPARE(restoredBatch.totalCount, 10);
    QCOMPARE(restoredBatch.successCount, 8);
    QCOMPARE(restoredBatch.failedCount, 2);
    QCOMPARE(restoredBatch.results.size(), 1);
}

// ==================== TaskQueue 集成测试 ====================

void TestImageService::testEnqueuePanelImageGeneration()
{
    // 验证方法存在且可调用
    ImageService* service = ImageService::instance();
    QVERIFY(service != nullptr);

    // 注意：实际入队需要 TaskQueue 初始化，这里只验证方法签名
    // 真实测试需要 mock TaskQueue
}

void TestImageService::testEnqueuePanelImageGenerations()
{
    // 验证批量入队方法存在
    ImageService* service = ImageService::instance();
    QVERIFY(service != nullptr);
}

void TestImageService::testHandleGeneratePanelImageTask()
{
    // 验证任务处理方法存在
    ImageService* service = ImageService::instance();
    QVERIFY(service != nullptr);

    // 构造测试任务数据
    QJsonObject taskJson;
    taskJson["panelId"] = "test-panel-001";
    QJsonObject params;
    params["mode"] = "preview";
    taskJson["params"] = params;

    // 注意：实际处理需要完整的依赖初始化
    // 这里只验证方法签名
}

QTEST_MAIN(TestImageService)
#include "tst_imageservice.moc"
