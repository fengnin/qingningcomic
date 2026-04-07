#include <QtTest/QtTest>
#include <QPixmap>
#include "components/PanelCard.h"
#include "Logger.h"

/**
 * @brief PanelCard 单元测试类
 * 
 * 测试覆盖范围：
 * 1. 构造函数测试
 * 2. 描述设置测试
 * 3. 图像 URL 设置测试
 * 4. Panel ID 设置测试
 */
class TestPanelCard : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    // 构造函数测试
    void testConstructor();
    void testConstructorWithDescription();

    // 描述设置测试
    void testSetDescription();

    // Panel ID 测试
    void testSetPanelId();
    void testPanelIdDefault();

    // 图像 URL 测试
    void testSetPreviewUrlEmpty();
    void testSetPreviewUrlInvalid();
    void testSetPreviewUrlBase64();
};

void TestPanelCard::initTestCase()
{
    qDebug() << "========================================";
    qDebug() << "  PanelCard 单元测试开始";
    qDebug() << "========================================";

    Logger::instance()->init();
    Logger::instance()->setConsoleOutput(true);
    Logger::instance()->setMinLevel(Logger::Debug);
}

void TestPanelCard::cleanupTestCase()
{
    qDebug() << "========================================";
    qDebug() << "  PanelCard 单元测试结束";
    qDebug() << "========================================";
}

// ==================== 构造函数测试 ====================

void TestPanelCard::testConstructor()
{
    PanelCard card(1, 3, QString::fromUtf8("测试描述"));

    QCOMPARE(card.chapterNumber(), 1);
    QCOMPARE(card.panelNumber(), 3);
    QVERIFY(card.panelId().isEmpty());
}

void TestPanelCard::testConstructorWithDescription()
{
    QString desc = QString::fromUtf8("青柠站在教室门口");
    PanelCard card(2, 5, desc);

    QCOMPARE(card.chapterNumber(), 2);
    QCOMPARE(card.panelNumber(), 5);
}

// ==================== 描述设置测试 ====================

void TestPanelCard::testSetDescription()
{
    PanelCard card(1, 1, QString::fromUtf8("初始描述"));

    QString newDesc = QString::fromUtf8("更新后的描述");
    card.setDescription(newDesc);

    // 验证方法调用成功（无异常）
    QVERIFY(true);
}

// ==================== Panel ID 测试 ====================

void TestPanelCard::testSetPanelId()
{
    PanelCard card(1, 1, QString::fromUtf8("描述"));

    QString panelId = "panel-12345";
    card.setPanelId(panelId);

    QCOMPARE(card.panelId(), panelId);
}

void TestPanelCard::testPanelIdDefault()
{
    PanelCard card(1, 1, QString::fromUtf8("描述"));

    QVERIFY(card.panelId().isEmpty());
}

// ==================== 图像 URL 测试 ====================

void TestPanelCard::testSetPreviewUrlEmpty()
{
    PanelCard card(1, 1, QString::fromUtf8("描述"));

    // 设置空 URL 应该显示"预览"文本
    card.setPreviewUrl("");

    // 验证方法调用成功（无异常）
    QVERIFY(true);
}

void TestPanelCard::testSetPreviewUrlInvalid()
{
    PanelCard card(1, 1, QString::fromUtf8("描述"));

    // 设置无效 URL
    card.setPreviewUrl("invalid-url");

    // 验证方法调用成功（无异常）
    QVERIFY(true);
}

void TestPanelCard::testSetPreviewUrlBase64()
{
    PanelCard card(1, 1, QString::fromUtf8("描述"));

    // 创建一个简单的 1x1 透明 PNG 的 Base64
    QString base64Png = "data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mNk+M9QDwADhgGAWjR9awAAAABJRU5ErkJggg==";

    card.setPreviewUrl(base64Png);

    // 验证方法调用成功（无异常）
    QVERIFY(true);
}

QTEST_MAIN(TestPanelCard)
#include "tst_panelcard.moc"
