#include <QtTest/QtTest>
#include "utils/ShotTypeHelper.h"

class TestShotHelper : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testShotTypeToChinese();
    void testShotTypeToEnglish();
    void testShotTypeChineseList();
    void testShotTypeUnknownEnglish();
    void testShotTypeCaseInsensitive();
    
    void testCameraAngleToChinese();
    void testCameraAngleToEnglish();
    void testCameraAngleChineseList();
    void testCameraAngleUnknownEnglish();
    void testCameraAngleCaseInsensitive();
    void testCameraAngleWithSpace();
};

void TestShotHelper::initTestCase()
{
    qDebug() << "========================================";
    qDebug() << "  ShotTypeHelper 单元测试开始";
    qDebug() << "========================================";
}

void TestShotHelper::cleanupTestCase()
{
    qDebug() << "========================================";
    qDebug() << "  ShotTypeHelper 单元测试结束";
    qDebug() << "========================================";
}

void TestShotHelper::testShotTypeToChinese()
{
    QCOMPARE(ShotTypeHelper::ShotType::toChinese("wide"), QString::fromUtf8("广角"));
    QCOMPARE(ShotTypeHelper::ShotType::toChinese("medium"), QString::fromUtf8("中景"));
    QCOMPARE(ShotTypeHelper::ShotType::toChinese("close-up"), QString::fromUtf8("近景"));
    QCOMPARE(ShotTypeHelper::ShotType::toChinese("closeup"), QString::fromUtf8("特写"));
    QCOMPARE(ShotTypeHelper::ShotType::toChinese("long"), QString::fromUtf8("远景"));
    QCOMPARE(ShotTypeHelper::ShotType::toChinese("panoramic"), QString::fromUtf8("全景"));
}

void TestShotHelper::testShotTypeToEnglish()
{
    QCOMPARE(ShotTypeHelper::ShotType::toEnglish(QString::fromUtf8("广角")), QString("wide"));
    QCOMPARE(ShotTypeHelper::ShotType::toEnglish(QString::fromUtf8("中景")), QString("medium"));
    QCOMPARE(ShotTypeHelper::ShotType::toEnglish(QString::fromUtf8("近景")), QString("close-up"));
    QCOMPARE(ShotTypeHelper::ShotType::toEnglish(QString::fromUtf8("特写")), QString("closeup"));
    QCOMPARE(ShotTypeHelper::ShotType::toEnglish(QString::fromUtf8("远景")), QString("long"));
    QCOMPARE(ShotTypeHelper::ShotType::toEnglish(QString::fromUtf8("全景")), QString("panoramic"));
}

void TestShotHelper::testShotTypeChineseList()
{
    QStringList list = ShotTypeHelper::ShotType::chineseList();
    QCOMPARE(list.size(), 6);
    QVERIFY(list.contains(QString::fromUtf8("广角")));
    QVERIFY(list.contains(QString::fromUtf8("中景")));
    QVERIFY(list.contains(QString::fromUtf8("近景")));
    QVERIFY(list.contains(QString::fromUtf8("特写")));
    QVERIFY(list.contains(QString::fromUtf8("远景")));
    QVERIFY(list.contains(QString::fromUtf8("全景")));
}

void TestShotHelper::testShotTypeUnknownEnglish()
{
    QString unknown = "unknown_type";
    QCOMPARE(ShotTypeHelper::ShotType::toChinese(unknown), unknown);
    
    QString unknownChinese = QString::fromUtf8("未知类型");
    QCOMPARE(ShotTypeHelper::ShotType::toEnglish(unknownChinese), unknownChinese.toLower());
}

void TestShotHelper::testShotTypeCaseInsensitive()
{
    QCOMPARE(ShotTypeHelper::ShotType::toChinese("WIDE"), QString::fromUtf8("广角"));
    QCOMPARE(ShotTypeHelper::ShotType::toChinese("Wide"), QString::fromUtf8("广角"));
    QCOMPARE(ShotTypeHelper::ShotType::toChinese("MEDIUM"), QString::fromUtf8("中景"));
    QCOMPARE(ShotTypeHelper::ShotType::toChinese("CLOSE-UP"), QString::fromUtf8("近景"));
}

