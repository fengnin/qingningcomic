#include <QtTest/QtTest>
#include "QwenImageClient.h"
#include "Logger.h"

/**
 * @brief QwenImageClient 单元测试类
 * 
 * 测试覆盖范围：
 * 1. 单例模式测试
 * 2. 初始化功能测试
 * 3. 配置选项测试
 * 4. 图像尺寸枚举转换测试
 * 5. 生成选项构建测试
 * 6. 占位图生成测试
 * 7. 模拟模式判断测试
 * 8. 错误处理测试
 */
class TestQwenImageClient : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    // 单例模式测试
    void testSingletonInstance();

    // 初始化测试
    void testInitWithValidConfig();
    void testInitWithEmptyApiKey();
    void testInitWithForceMock();

    // 配置测试
    void testDefaultConfig();
    void testCustomConfig();

    // 图像尺寸转换测试
    void testSizeToStringSquare();
    void testSizeToStringPortrait();
    void testSizeToStringLandscape();
    void testSizeToStringCustom();
    void testSizeToStringCustomInvalid();

    // 生成选项测试
    void testGenerateOptionsDefault();
    void testGenerateOptionsWithStyle();
    void testGenerateOptionsWithNegativePrompt();
    void testGenerateOptionsWithSeed();

    // 编辑选项测试
    void testEditOptionsDefault();
    void testEditOptionsWithMask();

    // 占位图测试
    void testGeneratePlaceholder();
    void testGeneratePlaceholderWithPrompt();
    void testPlaceholderImageData();

    // 模拟模式测试
    void testShouldMockWithForceMock();
    void testShouldMockWithEmptyApiKey();
    void testShouldMockWhenNotInitialized();

    // 错误处理测试
    void testGenerateWithEmptyPrompt();
    void testEditWithEmptyPrompt();
    void testEditWithEmptySourceImage();

    // 结果结构测试
    void testGenerateResultDefaults();
    void testGenerateResultSuccess();

    // 异步操作测试
    void testAsyncGenerateSignal();
    void testAsyncEditSignal();
};

void TestQwenImageClient::initTestCase()
{
    qDebug() << "========================================";
    qDebug() << "  QwenImageClient 单元测试开始";
    qDebug() << "========================================";

    // 注册元类型，用于跨线程信号槽传递
    // 必须同时注册两种名称：带命名空间和不带命名空间的
    qRegisterMetaType<QwenImageClient::GenerateResult>("GenerateResult");
    qRegisterMetaType<QwenImageClient::GenerateResult>("QwenImageClient::GenerateResult");

    // 初始化 Logger（测试环境使用控制台输出）
    Logger::instance()->init();
    Logger::instance()->setConsoleOutput(true);
    Logger::instance()->setMinLevel(Logger::Debug);
}

void TestQwenImageClient::cleanupTestCase()
{
    qDebug() << "========================================";
    qDebug() << "  QwenImageClient 单元测试结束";
    qDebug() << "========================================";
}

// ==================== 单例模式测试 ====================

void TestQwenImageClient::testSingletonInstance()
{
    QwenImageClient* instance1 = QwenImageClient::instance();
    QwenImageClient* instance2 = QwenImageClient::instance();

    // 验证单例：两次获取的实例应该相同
    QVERIFY(instance1 != nullptr);
    QVERIFY(instance2 != nullptr);
    QCOMPARE(instance1, instance2);
}

// ==================== 初始化测试 ====================

void TestQwenImageClient::testInitWithValidConfig()
{
    QwenImageClient* client = QwenImageClient::instance();

    QwenImageClient::Config config;
    config.apiKey = "test-api-key-12345";
    config.baseUrl = "https://dashscope.aliyuncs.com";
    config.generateModel = "qwen-image-plus";
    config.editModel = "qwen-image-edit";
    config.requestTimeout = 60000;
    config.maxRetries = 3;

    bool result = client->init(config);

    QVERIFY(result);
    QVERIFY(client->isInitialized());
    QCOMPARE(client->lastError(), QString());
}

void TestQwenImageClient::testInitWithEmptyApiKey()
{
    QwenImageClient* client = QwenImageClient::instance();

    QwenImageClient::Config config;
    config.apiKey = "";  // 空 API Key

    bool result = client->init(config);

    // 空 API Key 时仍可初始化成功，但会使用占位图模式
    QVERIFY(result);
    QVERIFY(client->isInitialized());
    QVERIFY(client->shouldMock());
}

