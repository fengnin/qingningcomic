#include <QtTest/QtTest>
#include "Bible.h"

class TestBible : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testDefaultConstructor();
    void testParameterizedConstructor();
    void testSettersAndGetters();
    void testBibleTypeCharacter();
    void testBibleTypeScene();
    void testToJsonCharacter();
    void testToJsonScene();
    void testFromJsonCharacter();
    void testFromJsonScene();
    void testJsonRoundTripCharacter();
    void testJsonRoundTripScene();
    void testToVariantMapCharacter();
    void testFromVariantMapCharacter();
    void testVariantMapRoundTripCharacter();
    void testToDisplayDetailsCharacter();
    void testToDisplayDetailsScene();
    void testChineseCharacterName();
};

void TestBible::initTestCase()
{
    qDebug() << "========================================";
    qDebug() << "  Bible 模型单元测试开始";
    qDebug() << "========================================";
}

void TestBible::cleanupTestCase()
{
    qDebug() << "========================================";
    qDebug() << "  Bible 模型单元测试结束";
    qDebug() << "========================================";
}

void TestBible::testDefaultConstructor()
{
    BibleEntry entry;
    
    QVERIFY(entry.id().isEmpty());
    QVERIFY(entry.novelId().isEmpty());
    QVERIFY(entry.name().isEmpty());
    QCOMPARE(entry.type(), BibleType::Character);
    QVERIFY(entry.tags().isEmpty());
    QVERIFY(entry.referenceImages().isEmpty());
    QVERIFY(entry.createdAt().isValid());
    QVERIFY(entry.updatedAt().isValid());
}

void TestBible::testParameterizedConstructor()
{
    BibleEntry entry("bible-001", "novel-001", QString::fromUtf8("张三"), BibleType::Character);
    
    QCOMPARE(entry.id(), QString("bible-001"));
    QCOMPARE(entry.novelId(), QString("novel-001"));
    QCOMPARE(entry.name(), QString::fromUtf8("张三"));
    QCOMPARE(entry.type(), BibleType::Character);
}

void TestBible::testSettersAndGetters()
{
    BibleEntry entry;
    
    entry.setId("bible-002");
    QCOMPARE(entry.id(), QString("bible-002"));
    
    entry.setNovelId("novel-002");
    QCOMPARE(entry.novelId(), QString("novel-002"));
    
    entry.setName(QString::fromUtf8("李四"));
    QCOMPARE(entry.name(), QString::fromUtf8("李四"));
    
    entry.setType(BibleType::Scene);
    QCOMPARE(entry.type(), BibleType::Scene);
    
    QStringList tags;
    tags << "主角" << "修仙者";
    entry.setTags(tags);
    QCOMPARE(entry.tags(), tags);
    
    QStringList images;
    images << "image1.png" << "image2.png";
    entry.setReferenceImages(images);
    QCOMPARE(entry.referenceImages(), images);
}

void TestBible::testBibleTypeCharacter()
{
    BibleEntry entry;
    entry.setType(BibleType::Character);
    
    CharacterAppearance app;
    app.gender = "male";
    app.age = 25;
    app.hairColor = "black";
    app.hairStyle = "short";
    app.eyeColor = "brown";
    entry.setCharacterAppearance(app);
    
    QCOMPARE(entry.characterAppearance().gender, QString("male"));
    QCOMPARE(entry.characterAppearance().age, 25);
    QCOMPARE(entry.characterAppearance().hairColor, QString("black"));
    
    QStringList personality;
    personality << "勇敢" << "正义";
    entry.setPersonality(personality);
    QCOMPARE(entry.personality(), personality);
}

void TestBible::testBibleTypeScene()
{
    BibleEntry entry;
    entry.setType(BibleType::Scene);
    
    SceneDetails details;
    details.description = QString::fromUtf8("神秘森林");
    details.building = "cabin";
    details.atmosphere = "mysterious";
    entry.setSceneDetails(details);
    
    QCOMPARE(entry.sceneDetails().description, QString::fromUtf8("神秘森林"));
    QCOMPARE(entry.sceneDetails().building, QString("cabin"));
    QCOMPARE(entry.sceneDetails().atmosphere, QString("mysterious"));
}