void TestShotHelper::testCameraAngleToChinese()
{
    QCOMPARE(ShotTypeHelper::CameraAngle::toChinese("high-angle"), QString::fromUtf8("俯视"));
    QCOMPARE(ShotTypeHelper::CameraAngle::toChinese("low-angle"), QString::fromUtf8("仰视"));
    QCOMPARE(ShotTypeHelper::CameraAngle::toChinese("eye-level"), QString::fromUtf8("平视"));
    QCOMPARE(ShotTypeHelper::CameraAngle::toChinese("dutch"), QString::fromUtf8("荷兰角"));
    QCOMPARE(ShotTypeHelper::CameraAngle::toChinese("bird-eye"), QString::fromUtf8("鸟眼"));
    QCOMPARE(ShotTypeHelper::CameraAngle::toChinese("worm-eye"), QString::fromUtf8("虫视"));
}

void TestShotHelper::testCameraAngleToEnglish()
{
    QCOMPARE(ShotTypeHelper::CameraAngle::toEnglish(QString::fromUtf8("俯视")), QString("high-angle"));
    QCOMPARE(ShotTypeHelper::CameraAngle::toEnglish(QString::fromUtf8("仰视")), QString("low-angle"));
    QCOMPARE(ShotTypeHelper::CameraAngle::toEnglish(QString::fromUtf8("平视")), QString("eye-level"));
    QCOMPARE(ShotTypeHelper::CameraAngle::toEnglish(QString::fromUtf8("荷兰角")), QString("dutch"));
    QCOMPARE(ShotTypeHelper::CameraAngle::toEnglish(QString::fromUtf8("鸟眼")), QString("bird-eye"));
    QCOMPARE(ShotTypeHelper::CameraAngle::toEnglish(QString::fromUtf8("虫视")), QString("worm-eye"));
}

void TestShotHelper::testCameraAngleChineseList()
{
    QStringList list = ShotTypeHelper::CameraAngle::chineseList();
    QCOMPARE(list.size(), 6);
    QVERIFY(list.contains(QString::fromUtf8("俯视")));
    QVERIFY(list.contains(QString::fromUtf8("仰视")));
    QVERIFY(list.contains(QString::fromUtf8("平视")));
    QVERIFY(list.contains(QString::fromUtf8("荷兰角")));
    QVERIFY(list.contains(QString::fromUtf8("鸟眼")));
    QVERIFY(list.contains(QString::fromUtf8("虫视")));
}

void TestShotHelper::testCameraAngleUnknownEnglish()
{
    QString unknown = "unknown_angle";
    QCOMPARE(ShotTypeHelper::CameraAngle::toChinese(unknown), unknown);
    
    QString unknownChinese = QString::fromUtf8("未知角度");
    QCOMPARE(ShotTypeHelper::CameraAngle::toEnglish(unknownChinese), unknownChinese.toLower());
}

void TestShotHelper::testCameraAngleCaseInsensitive()
{
    QCOMPARE(ShotTypeHelper::CameraAngle::toChinese("HIGH-ANGLE"), QString::fromUtf8("俯视"));
    QCOMPARE(ShotTypeHelper::CameraAngle::toChinese("High-Angle"), QString::fromUtf8("俯视"));
    QCOMPARE(ShotTypeHelper::CameraAngle::toChinese("LOW-ANGLE"), QString::fromUtf8("仰视"));
}

void TestShotHelper::testCameraAngleWithSpace()
{
    QCOMPARE(ShotTypeHelper::CameraAngle::toChinese("high angle"), QString::fromUtf8("俯视"));
    QCOMPARE(ShotTypeHelper::CameraAngle::toChinese("low angle"), QString::fromUtf8("仰视"));
    QCOMPARE(ShotTypeHelper::CameraAngle::toChinese("eye level"), QString::fromUtf8("平视"));
    QCOMPARE(ShotTypeHelper::CameraAngle::toChinese("bird eye"), QString::fromUtf8("鸟眼"));
    QCOMPARE(ShotTypeHelper::CameraAngle::toChinese("worm eye"), QString::fromUtf8("虫视"));
}

QTEST_APPLESS_MAIN(TestShotHelper)
#include "tst_shothelper.moc"
