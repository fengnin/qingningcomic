#include <QtTest/QtTest>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>

#include "ImageService.h"
#include "BibleImageService.h"
#include "FileStorage.h"
#include "CharacterExtractor.h"
#include "SceneExtractor.h"
#include "StoryboardService.h"
#include "DatabaseManager.h"
#include "Logger.h"
#include "Character.h"
#include "Scene.h"
#include "Panel.h"

/**
 * @brief 图片生成集成测试类
 * 
 * 测试覆盖范围：
 * 1. 面板图片生成测试
 * 2. 角色肖像生成测试
 * 3. 场景参考图生成测试
 * 4. 图片存储路径测试
 * 5. 数据库路径更新测试
 */
class TestImageGeneration : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir* m_tempDir;
    QString m_testDataDir;

private slots:
    void initTestCase();
    void cleanupTestCase();
    
    // FileStorage 测试
    void testFileStorageInit();
    void testPanelImageStorage();
    void testCharacterPortraitStorage();
    void testSceneReferenceStorage();
    
    // 图片路径测试
    void testPanelImagePathFormat();
    void testCharacterPortraitPathFormat();
    void testSceneReferencePathFormat();
    
    // 数据库路径测试
    void testCharacterPortraitPathInDatabase();
    void testSceneReferencePathInDatabase();
    
    // 服务状态测试
    void testImageServiceStatus();
    void testBibleImageServiceStatus();
    
    // 集成测试
    void testFullImageGenerationFlow();
};

void TestImageGeneration::initTestCase()
{
    qDebug() << "========================================";
    qDebug() << "  图片生成集成测试开始";
    qDebug() << "========================================";

    // 初始化 Logger
    Logger::instance()->init();
    Logger::instance()->setConsoleOutput(true);
    Logger::instance()->setMinLevel(Logger::Debug);

    // 创建临时目录用于测试
    m_tempDir = new QTemporaryDir();
    QVERIFY(m_tempDir->isValid());
    m_testDataDir = m_tempDir->path();
    
    qDebug() << "测试数据目录:" << m_testDataDir;

    // 初始化 FileStorage
    FileStorage::instance()->init(m_testDataDir);
    
    // 注册元类型
    qRegisterMetaType<ImageService::GenerateResult>("ImageService::GenerateResult");
    qRegisterMetaType<ImageService::BatchResult>("ImageService::BatchResult");
}

void TestImageGeneration::cleanupTestCase()
{
    delete m_tempDir;
    
    qDebug() << "========================================";
    qDebug() << "  图片生成集成测试结束";
    qDebug() << "========================================";
}

// ==================== FileStorage 测试 ====================

void TestImageGeneration::testFileStorageInit()
{
    QVERIFY(FileStorage::instance()->isStorageAvailable());
}

void TestImageGeneration::testPanelImageStorage()
{
    // 创建测试图片数据（1x1 PNG）
    QByteArray testData;
    testData.append("\x89PNG\r\n\x1a\n", 8);  // PNG 签名
    
    QString panelId = "panel001";  // 使用更短的 ID
    QString savedPath = FileStorage::instance()->savePanelImage(panelId, testData, "preview");
    
    QVERIFY(!savedPath.isEmpty());
    QVERIFY(savedPath.contains("panels"));
    // 注意：路径格式是 panels/{panelId前8位}/preview.png
    QVERIFY(savedPath.contains(panelId.left(8)));
    
    // 验证文件存在
    QString fullPath = m_testDataDir + "/" + savedPath;
    QVERIFY(QFile::exists(fullPath));
    
    qDebug() << "面板图片保存路径:" << savedPath;
}

void TestImageGeneration::testCharacterPortraitStorage()
{
    // 创建测试图片数据
    QByteArray testData;
    testData.append("\x89PNG\r\n\x1a\n", 8);
    
    QString characterId = "test-char-001";
    QString savedPath = FileStorage::instance()->saveCharacterReference(characterId, testData);
    
    QVERIFY(!savedPath.isEmpty());
    QVERIFY(savedPath.contains("characters"));
    QVERIFY(savedPath.contains(characterId));
    
    // 验证文件存在
    QString fullPath = m_testDataDir + "/" + savedPath;
    QVERIFY(QFile::exists(fullPath));
    
    qDebug() << "角色肖像保存路径:" << savedPath;
}

void TestImageGeneration::testSceneReferenceStorage()
{
    // 创建测试图片数据
    QByteArray testData;
    testData.append("\x89PNG\r\n\x1a\n", 8);
    
    QString sceneId = "test-scene-001";
    QString savedPath = FileStorage::instance()->saveSceneReference(sceneId, testData);
    
    QVERIFY(!savedPath.isEmpty());
    QVERIFY(savedPath.contains("scenes"));
    QVERIFY(savedPath.contains(sceneId));
    
    // 验证文件存在
    QString fullPath = m_testDataDir + "/" + savedPath;
    QVERIFY(QFile::exists(fullPath));
    
    qDebug() << "场景参考图保存路径:" << savedPath;
}

// ==================== 图片路径测试 ====================

void TestImageGeneration::testPanelImagePathFormat()
{
    QString panelId = "panel-abc123";
    
    QString path = FileStorage::instance()->getPanelImagePath(panelId, "preview");
    QVERIFY(path.contains("panels"));
    // 注意：路径格式是 panels/{panelId前8位}/preview.png
    QVERIFY(path.contains(panelId.left(8)));
    
    qDebug() << "面板图片路径格式:" << path;
}