void TestBible::testToJsonCharacter()
{
    BibleEntry entry;
    entry.setId("char-001");
    entry.setName(QString::fromUtf8("王五"));
    entry.setType(BibleType::Character);
    
    CharacterAppearance app;
    app.gender = "female";
    app.age = 20;
    app.hairColor = "blonde";
    entry.setCharacterAppearance(app);
    
    QJsonObject json = entry.toJson();
    
    QCOMPARE(json["id"].toString(), QString("char-001"));
    QCOMPARE(json["name"].toString(), QString::fromUtf8("王五"));
    QCOMPARE(json["type"].toString(), QString("character"));
    QVERIFY(json.contains("appearance"));
    
    QJsonObject appJson = json["appearance"].toObject();
    QCOMPARE(appJson["gender"].toString(), QString("female"));
    QCOMPARE(appJson["age"].toInt(), 20);
    QCOMPARE(appJson["hairColor"].toString(), QString("blonde"));
}

void TestBible::testToJsonScene()
{
    BibleEntry entry;
    entry.setId("scene-001");
    entry.setName(QString::fromUtf8("古老城堡"));
    entry.setType(BibleType::Scene);
    
    SceneDetails details;
    details.description = QString::fromUtf8("废弃的古堡");
    details.building = "castle";
    entry.setSceneDetails(details);
    
    QJsonObject json = entry.toJson();
    
    QCOMPARE(json["id"].toString(), QString("scene-001"));
    QCOMPARE(json["name"].toString(), QString::fromUtf8("古老城堡"));
    QCOMPARE(json["type"].toString(), QString("scene"));
    QVERIFY(json.contains("sceneDetails"));
}

void TestBible::testFromJsonCharacter()
{
    QJsonObject json;
    json["id"] = "char-002";
    json["novelId"] = "novel-001";
    json["name"] = QString::fromUtf8("赵六");
    json["type"] = "character";
    
    QJsonObject appJson;
    appJson["gender"] = "male";
    appJson["age"] = 30;
    appJson["hairColor"] = "brown";
    json["appearance"] = appJson;
    
    BibleEntry entry = BibleEntry::fromJson(json);
    
    QCOMPARE(entry.id(), QString("char-002"));
    QCOMPARE(entry.novelId(), QString("novel-001"));
    QCOMPARE(entry.name(), QString::fromUtf8("赵六"));
    QCOMPARE(entry.type(), BibleType::Character);
    QCOMPARE(entry.characterAppearance().gender, QString("male"));
    QCOMPARE(entry.characterAppearance().age, 30);
}

void TestBible::testFromJsonScene()
{
    QJsonObject json;
    json["id"] = "scene-002";
    json["name"] = QString::fromUtf8("神秘洞穴");
    json["type"] = "scene";
    
    QJsonObject detailsJson;
    detailsJson["description"] = QString::fromUtf8("深山洞穴");
    detailsJson["building"] = "cave";
    detailsJson["atmosphere"] = "dark";
    json["sceneDetails"] = detailsJson;
    
    BibleEntry entry = BibleEntry::fromJson(json);
    
    QCOMPARE(entry.id(), QString("scene-002"));
    QCOMPARE(entry.name(), QString::fromUtf8("神秘洞穴"));
    QCOMPARE(entry.type(), BibleType::Scene);
    QCOMPARE(entry.sceneDetails().description, QString::fromUtf8("深山洞穴"));
    QCOMPARE(entry.sceneDetails().building, QString("cave"));
}

void TestBible::testJsonRoundTripCharacter()
{
    BibleEntry original;
    original.setId("char-003");
    original.setNovelId("novel-002");
    original.setName(QString::fromUtf8("测试角色"));
    original.setType(BibleType::Character);
    
    CharacterAppearance app;
    app.gender = "female";
    app.age = 22;
    app.hairColor = "red";
    app.hairStyle = "long";
    app.eyeColor = "blue";
    app.height = "tall";
    app.build = "slim";
    original.setCharacterAppearance(app);
    
    QStringList personality;
    personality << "聪明" << "善良";
    original.setPersonality(personality);
    
    QJsonObject json = original.toJson();
    BibleEntry restored = BibleEntry::fromJson(json);
    
    QCOMPARE(restored.id(), original.id());
    QCOMPARE(restored.novelId(), original.novelId());
    QCOMPARE(restored.name(), original.name());
    QCOMPARE(restored.type(), original.type());
    QCOMPARE(restored.characterAppearance().gender, app.gender);
    QCOMPARE(restored.characterAppearance().age, app.age);
    QCOMPARE(restored.characterAppearance().hairColor, app.hairColor);
    QCOMPARE(restored.personality(), personality);
}