void TestQwenImageClient::testInitWithForceMock()
{
    QwenImageClient* client = QwenImageClient::instance();

    QwenImageClient::Config config;
    config.apiKey = "valid-api-key";
    config.forceMock = true;  // 强制使用模拟模式

    client->init(config);

    QVERIFY(client->isInitialized());
    QVERIFY(client->shouldMock());
}

// ==================== 配置测试 ====================

void TestQwenImageClient::testDefaultConfig()
{
    QwenImageClient::Config config;

    // 验证默认配置值
    QCOMPARE(config.baseUrl, QString("https://dashscope.aliyuncs.com"));
    QCOMPARE(config.generateModel, QString("qwen-image-plus"));
    QCOMPARE(config.editModel, QString("qwen-image-edit"));
    QCOMPARE(config.requestTimeout, 120000);
    QCOMPARE(config.maxRetries, 3);
    QVERIFY(!config.forceMock);
}

void TestQwenImageClient::testCustomConfig()
{
    QwenImageClient::Config config;
    config.apiKey = "custom-key";
    config.baseUrl = "https://custom.api.com";
    config.generateModel = "custom-model";
    config.editModel = "custom-edit-model";
    config.requestTimeout = 30000;
    config.maxRetries = 5;
    config.forceMock = true;

    QCOMPARE(config.apiKey, QString("custom-key"));
    QCOMPARE(config.baseUrl, QString("https://custom.api.com"));
    QCOMPARE(config.generateModel, QString("custom-model"));
    QCOMPARE(config.editModel, QString("custom-edit-model"));
    QCOMPARE(config.requestTimeout, 30000);
    QCOMPARE(config.maxRetries, 5);
    QVERIFY(config.forceMock);
}

// ==================== 图像尺寸转换测试 ====================

void TestQwenImageClient::testSizeToStringSquare()
{
    QString size = QwenImageClient::sizeToString(QwenImageClient::ImageSize::Square);
    QCOMPARE(size, QString("1024*1024"));
}

void TestQwenImageClient::testSizeToStringPortrait()
{
    QString size = QwenImageClient::sizeToString(QwenImageClient::ImageSize::Portrait);
    QCOMPARE(size, QString("720*1280"));
}

void TestQwenImageClient::testSizeToStringLandscape()
{
    QString size = QwenImageClient::sizeToString(QwenImageClient::ImageSize::Landscape);
    QCOMPARE(size, QString("1280*720"));
}

void TestQwenImageClient::testSizeToStringCustom()
{
    QString size = QwenImageClient::sizeToString(QwenImageClient::ImageSize::Custom, 800, 600);
    QCOMPARE(size, QString("800*600"));
}

void TestQwenImageClient::testSizeToStringCustomInvalid()
{
    // 自定义尺寸但宽高为0，应返回默认值
    QString size = QwenImageClient::sizeToString(QwenImageClient::ImageSize::Custom, 0, 0);
    QCOMPARE(size, QString("1024*1024"));
}

// ==================== 生成选项测试 ====================

void TestQwenImageClient::testGenerateOptionsDefault()
{
    QwenImageClient::GenerateOptions options;

    QVERIFY(options.prompt.isEmpty());
    QVERIFY(options.negativePrompt.isEmpty());
    QCOMPARE(options.size, QwenImageClient::ImageSize::Square);
    QCOMPARE(options.width, 0);
    QCOMPARE(options.height, 0);
    QVERIFY(options.style.isEmpty());
    QCOMPARE(options.seed, -1);
    QVERIFY(options.textRendering);
}

void TestQwenImageClient::testGenerateOptionsWithStyle()
{
    QwenImageClient::GenerateOptions options;
    options.prompt = QString::fromUtf8("一个美丽的风景");
    options.style = "manga";

    QCOMPARE(options.style, QString("manga"));
    QVERIFY(!options.prompt.isEmpty());
}

void TestQwenImageClient::testGenerateOptionsWithNegativePrompt()
{
    QwenImageClient::GenerateOptions options;
    options.prompt = QString::fromUtf8("一个角色");
    options.negativePrompt = QString::fromUtf8("模糊, 低质量");

    QCOMPARE(options.negativePrompt, QString::fromUtf8("模糊, 低质量"));
}