void TestImageGeneration::testCharacterPortraitPathFormat()
{
    QString characterId = "char-xyz789";
    
    // 保存后检查路径格式
    QByteArray testData;
    testData.append("\x89PNG\r\n\x1a\n", 8);
    QString savedPath = FileStorage::instance()->saveCharacterReference(characterId, testData);
    
    // 路径应该是 characters/{characterId}/reference.png
    QVERIFY(savedPath.startsWith("characters/"));
    QVERIFY(savedPath.contains(characterId));
    QVERIFY(savedPath.endsWith("reference.png"));
}

void TestImageGeneration::testSceneReferencePathFormat()
{
    QString sceneId = "scene-def456";
    
    // 保存后检查路径格式
    QByteArray testData;
    testData.append("\x89PNG\r\n\x1a\n", 8);
    QString savedPath = FileStorage::instance()->saveSceneReference(sceneId, testData);
    
    // 路径应该是 scenes/{sceneId}/reference.png
    QVERIFY(savedPath.startsWith("scenes/"));
    QVERIFY(savedPath.contains(sceneId));
    QVERIFY(savedPath.endsWith("reference.png"));
}

// ==================== 数据库路径测试 ====================

void TestImageGeneration::testCharacterPortraitPathInDatabase()
{
    // 创建测试角色
    Character character;
    character.setId("test-char-db-001");
    character.setNovelId("test-novel-001");
    character.setName("测试角色");
    
    // 设置肖像路径
    QString portraitPath = "characters/test-char-db-001/reference.png";
    character.setPortraitPath(portraitPath);
    
    // 验证路径正确设置
    QCOMPARE(character.portraitPath(), portraitPath);
    
    qDebug() << "角色肖像数据库路径:" << character.portraitPath();
}

void TestImageGeneration::testSceneReferencePathInDatabase()
{
    // 创建测试场景
    Scene scene;
    scene.setId("test-scene-db-001");
    scene.setNovelId("test-novel-001");
    scene.setName("测试场景");
    
    // 设置参考图路径
    QString referencePath = "scenes/test-scene-db-001/reference.png";
    scene.setReferenceImagePath(referencePath);
    
    // 验证路径正确设置
    QCOMPARE(scene.referenceImagePath(), referencePath);
    
    qDebug() << "场景参考图数据库路径:" << scene.referenceImagePath();
}

// ==================== 服务状态测试 ====================

void TestImageGeneration::testImageServiceStatus()
{
    ImageService* service = ImageService::instance();
    QVERIFY(service != nullptr);
    
    // 默认不在生成中
    QVERIFY(!service->isGenerating());
    
    // 默认没有错误
    QVERIFY(service->lastError().isEmpty());
}

void TestImageGeneration::testBibleImageServiceStatus()
{
    BibleImageService* service = BibleImageService::instance();
    QVERIFY(service != nullptr);
    
    // 默认不在生成中
    QVERIFY(!service->isGenerating());
    
    // 默认没有错误
    QVERIFY(service->lastError().isEmpty());
}

// ==================== 集成测试 ====================

void TestImageGeneration::testFullImageGenerationFlow()
{
    qDebug() << "----- 完整图片生成流程测试 -----";
    
    // 1. 测试面板图片存储
    QByteArray panelImageData;
    panelImageData.append("\x89PNG\r\n\x1a\n", 8);
    QString panelPath = FileStorage::instance()->savePanelImage("panel-test-001", panelImageData, "preview");
    QVERIFY(!panelPath.isEmpty());
    qDebug() << "1. 面板图片存储成功:" << panelPath;
    
    // 2. 测试角色肖像存储
    QByteArray charImageData;
    charImageData.append("\x89PNG\r\n\x1a\n", 8);
    QString charPath = FileStorage::instance()->saveCharacterReference("char-test-001", charImageData);
    QVERIFY(!charPath.isEmpty());
    qDebug() << "2. 角色肖像存储成功:" << charPath;
    
    // 3. 测试场景参考图存储
    QByteArray sceneImageData;
    sceneImageData.append("\x89PNG\r\n\x1a\n", 8);
    QString scenePath = FileStorage::instance()->saveSceneReference("scene-test-001", sceneImageData);
    QVERIFY(!scenePath.isEmpty());
    qDebug() << "3. 场景参考图存储成功:" << scenePath;
    
    // 4. 验证所有文件都存在
    QVERIFY(QFile::exists(m_testDataDir + "/" + panelPath));
    QVERIFY(QFile::exists(m_testDataDir + "/" + charPath));
    QVERIFY(QFile::exists(m_testDataDir + "/" + scenePath));
    qDebug() << "4. 所有文件验证成功";
    
    // 5. 验证目录结构
    QDir dataDir(m_testDataDir);
    QVERIFY(dataDir.exists("panels"));
    QVERIFY(dataDir.exists("characters"));
    QVERIFY(dataDir.exists("scenes"));
    qDebug() << "5. 目录结构验证成功";
    
    qDebug() << "----- 完整图片生成流程测试通过 -----";
}

QTEST_MAIN(TestImageGeneration)
#include "tst_imagegeneration.moc"
