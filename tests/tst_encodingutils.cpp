#include <QtTest/QtTest>
#include <QString>
#include <QByteArray>
#include "EncodingUtils.h"

class TestEncodingUtils : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    // fromUtf8 测试
    void testFromUtf8Basic();
    void testFromUtf8Chinese();
    void testFromUtf8Empty();
    void testFromUtf8SpecialChars();
    void testFromUtf8Emoji();

    // fromLocal8Bit 测试
    void testFromLocal8BitByteArray();
    void testFromLocal8BitCString();
    void testFromLocal8BitEmpty();
    void testFromLocal8BitNull();

    // toLocal8Bit 测试
    void testToLocal8BitBasic();
    void testToLocal8BitChinese();
    void testToLocal8BitEmpty();

    // containsChinese 测试
    void testContainsChineseTrue();
    void testContainsChineseFalse();
    void testContainsChineseMixed();
    void testContainsChineseEmpty();
    void testContainsChineseOnlyChinese();
    void testContainsChineseBoundary();

    // fixGarbledString 测试
    void testFixGarbledStringEmpty();
    void testFixGarbledStringNormal();
    void testFixGarbledStringAlreadyFixed();

    // TR 宏测试
    void testTrMacro();
    void testTrFmtMacro();

    // Round Trip 测试
    void testRoundTripUtf8();
    void testRoundTripLocal8Bit();
};

void TestEncodingUtils::initTestCase()
{
    qDebug() << "========================================";
    qDebug() << "  EncodingUtils 单元测试开始";
    qDebug() << "========================================";
    
    EncodingUtils::initSystemEncoding();
}

void TestEncodingUtils::cleanupTestCase()
{
    qDebug() << "========================================";
    qDebug() << "  EncodingUtils 单元测试结束";
    qDebug() << "========================================";
}

// ==================== fromUtf8 测试 ====================

void TestEncodingUtils::testFromUtf8Basic()
{
    QString result = EncodingUtils::fromUtf8("Hello World");
    QCOMPARE(result, QString("Hello World"));
}

void TestEncodingUtils::testFromUtf8Chinese()
{
    QString result = EncodingUtils::fromUtf8("中文测试");
    QCOMPARE(result, QString::fromUtf8("中文测试"));
}

void TestEncodingUtils::testFromUtf8Empty()
{
    QString result = EncodingUtils::fromUtf8("");
    QVERIFY(result.isEmpty());
}

void TestEncodingUtils::testFromUtf8SpecialChars()
{
    QString result = EncodingUtils::fromUtf8("Line1\nLine2\tTab");
    QVERIFY(result.contains('\n'));
    QVERIFY(result.contains('\t'));
}

void TestEncodingUtils::testFromUtf8Emoji()
{
    QString result = EncodingUtils::fromUtf8("Hello World");
    QVERIFY(result.contains("Hello"));
    QVERIFY(result.contains("World"));
}

// ==================== fromLocal8Bit 测试 ====================

void TestEncodingUtils::testFromLocal8BitByteArray()
{
    QByteArray data = QByteArray("Test String");
    QString result = EncodingUtils::fromLocal8Bit(data);
    QCOMPARE(result, QString("Test String"));
}

void TestEncodingUtils::testFromLocal8BitCString()
{
    QString result = EncodingUtils::fromLocal8Bit("Test C String");
    QCOMPARE(result, QString("Test C String"));
}

void TestEncodingUtils::testFromLocal8BitEmpty()
{
    QString result = EncodingUtils::fromLocal8Bit("");
    QVERIFY(result.isEmpty());
}

void TestEncodingUtils::testFromLocal8BitNull()
{
    QString result = EncodingUtils::fromLocal8Bit(nullptr);
    QVERIFY(result.isEmpty());
}

// ==================== toLocal8Bit 测试 ====================

void TestEncodingUtils::testToLocal8BitBasic()
{
    QString str = "Hello World";
    QByteArray result = EncodingUtils::toLocal8Bit(str);
    QCOMPARE(result, QByteArray("Hello World"));
}

void TestEncodingUtils::testToLocal8BitChinese()
{
    QString str = QString::fromUtf8("中文测试");
    QByteArray result = EncodingUtils::toLocal8Bit(str);
    QVERIFY(!result.isEmpty());
}