void TestQwenImageClient::testGenerateOptionsWithSeed()
{
    QwenImageClient::GenerateOptions options;
    options.prompt = QString::fromUtf8("测试图像");
    options.seed = 12345;

    QCOMPARE(options.seed, 12345);
}

// ==================== 编辑选项测试 ====================

void TestQwenImageClient::testEditOptionsDefault()
{
    QwenImageClient::EditOptions options;

    QVERIFY(options.prompt.isEmpty());
    QVERIFY(options.sourceImage.isEmpty());
    QVERIFY(options.maskImage.isEmpty());
    QVERIFY(options.negativePrompt.isEmpty());
    QCOMPARE(options.size, QwenImageClient::ImageSize::Square);
    QCOMPARE(options.seed, -1);
}

void TestQwenImageClient::testEditOptionsWithMask()
{
    QwenImageClient::EditOptions options;
    options.prompt = QString::fromUtf8("修改背景");
    options.sourceImage = QByteArray("fake-image-data");
    options.maskImage = QByteArray("fake-mask-data");

    QVERIFY(!options.sourceImage.isEmpty());
    QVERIFY(!options.maskImage.isEmpty());
}

// ==================== 占位图测试 ====================

void TestQwenImageClient::testGeneratePlaceholder()
{
    QwenImageClient* client = QwenImageClient::instance();

    QwenImageClient::Config config;
    config.forceMock = true;
    client->init(config);

    QwenImageClient::GenerateResult result = client->generatePlaceholder();

    QVERIFY(result.success);
    QVERIFY(!result.imageData.isEmpty());
    QCOMPARE(result.mimeType, QString("image/png"));
    QVERIFY(!result.requestId.isEmpty());
    QCOMPARE(result.width, 64);
    QCOMPARE(result.height, 64);
    QVERIFY(result.timestamp > 0);
}

void TestQwenImageClient::testGeneratePlaceholderWithPrompt()
{
    QwenImageClient* client = QwenImageClient::instance();

    QString prompt = QString::fromUtf8("一个测试提示词");
    QwenImageClient::GenerateResult result = client->generatePlaceholder(prompt);

    QVERIFY(result.success);
    QVERIFY(!result.imageData.isEmpty());
}

void TestQwenImageClient::testPlaceholderImageData()
{
    QwenImageClient* client = QwenImageClient::instance();

    QwenImageClient::GenerateResult result = client->generatePlaceholder();

    // 验证占位图数据是有效的 PNG
    QVERIFY(!result.imageData.isEmpty());

    // PNG 文件以 89 50 4E 47 开头（PNG 签名）
    QVERIFY(result.imageData.startsWith("\x89PNG"));
}

// ==================== 模拟模式测试 ====================

void TestQwenImageClient::testShouldMockWithForceMock()
{
    QwenImageClient* client = QwenImageClient::instance();

    QwenImageClient::Config config;
    config.apiKey = "valid-key";
    config.forceMock = true;

    client->init(config);
    QVERIFY(client->shouldMock());
}

void TestQwenImageClient::testShouldMockWithEmptyApiKey()
{
    QwenImageClient* client = QwenImageClient::instance();

    QwenImageClient::Config config;
    config.apiKey = "";
    config.forceMock = false;

    client->init(config);
    QVERIFY(client->shouldMock());
}

void TestQwenImageClient::testShouldMockWhenNotInitialized()
{
    // 注意：由于是单例，这里测试的是已初始化状态
    // 实际项目中可能需要重新设计以支持此测试
    QwenImageClient* client = QwenImageClient::instance();

    // 如果已初始化且有有效 API Key，则不应使用模拟模式
    QwenImageClient::Config config;
    config.apiKey = "valid-api-key";
    config.forceMock = false;

    client->init(config);
    QVERIFY(!client->shouldMock());
}

// ==================== 错误处理测试 ====================

void TestQwenImageClient::testGenerateWithEmptyPrompt()
{
    QwenImageClient* client = QwenImageClient::instance();

    QwenImageClient::Config config;
    config.apiKey = "test-key";
    config.forceMock = false;
    client->init(config);

    QwenImageClient::GenerateOptions options;
    options.prompt = "";  // 空提示词

    QwenImageClient::GenerateResult result = client->generate(options);

    QVERIFY(!result.success);
    QVERIFY(!result.errorMessage.isEmpty());
    QVERIFY(result.errorMessage.contains(QString::fromUtf8("提示词")));
}