void TestBible::testJsonRoundTripScene()
{
    BibleEntry original;
    original.setId("scene-003");
    original.setName(QString::fromUtf8("测试场景"));
    original.setType(BibleType::Scene);
    
    SceneDetails details;
    details.description = QString::fromUtf8("测试描述");
    details.building = "temple";
    details.atmosphere = "sacred";
    original.setSceneDetails(details);
    
    QJsonObject json = original.toJson();
    BibleEntry restored = BibleEntry::fromJson(json);
    
    QCOMPARE(restored.id(), original.id());
    QCOMPARE(restored.name(), original.name());
    QCOMPARE(restored.type(), original.type());
    QCOMPARE(restored.sceneDetails().description, details.description);
    QCOMPARE(restored.sceneDetails().building, details.building);
}

void TestBible::testToVariantMapCharacter()
{
    BibleEntry entry;
    entry.setId("char-004");
    entry.setName(QString::fromUtf8("变体角色"));
    entry.setType(BibleType::Character);
    
    QVariantMap map = entry.toVariantMap();
    
    QCOMPARE(map["id"].toString(), QString("char-004"));
    QCOMPARE(map["name"].toString(), QString::fromUtf8("变体角色"));
    QCOMPARE(map["type"].toString(), QString("character"));
    QVERIFY(map.contains("appearance"));
}

void TestBible::testFromVariantMapCharacter()
{
    QVariantMap map;
    map["id"] = "char-005";
    map["novelId"] = "novel-003";
    map["name"] = QString::fromUtf8("变体角色2");
    map["type"] = "character";
    
    QVariantMap appMap;
    appMap["gender"] = "male";
    appMap["age"] = 28;
    map["appearance"] = appMap;
    
    BibleEntry entry = BibleEntry::fromVariantMap(map);
    
    QCOMPARE(entry.id(), QString("char-005"));
    QCOMPARE(entry.novelId(), QString("novel-003"));
    QCOMPARE(entry.name(), QString::fromUtf8("变体角色2"));
    QCOMPARE(entry.characterAppearance().gender, QString("male"));
    QCOMPARE(entry.characterAppearance().age, 28);
}

void TestBible::testVariantMapRoundTripCharacter()
{
    BibleEntry original;
    original.setId("char-006");
    original.setName(QString::fromUtf8("往返测试"));
    original.setType(BibleType::Character);
    
    CharacterAppearance app;
    app.gender = "female";
    app.age = 25;
    original.setCharacterAppearance(app);
    
    QVariantMap map = original.toVariantMap();
    BibleEntry restored = BibleEntry::fromVariantMap(map);
    
    QCOMPARE(restored.id(), original.id());
    QCOMPARE(restored.name(), original.name());
    QCOMPARE(restored.characterAppearance().gender, app.gender);
    QCOMPARE(restored.characterAppearance().age, app.age);
}

void TestBible::testToDisplayDetailsCharacter()
{
    BibleEntry entry;
    entry.setType(BibleType::Character);
    
    CharacterAppearance app;
    app.gender = "male";
    app.age = 30;
    app.hairColor = "black";
    app.hairStyle = "short";
    app.eyeColor = "brown";
    app.build = "athletic";
    entry.setCharacterAppearance(app);
    
    QStringList personality;
    personality << "勇敢" << "正义";
    entry.setPersonality(personality);
    
    QStringList details = entry.toDisplayDetails();
    
    QVERIFY(details.size() > 0);
    QVERIFY(details.join("").contains(QString::fromUtf8("性别")));
    QVERIFY(details.join("").contains(QString::fromUtf8("年龄")));
    QVERIFY(details.join("").contains(QString::fromUtf8("性格")));
}

void TestBible::testToDisplayDetailsScene()
{
    BibleEntry entry;
    entry.setType(BibleType::Scene);
    
    SceneDetails details;
    details.description = QString::fromUtf8("神秘森林");
    details.type = "outdoor";
    details.atmosphere = "mysterious";
    details.building = "cabin";
    entry.setSceneDetails(details);
    
    QStringList displayDetails = entry.toDisplayDetails();
    
    QVERIFY(displayDetails.size() > 0);
    QVERIFY(displayDetails.join("").contains(QString::fromUtf8("描述")));
    QVERIFY(displayDetails.join("").contains(QString::fromUtf8("氛围")));
}

void TestBible::testChineseCharacterName()
{
    BibleEntry entry;
    QString chineseName = QString::fromUtf8("欧阳修仙者");
    entry.setName(chineseName);
    
    QCOMPARE(entry.name(), chineseName);
    
    QJsonObject json = entry.toJson();
    QCOMPARE(json["name"].toString(), chineseName);
    
    BibleEntry restored = BibleEntry::fromJson(json);
    QCOMPARE(restored.name(), chineseName);
}

QTEST_APPLESS_MAIN(TestBible)
#include "tst_bible.moc"
