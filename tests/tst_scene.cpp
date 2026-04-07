#include <QtTest/QtTest>
#include "Scene.h"

class TestScene : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testSceneDefaultConstructor();
    void testSceneSettersAndGetters();
    void testSceneToJson();
    void testSceneFromJson();
    void testSceneJsonRoundTrip();
    
    void testSceneDetailsToJson();
    void testSceneDetailsFromJson();
    void testSceneDetailsJsonRoundTrip();
    void testSceneDetailsWithVisualCharacteristics();
    void testSceneDetailsToDisplayStrings();
    
    void testParsedVisualData();
    void testWriteVisualCharacteristics();
    void testChineseSceneName();
};

void TestScene::initTestCase()
{
    qDebug() << "========================================";
    qDebug() << "  Scene 模型单元测试开始";
    qDebug() << "========================================";
}

void TestScene::cleanupTestCase()
{
    qDebug() << "========================================";
    qDebug() << "  Scene 模型单元测试结束";
    qDebug() << "========================================";
}

void TestScene::testSceneDefaultConstructor()
{
    Scene scene;
    
    QVERIFY(scene.id().isEmpty());
    QVERIFY(scene.novelId().isEmpty());
    QVERIFY(scene.name().isEmpty());
    QVERIFY(scene.sceneId().isEmpty());
    QVERIFY(scene.tags().isEmpty());
}

void TestScene::testSceneSettersAndGetters()
{
    Scene scene;
    
    scene.setId("scene-001");
    QCOMPARE(scene.id(), QString("scene-001"));
    
    scene.setNovelId("novel-001");
    QCOMPARE(scene.novelId(), QString("novel-001"));
    
    scene.setName(QString::fromUtf8("神秘森林"));
    QCOMPARE(scene.name(), QString::fromUtf8("神秘森林"));
    
    scene.setSceneId("SCENE-001");
    QCOMPARE(scene.sceneId(), QString("SCENE-001"));
    
    QStringList tags;
    tags << "outdoor" << "nature";
    scene.setTags(tags);
    QCOMPARE(scene.tags(), tags);
    
    SceneDetails details;
    details.description = QString::fromUtf8("古老的森林");
    details.atmosphere = "mysterious";
    scene.setDetails(details);
    QCOMPARE(scene.details().description, QString::fromUtf8("古老的森林"));
    QCOMPARE(scene.details().atmosphere, QString("mysterious"));
}

void TestScene::testSceneToJson()
{
    Scene scene;
    scene.setId("scene-002");
    scene.setNovelId("novel-002");
    scene.setName(QString::fromUtf8("城市街道"));
    scene.setSceneId("SCENE-002");
    
    SceneDetails details;
    details.description = QString::fromUtf8("繁忙的街道");
    details.type = "outdoor";
    scene.setDetails(details);
    
    QStringList tags;
    tags << "urban" << "day";
    scene.setTags(tags);
    
    QJsonObject json = scene.toJson();
    
    QCOMPARE(json["id"].toString(), QString("scene-002"));
    QCOMPARE(json["novelId"].toString(), QString("novel-002"));
    QCOMPARE(json["name"].toString(), QString::fromUtf8("城市街道"));
    QCOMPARE(json["sceneId"].toString(), QString("SCENE-002"));
    QVERIFY(json.contains("details"));
    QVERIFY(json.contains("tags"));
}

void TestScene::testSceneFromJson()
{
    QJsonObject json;
    json["id"] = "scene-003";
    json["novelId"] = "novel-003";
    json["name"] = QString::fromUtf8("古老城堡");
    json["sceneId"] = "SCENE-003";
    
    QJsonObject detailsJson;
    detailsJson["description"] = QString::fromUtf8("废弃的古堡");
    detailsJson["type"] = "indoor";
    detailsJson["atmosphere"] = "haunted";
    json["details"] = detailsJson;
    
    QJsonArray tagsArray;
    tagsArray << "castle" << "night";
    json["tags"] = tagsArray;
    
    Scene scene = Scene::fromJson(json);
    
    QCOMPARE(scene.id(), QString("scene-003"));
    QCOMPARE(scene.novelId(), QString("novel-003"));
    QCOMPARE(scene.name(), QString::fromUtf8("古老城堡"));
    QCOMPARE(scene.sceneId(), QString("SCENE-003"));
    QCOMPARE(scene.details().description, QString::fromUtf8("废弃的古堡"));
    QCOMPARE(scene.details().type, QString("indoor"));
    QCOMPARE(scene.tags().size(), 2);
}