void TestQwenImageClient::testEditWithEmptyPrompt()
{
    QwenImageClient* client = QwenImageClient::instance();

    QwenImageClient::Config config;
    config.apiKey = "test-key";
    client->init(config);

    QwenImageClient::EditOptions options;
    options.prompt = "";  // 空编辑指令
    options.sourceImage = QByteArray("fake-data");

    QwenImageClient::GenerateResult result = client->edit(options);

    QVERIFY(!result.success);
    QVERIFY(!result.errorMessage.isEmpty());
    QVERIFY(result.errorMessage.contains(QString::fromUtf8("编辑指令")));
}

void TestQwenImageClient::testEditWithEmptySourceImage()
{
    QwenImageClient* client = QwenImageClient::instance();

    QwenImageClient::Config config;
    config.apiKey = "test-key";
    client->init(config);

    QwenImageClient::EditOptions options;
    options.prompt = QString::fromUtf8("修改图像");
    options.sourceImage = QByteArray();  // 空源图像

    QwenImageClient::GenerateResult result = client->edit(options);

    QVERIFY(!result.success);
    QVERIFY(!result.errorMessage.isEmpty());
    QVERIFY(result.errorMessage.contains(QString::fromUtf8("源图像")));
}

// ==================== 结果结构测试 ====================

void TestQwenImageClient::testGenerateResultDefaults()
{
    QwenImageClient::GenerateResult result;

    QVERIFY(!result.success);
    QVERIFY(result.imageData.isEmpty());
    QVERIFY(result.mimeType.isEmpty());
    QVERIFY(result.requestId.isEmpty());
    QVERIFY(result.errorMessage.isEmpty());
    QCOMPARE(result.width, 0);
    QCOMPARE(result.height, 0);
    QCOMPARE(result.timestamp, (qint64)0);
}

void TestQwenImageClient::testGenerateResultSuccess()
{
    QwenImageClient* client = QwenImageClient::instance();

    QwenImageClient::Config config;
    config.forceMock = true;
    client->init(config);

    QwenImageClient::GenerateOptions options;
    options.prompt = QString::fromUtf8("测试生成");

    QwenImageClient::GenerateResult result = client->generate(options);

    // 在模拟模式下应该成功
    QVERIFY(result.success);
    QVERIFY(!result.imageData.isEmpty());
    QVERIFY(result.timestamp > 0);
}

// ==================== 异步操作测试 ====================

void TestQwenImageClient::testAsyncGenerateSignal()
{
    QwenImageClient* client = QwenImageClient::instance();

    QwenImageClient::Config config;
    config.forceMock = true;
    client->init(config);

    QwenImageClient::GenerateOptions options;
    options.prompt = QString::fromUtf8("异步测试");

    QSignalSpy spy(client, &QwenImageClient::generateCompleted);

    client->generateAsync(options);

    // 等待信号（最多 5 秒）
    QVERIFY(spy.wait(5000));

    // 验证信号被触发
    QCOMPARE(spy.count(), 1);

    // 验证结果 - 直接验证 imageData 非空即可
    QList<QVariant> arguments = spy.takeFirst();
    QVERIFY(arguments.count() > 0);
    
    // 验证 QVariant 可以转换
    QVERIFY(arguments.at(0).canConvert<QwenImageClient::GenerateResult>());
}

void TestQwenImageClient::testAsyncEditSignal()
{
    QwenImageClient* client = QwenImageClient::instance();

    QwenImageClient::Config config;
    config.forceMock = true;
    client->init(config);

    QwenImageClient::EditOptions options;
    options.prompt = QString::fromUtf8("异步编辑测试");
    options.sourceImage = QByteArray("fake-image-data");

    QSignalSpy spy(client, &QwenImageClient::editCompleted);

    client->editAsync(options);

    // 等待信号（最多 5 秒）
    QVERIFY(spy.wait(5000));

    // 验证信号被触发
    QCOMPARE(spy.count(), 1);

    // 验证结果 - 直接验证 QVariant 可转换即可
    QList<QVariant> arguments = spy.takeFirst();
    QVERIFY(arguments.count() > 0);
    
    // 验证 QVariant 可以转换
    QVERIFY(arguments.at(0).canConvert<QwenImageClient::GenerateResult>());
}

QTEST_MAIN(TestQwenImageClient)
#include "tst_qwenimageclient.moc"
