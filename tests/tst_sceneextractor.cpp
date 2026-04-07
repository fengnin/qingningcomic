#include <QtTest/QtTest>
#include <QJsonArray>
#include <QJsonObject>
#include "SceneExtractor.h"
#include "Scene.h"

class TestSceneExtractor : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testExtractedSceneToJson();
    void testExtractedSceneFromJson();
    void testExtractedSceneRoundTrip();
    void testExtractFromScenesBasic();
    void testExtractFromScenesEmpty();
    void testExtractFromScenesWithVisualData();
    void testExtractFromScenesWithDetails();
    void testParseAISceneBasic();
    void testParseAISceneWithVisualCharacteristics();
    void testParseAISceneEmpty();

private:
    QJsonObject createTestScene(const QString& id, const QString& name, const QString& description);
};

void TestSceneExtractor::initTestCase()
{
    qDebug() << "========================================";
    qDebug() << "  SceneExtractor 单元测试开始";
    qDebug() << "========================================";
}

void TestSceneExtractor::cleanupTestCase()
{
    qDebug() << "========================================";
    qDebug() << "  SceneExtractor 单元测试结束";
    qDebug() << "========================================";
}

void TestSceneExtractor::testExtractedSceneToJson()
{
    ExtractedScene scene;
    scene.id = "scene-001";
    scene.name = QString::fromUtf8("神秘森林");
    scene.description = QString::fromUtf8("古老的森林");
    scene.building = "cabin";
    scene.color = "green";
    scene.atmosphere = "mysterious";
    scene.details << "moss" << "fog";
    
    QJsonObject json = scene.toJson();
    
    QCOMPARE(json["id"].toString(), QString("scene-001"));
    QCOMPARE(json["name"].toString(), QString::fromUtf8("神秘森林"));
    QCOMPARE(json["building"].toString(), QString("cabin"));
}

void TestSceneExtractor::testExtractedSceneFromJson()
{
    QJsonObject json;
    json["id"] = "scene-002";
    json["name"] = QString::fromUtf8("城市街道");
    json["description"] = QString::fromUtf8("繁忙的街道");
    json["type"] = "outdoor";
    
    ExtractedScene scene = ExtractedScene::fromJson(json);
    
    QCOMPARE(scene.id, QString("scene-002"));
    QCOMPARE(scene.name, QString::fromUtf8("城市街道"));
    QCOMPARE(scene.type, QString("outdoor"));
}

void TestSceneExtractor::testExtractedSceneRoundTrip()
{
    ExtractedScene original;
    original.id = "scene-003";
    original.name = QString::fromUtf8("古老城堡");
    original.building = "castle";
    original.atmosphere = "haunted";
    
    QJsonObject json = original.toJson();
    ExtractedScene restored = ExtractedScene::fromJson(json);
    
    QCOMPARE(restored.id, original.id);
    QCOMPARE(restored.name, original.name);
    QCOMPARE(restored.building, original.building);
}

QJsonObject TestSceneExtractor::createTestScene(const QString& id, const QString& name, const QString& description)
{
    QJsonObject scene;
    scene["id"] = id;
    scene["name"] = name;
    scene["description"] = description;
    return scene;
}

void TestSceneExtractor::testExtractFromScenesBasic()
{
    QJsonArray scenes;
    scenes.append(createTestScene("s1", "Forest", "Dark forest"));
    scenes.append(createTestScene("s2", "City", "Modern city"));
    
    QList<ExtractedScene> result = SceneExtractor::instance()->extractFromScenes(scenes);
    
    QCOMPARE(result.size(), 2);
    QCOMPARE(result[0].name, QString("Forest"));
    QCOMPARE(result[1].name, QString("City"));
}

void TestSceneExtractor::testExtractFromScenesEmpty()
{
    QJsonArray scenes;
    QList<ExtractedScene> result = SceneExtractor::instance()->extractFromScenes(scenes);
    QVERIFY(result.isEmpty());
}

void TestSceneExtractor::testExtractFromScenesWithVisualData()
{
    QJsonArray scenes;
    QJsonObject scene1;
    scene1["id"] = "s1";
    scene1["name"] = "Beach";
    
    QJsonObject visual;
    visual["building"] = "lighthouse";
    visual["color"] = "blue";
    scene1["visualCharacteristics"] = visual;
    
    scenes.append(scene1);
    
    QList<ExtractedScene> result = SceneExtractor::instance()->extractFromScenes(scenes);
    
    QCOMPARE(result.size(), 1);
    QCOMPARE(result[0].building, QString("lighthouse"));
    QCOMPARE(result[0].color, QString("blue"));
}

void TestSceneExtractor::testExtractFromScenesWithDetails()
{
    QJsonArray scenes;
    QJsonObject scene1;
    scene1["id"] = "s1";
    scene1["name"] = "Room";
    
    QJsonArray details;
    details << "furniture" << "paintings";
    scene1["details"] = details;
    
    scenes.append(scene1);
    
    QList<ExtractedScene> result = SceneExtractor::instance()->extractFromScenes(scenes);
    
    QCOMPARE(result.size(), 1);
    QCOMPARE(result[0].details.size(), 2);
}

void TestSceneExtractor::testParseAISceneBasic()
{
    QJsonObject sceneObj;
    sceneObj["id"] = "test-scene";
    sceneObj["name"] = QString::fromUtf8("测试场景");
    sceneObj["type"] = "indoor";
    
    ExtractedScene result = SceneExtractor::instance()->parseAIScene(sceneObj);
    
    QCOMPARE(result.id, QString("test-scene"));
    QCOMPARE(result.name, QString::fromUtf8("测试场景"));
    QCOMPARE(result.type, QString("indoor"));
}

void TestSceneExtractor::testParseAISceneWithVisualCharacteristics()
{
    QJsonObject sceneObj;
    sceneObj["id"] = "visual-scene";
    sceneObj["name"] = "Temple";
    
    QJsonObject visual;
    visual["building"] = "temple";
    visual["color"] = "gold";
    visual["atmosphere"] = "sacred";
    sceneObj["visualCharacteristics"] = visual;
    
    ExtractedScene result = SceneExtractor::instance()->parseAIScene(sceneObj);
    
    QCOMPARE(result.building, QString("temple"));
    QCOMPARE(result.color, QString("gold"));
    QCOMPARE(result.atmosphere, QString("sacred"));
}

void TestSceneExtractor::testParseAISceneEmpty()
{
    QJsonObject sceneObj;
    ExtractedScene result = SceneExtractor::instance()->parseAIScene(sceneObj);
    QVERIFY(result.id.isEmpty());
    QVERIFY(result.name.isEmpty());
}

QTEST_APPLESS_MAIN(TestSceneExtractor)
#include "tst_sceneextractor.moc"