void TestScene::testSceneJsonRoundTrip()
{
    Scene original;
    original.setId("scene-004");
    original.setNovelId("novel-004");
    original.setName(QString::fromUtf8("往返测试场景"));
    original.setSceneId("SCENE-004");
    
    SceneDetails details;
    details.description = QString::fromUtf8("测试描述");
    details.type = "outdoor";
    details.setting = "forest";
    details.timeOfDay = "morning";
    details.weather = "sunny";
    original.setDetails(details);
    
    QStringList tags;
    tags << "test" << "roundtrip";
    original.setTags(tags);
    
    QJsonObject json = original.toJson();
    Scene restored = Scene::fromJson(json);
    
    QCOMPARE(restored.id(), original.id());
    QCOMPARE(restored.novelId(), original.novelId());
    QCOMPARE(restored.name(), original.name());
    QCOMPARE(restored.sceneId(), original.sceneId());
    QCOMPARE(restored.details().description, details.description);
    QCOMPARE(restored.details().type, details.type);
    QCOMPARE(restored.details().setting, details.setting);
    QCOMPARE(restored.tags(), tags);
}

void TestScene::testSceneDetailsToJson()
{
    SceneDetails details;
    details.description = QString::fromUtf8("神秘洞穴");
    details.type = "indoor";
    details.setting = "cave";
    details.timeOfDay = "night";
    details.weather = "clear";
    details.building = "cave";
    details.color = "dark";
    details.atmosphere = "mysterious";
    
    QStringList detailList;
    detailList << "stalactites" << "underground river";
    details.details = detailList;
    
    QJsonObject json = details.toJson();
    
    QCOMPARE(json["description"].toString(), QString::fromUtf8("神秘洞穴"));
    QCOMPARE(json["type"].toString(), QString("indoor"));
    QCOMPARE(json["setting"].toString(), QString("cave"));
    QCOMPARE(json["timeOfDay"].toString(), QString("night"));
    QCOMPARE(json["weather"].toString(), QString("clear"));
    QVERIFY(json.contains("visualCharacteristics"));
}

void TestScene::testSceneDetailsFromJson()
{
    QJsonObject json;
    json["description"] = QString::fromUtf8("海边沙滩");
    json["type"] = "outdoor";
    json["setting"] = "beach";
    json["timeOfDay"] = "sunset";
    json["weather"] = "cloudy";
    
    QJsonArray detailsArray;
    detailsArray << "waves" << "sand";
    json["details"] = detailsArray;
    
    SceneDetails details = SceneDetails::fromJson(json);
    
    QCOMPARE(details.description, QString::fromUtf8("海边沙滩"));
    QCOMPARE(details.type, QString("outdoor"));
    QCOMPARE(details.setting, QString("beach"));
    QCOMPARE(details.timeOfDay, QString("sunset"));
    QCOMPARE(details.weather, QString("cloudy"));
    QCOMPARE(details.details.size(), 2);
}

void TestScene::testSceneDetailsJsonRoundTrip()
{
    SceneDetails original;
    original.description = QString::fromUtf8("往返测试详情");
    original.type = "indoor";
    original.setting = "room";
    original.timeOfDay = "evening";
    original.weather = "rainy";
    original.building = "house";
    original.color = "warm";
    original.atmosphere = "cozy";
    
    QStringList detailList;
    detailList << "fireplace" << "books";
    original.details = detailList;
    
    QJsonObject json = original.toJson();
    SceneDetails restored = SceneDetails::fromJson(json);
    
    QCOMPARE(restored.description, original.description);
    QCOMPARE(restored.type, original.type);
    QCOMPARE(restored.setting, original.setting);
    QCOMPARE(restored.timeOfDay, original.timeOfDay);
    QCOMPARE(restored.weather, original.weather);
    QCOMPARE(restored.building, original.building);
    QCOMPARE(restored.color, original.color);
    QCOMPARE(restored.atmosphere, original.atmosphere);
    QCOMPARE(restored.details, detailList);
}