void TestEncodingUtils::testToLocal8BitEmpty()
{
    QString str;
    QByteArray result = EncodingUtils::toLocal8Bit(str);
    QVERIFY(result.isEmpty());
}

// ==================== containsChinese 测试 ====================

void TestEncodingUtils::testContainsChineseTrue()
{
    QVERIFY(EncodingUtils::containsChinese(QString::fromUtf8("中文")));
    QVERIFY(EncodingUtils::containsChinese(QString::fromUtf8("Hello中文World")));
    QVERIFY(EncodingUtils::containsChinese(QString::fromUtf8("测试")));
}

void TestEncodingUtils::testContainsChineseFalse()
{
    QVERIFY(!EncodingUtils::containsChinese("Hello World"));
    QVERIFY(!EncodingUtils::containsChinese("12345"));
    QVERIFY(!EncodingUtils::containsChinese("!@#$%"));
    QVERIFY(!EncodingUtils::containsChinese(""));
}

void TestEncodingUtils::testContainsChineseMixed()
{
    QVERIFY(EncodingUtils::containsChinese(QString::fromUtf8("Hello世界")));
    QVERIFY(EncodingUtils::containsChinese(QString::fromUtf8("测试123")));
    QVERIFY(EncodingUtils::containsChinese(QString::fromUtf8("abc中文xyz")));
}

void TestEncodingUtils::testContainsChineseEmpty()
{
    QVERIFY(!EncodingUtils::containsChinese(""));
    QVERIFY(!EncodingUtils::containsChinese(QString()));
}

void TestEncodingUtils::testContainsChineseOnlyChinese()
{
    QVERIFY(EncodingUtils::containsChinese(QString::fromUtf8("中华人民共和国")));
    QVERIFY(EncodingUtils::containsChinese(QString::fromUtf8("青柠漫画")));
}

void TestEncodingUtils::testContainsChineseBoundary()
{
    QVERIFY(EncodingUtils::containsChinese(QString(QChar(0x4E00))));
    QVERIFY(EncodingUtils::containsChinese(QString(QChar(0x9FFF))));
    QVERIFY(!EncodingUtils::containsChinese(QString(QChar(0x4DFF))));
    QVERIFY(!EncodingUtils::containsChinese(QString(QChar(0xA000))));
}

// ==================== fixGarbledString 测试 ====================

void TestEncodingUtils::testFixGarbledStringEmpty()
{
    QString result = EncodingUtils::fixGarbledString("");
    QVERIFY(result.isEmpty());
}

void TestEncodingUtils::testFixGarbledStringNormal()
{
    QString normal = QString::fromUtf8("正常字符串");
    QString result = EncodingUtils::fixGarbledString(normal);
    QCOMPARE(result, normal);
}

void TestEncodingUtils::testFixGarbledStringAlreadyFixed()
{
    QString str = QString::fromUtf8("已经正确的中文");
    QString result = EncodingUtils::fixGarbledString(str);
    QVERIFY(!result.isEmpty());
}

// ==================== TR 宏测试 ====================

void TestEncodingUtils::testTrMacro()
{
    QString result = TR("测试字符串");
    QCOMPARE(result, QString::fromUtf8("测试字符串"));
}

void TestEncodingUtils::testTrFmtMacro()
{
    QString result = TR_FMT("Hello %1, count: %2", "World", 42);
    QCOMPARE(result, QString("Hello World, count: 42"));
}

// ==================== Round Trip 测试 ====================

void TestEncodingUtils::testRoundTripUtf8()
{
    QString original = QString::fromUtf8("中文测试123");
    QString utf8String = EncodingUtils::fromUtf8(original.toUtf8().constData());
    QCOMPARE(utf8String, original);
}

void TestEncodingUtils::testRoundTripLocal8Bit()
{
    QString original = "ASCII String";
    QByteArray local = EncodingUtils::toLocal8Bit(original);
    QString restored = EncodingUtils::fromLocal8Bit(local);
    QCOMPARE(restored, original);
}

QTEST_APPLESS_MAIN(TestEncodingUtils)
#include "tst_encodingutils.moc"