void TestScene::testSceneDetailsWithVisualCharacteristics()
{
    QJsonObject json;
    json["description"] = QString::fromUtf8("视觉特征测试");
    
    QJsonObject visualChars;
    visualChars["architecture"] = "temple";
    visualChars["colorScheme"] = "gold";
    visualChars["atmosphere"] = "sacred";
    
    QJsonArray landmarks;
    landmarks << "statue" << "altar";
    visualChars["keyLandmarks"] = landmarks;
    json["visualCharacteristics"] = visualChars;
    
    QJsonObject spatialLayout;
    spatialLayout["layout"] = "symmetrical";
    json["spatialLayout"] = spatialLayout;
    
    SceneDetails details = SceneDetails::fromJson(json);
    
    QCOMPARE(details.building, QString("temple"));
    QCOMPARE(details.color, QString("gold"));
    QCOMPARE(details.atmosphere, QString("sacred"));
    QCOMPARE(details.landmark, QString("statue, altar"));
    QCOMPARE(details.layout, QString("symmetrical"));
}

void TestScene::testSceneDetailsToDisplayStrings()
{
    SceneDetails details;
    details.description = QString::fromUtf8("显示测试场景");
    details.building = "castle";
    details.color = "gray";
    details.landmark = "tower";
    details.layout = "courtyard";
    details.atmosphere = "medieval";
    
    QStringList displayStrings = details.toDisplayStrings();
    
    QVERIFY(displayStrings.size() > 0);
    
    QString joined = displayStrings.join("\n");
    QVERIFY(joined.contains(QString::fromUtf8("场景描述")));
    QVERIFY(joined.contains(QString::fromUtf8("建筑")));
    QVERIFY(joined.contains(QString::fromUtf8("色调")));
    QVERIFY(joined.contains(QString::fromUtf8("氛围")));
}

void TestScene::testParsedVisualData()
{
    QJsonObject json;
    
    QJsonObject visualChars;
    visualChars["architecture"] = "palace";
    visualChars["colorScheme"] = "red";
    visualChars["atmosphere"] = "royal";
    
    QJsonArray landmarks;
    landmarks << "throne" << "garden";
    visualChars["keyLandmarks"] = landmarks;
    json["visualCharacteristics"] = visualChars;
    
    QJsonObject spatialLayout;
    spatialLayout["layout"] = "open";
    json["spatialLayout"] = spatialLayout;
    
    SceneJsonParser::ParsedVisualData data = SceneJsonParser::parseVisualCharacteristics(json);
    
    QCOMPARE(data.building, QString("palace"));
    QCOMPARE(data.color, QString("red"));
    QCOMPARE(data.atmosphere, QString("royal"));
    QCOMPARE(data.landmark, QString("throne, garden"));
    QCOMPARE(data.layout, QString("open"));
}

void TestScene::testWriteVisualCharacteristics()
{
    QJsonObject json;
    SceneJsonParser::writeVisualCharacteristics(json, "tower", "blue", "calm", "fountain", "circular");
    
    QVERIFY(json.contains("visualCharacteristics"));
    QVERIFY(json.contains("spatialLayout"));
    
    QJsonObject visualChars = json["visualCharacteristics"].toObject();
    QCOMPARE(visualChars["architecture"].toString(), QString("tower"));
    QCOMPARE(visualChars["colorScheme"].toString(), QString("blue"));
    QCOMPARE(visualChars["atmosphere"].toString(), QString("calm"));
    
    QJsonArray landmarks = visualChars["keyLandmarks"].toArray();
    QCOMPARE(landmarks.size(), 1);
    QCOMPARE(landmarks[0].toString(), QString("fountain"));
    
    QJsonObject spatialLayout = json["spatialLayout"].toObject();
    QCOMPARE(spatialLayout["layout"].toString(), QString("circular"));
}

void TestScene::testChineseSceneName()
{
    Scene scene;
    QString chineseName = QString::fromUtf8("修仙宗门大殿");
    scene.setName(chineseName);
    
    QCOMPARE(scene.name(), chineseName);
    
    QJsonObject json = scene.toJson();
    QCOMPARE(json["name"].toString(), chineseName);
    
    Scene restored = Scene::fromJson(json);
    QCOMPARE(restored.name(), chineseName);
}

QTEST_APPLESS_MAIN(TestScene)
#include "tst_scene.moc"
