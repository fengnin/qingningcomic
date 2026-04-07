#include <QtTest/QtTest>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include "utils/JsonUtils.h"
#include "Character.h"
#include "Storyboard.h"
#include "Panel.h"
#include "Job.h"

class TestJsonUtils : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    // 基础 JSON 操作测试
    void testJsonArrayToStringList();
    void testStringListToJsonArray();
    void testJsonToString();
    void testStringToJson();
    void testGetStringField();
    void testGetIntField();
    void testGetBoolField();
    void testGetDoubleField();
    void testGetObjectField();
    void testGetArrayField();

    // 边界情况测试
    void testGetStringFieldWithDefault();
    void testGetIntFieldWithDefault();
    void testGetFieldFromEmptyObject();
    void testGetFieldFromMissingKey();

    // Character 序列化测试
    void testCharacterToJson();
    void testJsonToCharacter();
    void testCharacterRoundTrip();
    void testCharacterWithEmptyFields();

    // CharacterAppearance 序列化测试
    void testAppearanceToJson();
    void testJsonToAppearance();
    void testAppearanceRoundTrip();

    // Storyboard 序列化测试
    void testStoryboardToJson();
    void testJsonToStoryboard();
    void testStoryboardRoundTrip();

    // Panel 序列化测试
    void testPanelToJson();
    void testJsonToPanel();
    void testPanelRoundTrip();
    void testPanelSetSceneNotOverwrittenBySetContent();
    void testPanelSetContentUpdatesExistingFields();
    void testPanelMergeContent();

    // Job 序列化测试
    void testJobToJson();
    void testJsonToJob();
    void testJobRoundTrip();

    // Variant 转换测试
    void testVariantToJson();
    void testVariantMapToJsonString();
    void testJsonStringToVariantMap();

    // 异常情况测试
    void testGetIntFieldFromString();
    void testGetStringFieldFromNumber();
    void testGetBoolFieldFromString();
    void testStringToJsonInvalid();
    void testStringToJsonEmpty();
    void testCharacterWithSpecialChars();
    void testCharacterWithEmoji();
    void testVeryLongString();
    void testNullField();
    void testEmptyArrayField();
    void testNestedEmptyObject();

    // 高级测试
    void testEscapeCharacters();
    void testUnicodeEscape();
    void testScientificNotation();
    void testExtremeNumbers();
};

void TestJsonUtils::initTestCase()
{
    qDebug() << "========================================";
    qDebug() << "  JsonUtils 单元测试开始";
    qDebug() << "========================================";
}

void TestJsonUtils::cleanupTestCase()
{
    qDebug() << "========================================";
    qDebug() << "  JsonUtils 单元测试结束";
    qDebug() << "========================================";
}

// ==================== 基础 JSON 操作测试 ====================

void TestJsonUtils::testJsonArrayToStringList()
{
    QJsonArray arr;
    arr << "apple" << "banana" << "cherry";
    
    QStringList result = JsonUtils::jsonArrayToStringList(arr);
    
    QCOMPARE(result.size(), 3);
    QCOMPARE(result[0], QString("apple"));
    QCOMPARE(result[1], QString("banana"));
    QCOMPARE(result[2], QString("cherry"));
}

void TestJsonUtils::testStringListToJsonArray()
{
    QStringList list;
    list << "one" << "two" << "three";
    
    QJsonArray result = JsonUtils::stringListToJsonArray(list);
    
    QCOMPARE(result.size(), 3);
    QCOMPARE(result[0].toString(), QString("one"));
    QCOMPARE(result[1].toString(), QString("two"));
    QCOMPARE(result[2].toString(), QString("three"));
}

void TestJsonUtils::testJsonToString()
{
    QJsonObject obj;
    obj["name"] = "test";
    obj["value"] = 123;
    
    QString result = JsonUtils::jsonToString(obj);
    
    QVERIFY(result.contains("\"name\""));
    QVERIFY(result.contains("\"test\""));
    QVERIFY(result.contains("\"value\""));
    QVERIFY(result.contains("123"));
}

void TestJsonUtils::testStringToJson()
{
    QString jsonStr = "{\"name\":\"Alice\",\"age\":25}";
    
    QJsonObject result = JsonUtils::stringToJson(jsonStr);
    
    QCOMPARE(result["name"].toString(), QString("Alice"));
    QCOMPARE(result["age"].toInt(), 25);
}

void TestJsonUtils::testGetStringField()
{
    QJsonObject obj;
    obj["name"] = "Alice";
    obj["city"] = "Beijing";
    
    QCOMPARE(JsonUtils::getStringField(obj, "name"), QString("Alice"));
    QCOMPARE(JsonUtils::getStringField(obj, "city"), QString("Beijing"));
    QCOMPARE(JsonUtils::getStringField(obj, "missing"), QString());
}

void TestJsonUtils::testGetIntField()
{
    QJsonObject obj;
    obj["age"] = 25;
    obj["count"] = 100;
    
    QCOMPARE(JsonUtils::getIntField(obj, "age"), 25);
    QCOMPARE(JsonUtils::getIntField(obj, "count"), 100);
    QCOMPARE(JsonUtils::getIntField(obj, "missing"), 0);
}

void TestJsonUtils::testGetBoolField()
{
    QJsonObject obj;
    obj["active"] = true;
    obj["deleted"] = false;
    
    QCOMPARE(JsonUtils::getBoolField(obj, "active"), true);
    QCOMPARE(JsonUtils::getBoolField(obj, "deleted"), false);
    QCOMPARE(JsonUtils::getBoolField(obj, "missing"), false);
}

void TestJsonUtils::testGetDoubleField()
{
    QJsonObject obj;
    obj["price"] = 99.99;
    obj["rate"] = 3.14159;
    
    QCOMPARE(JsonUtils::getDoubleField(obj, "price"), 99.99);
    QVERIFY(qAbs(JsonUtils::getDoubleField(obj, "rate") - 3.14159) < 0.00001);
    QCOMPARE(JsonUtils::getDoubleField(obj, "missing"), 0.0);
}

void TestJsonUtils::testGetObjectField()
{
    QJsonObject obj;
    QJsonObject inner;
    inner["key"] = "value";
    obj["nested"] = inner;
    
    QJsonObject result = JsonUtils::getObjectField(obj, "nested");
    
    QCOMPARE(result["key"].toString(), QString("value"));
    
    QJsonObject emptyResult = JsonUtils::getObjectField(obj, "missing");
    QVERIFY(emptyResult.isEmpty());
}

void TestJsonUtils::testGetArrayField()
{
    QJsonObject obj;
    QJsonArray arr;
    arr << 1 << 2 << 3;
    obj["items"] = arr;
    
    QJsonArray result = JsonUtils::getArrayField(obj, "items");
    
    QCOMPARE(result.size(), 3);
    QCOMPARE(result[0].toInt(), 1);
    
    QJsonArray emptyResult = JsonUtils::getArrayField(obj, "missing");
    QVERIFY(emptyResult.isEmpty());
}

// ==================== 边界情况测试 ====================

void TestJsonUtils::testGetStringFieldWithDefault()
{
    QJsonObject obj;
    obj["name"] = "Alice";
    
    QCOMPARE(JsonUtils::getStringField(obj, "name", "default"), QString("Alice"));
    QCOMPARE(JsonUtils::getStringField(obj, "missing", "default"), QString("default"));
}

void TestJsonUtils::testGetIntFieldWithDefault()
{
    QJsonObject obj;
    obj["age"] = 25;
    
    QCOMPARE(JsonUtils::getIntField(obj, "age", -1), 25);
    QCOMPARE(JsonUtils::getIntField(obj, "missing", -1), -1);
}

void TestJsonUtils::testGetFieldFromEmptyObject()
{
    QJsonObject emptyObj;
    
    QCOMPARE(JsonUtils::getStringField(emptyObj, "any"), QString());
    QCOMPARE(JsonUtils::getIntField(emptyObj, "any"), 0);
    QCOMPARE(JsonUtils::getBoolField(emptyObj, "any"), false);
    QCOMPARE(JsonUtils::getDoubleField(emptyObj, "any"), 0.0);
    QVERIFY(JsonUtils::getObjectField(emptyObj, "any").isEmpty());
    QVERIFY(JsonUtils::getArrayField(emptyObj, "any").isEmpty());
}

void TestJsonUtils::testGetFieldFromMissingKey()
{
    QJsonObject obj;
    obj["existing"] = "value";
    
    QCOMPARE(JsonUtils::getStringField(obj, "nonexistent"), QString());
    QCOMPARE(JsonUtils::getIntField(obj, "nonexistent"), 0);
    QVERIFY(JsonUtils::getObjectField(obj, "nonexistent").isEmpty());
}

// ==================== Character 序列化测试 ====================

void TestJsonUtils::testCharacterToJson()
{
    Character character;
    character.setId("char-001");
    character.setNovelId("novel-001");
    character.setName("Alice");
    character.setRole("protagonist");
    
    QStringList personality;
    personality << "brave" << "kind";
    character.setPersonality(personality);
    
    CharacterAppearance appearance;
    appearance.gender = "female";
    appearance.age = 18;
    appearance.hairColor = "blonde";
    character.setAppearance(appearance);
    
    QJsonObject json = JsonUtils::characterToJson(character);
    
    QCOMPARE(json["id"].toString(), QString("char-001"));
    QCOMPARE(json["novelId"].toString(), QString("novel-001"));
    QCOMPARE(json["name"].toString(), QString("Alice"));
    QCOMPARE(json["role"].toString(), QString("protagonist"));
    QVERIFY(json.contains("appearance"));
    QVERIFY(json.contains("personality"));
}

void TestJsonUtils::testJsonToCharacter()
{
    QJsonObject json;
    json["id"] = "char-002";
    json["novelId"] = "novel-002";
    json["name"] = "Bob";
    json["role"] = "antagonist";
    
    QJsonObject appearanceObj;
    appearanceObj["gender"] = "male";
    appearanceObj["age"] = 30;
    json["appearance"] = appearanceObj;
    
    QJsonArray personalityArr;
    personalityArr << "cunning" << "ambitious";
    json["personality"] = personalityArr;
    
    Character character = JsonUtils::jsonToCharacter(json);
    
    QCOMPARE(character.id(), QString("char-002"));
    QCOMPARE(character.novelId(), QString("novel-002"));
    QCOMPARE(character.name(), QString("Bob"));
    QCOMPARE(character.role(), QString("antagonist"));
    QCOMPARE(character.appearance().gender, QString("male"));
    QCOMPARE(character.appearance().age, 30);
    QCOMPARE(character.personality().size(), 2);
}

void TestJsonUtils::testCharacterRoundTrip()
{
    Character original;
    original.setId("char-003");
    original.setNovelId("novel-003");
    original.setName("Charlie");
    original.setRole("secondary");
    
    CharacterAppearance appearance;
    appearance.gender = "female";
    appearance.age = 25;
    appearance.hairColor = "black";
    appearance.hairStyle = "long";
    appearance.eyeColor = "brown";
    appearance.height = "165cm";
    appearance.build = "slim";
    appearance.clothing << "dress" << "boots";
    appearance.distinctiveFeatures << "scar on left cheek";
    original.setAppearance(appearance);
    
    QStringList personality;
    personality << "mysterious" << "intelligent";
    original.setPersonality(personality);
    
    QJsonObject json = JsonUtils::characterToJson(original);
    Character restored = JsonUtils::jsonToCharacter(json);
    
    QCOMPARE(restored.id(), original.id());
    QCOMPARE(restored.novelId(), original.novelId());
    QCOMPARE(restored.name(), original.name());
    QCOMPARE(restored.role(), original.role());
    QCOMPARE(restored.appearance().gender, original.appearance().gender);
    QCOMPARE(restored.appearance().age, original.appearance().age);
    QCOMPARE(restored.appearance().hairColor, original.appearance().hairColor);
    QCOMPARE(restored.appearance().hairStyle, original.appearance().hairStyle);
    QCOMPARE(restored.appearance().eyeColor, original.appearance().eyeColor);
    QCOMPARE(restored.appearance().height, original.appearance().height);
    QCOMPARE(restored.appearance().build, original.appearance().build);
    QCOMPARE(restored.appearance().clothing, original.appearance().clothing);
    QCOMPARE(restored.appearance().distinctiveFeatures, original.appearance().distinctiveFeatures);
    QCOMPARE(restored.personality(), original.personality());
}

void TestJsonUtils::testCharacterWithEmptyFields()
{
    Character character;
    character.setId("char-empty");
    
    QJsonObject json = JsonUtils::characterToJson(character);
    Character restored = JsonUtils::jsonToCharacter(json);
    
    QCOMPARE(restored.id(), QString("char-empty"));
    QVERIFY(restored.novelId().isEmpty());
    QVERIFY(restored.name().isEmpty());
    QVERIFY(restored.role().isEmpty());
}

// ==================== CharacterAppearance 序列化测试 ====================

void TestJsonUtils::testAppearanceToJson()
{
    CharacterAppearance appearance;
    appearance.gender = "female";
    appearance.age = 20;
    appearance.hairColor = "pink";
    appearance.hairStyle = "twintails";
    appearance.eyeColor = "blue";
    appearance.height = "160cm";
    appearance.build = "petite";
    appearance.clothing << "school uniform" << "ribbon";
    appearance.distinctiveFeatures << "heterochromia";
    
    QJsonObject json = JsonUtils::appearanceToJson(appearance);
    
    QCOMPARE(json["gender"].toString(), QString("female"));
    QCOMPARE(json["age"].toInt(), 20);
    QCOMPARE(json["hairColor"].toString(), QString("pink"));
    QCOMPARE(json["hairStyle"].toString(), QString("twintails"));
    QCOMPARE(json["eyeColor"].toString(), QString("blue"));
    QCOMPARE(json["height"].toString(), QString("160cm"));
    QCOMPARE(json["build"].toString(), QString("petite"));
    QCOMPARE(json["clothing"].toArray().size(), 2);
    QCOMPARE(json["distinctiveFeatures"].toArray().size(), 1);
}

void TestJsonUtils::testJsonToAppearance()
{
    QJsonObject json;
    json["gender"] = "male";
    json["age"] = 35;
    json["hairColor"] = "brown";
    json["hairStyle"] = "short";
    json["eyeColor"] = "green";
    json["height"] = "180cm";
    json["build"] = "athletic";
    
    QJsonArray clothingArr;
    clothingArr << "suit" << "tie";
    json["clothing"] = clothingArr;
    
    QJsonArray featuresArr;
    featuresArr << "beard" << "glasses";
    json["distinctiveFeatures"] = featuresArr;
    
    CharacterAppearance appearance = JsonUtils::jsonToAppearance(json);
    
    QCOMPARE(appearance.gender, QString("male"));
    QCOMPARE(appearance.age, 35);
    QCOMPARE(appearance.hairColor, QString("brown"));
    QCOMPARE(appearance.hairStyle, QString("short"));
    QCOMPARE(appearance.eyeColor, QString("green"));
    QCOMPARE(appearance.height, QString("180cm"));
    QCOMPARE(appearance.build, QString("athletic"));
    QCOMPARE(appearance.clothing.size(), 2);
    QCOMPARE(appearance.distinctiveFeatures.size(), 2);
}

void TestJsonUtils::testAppearanceRoundTrip()
{
    CharacterAppearance original;
    original.gender = "other";
    original.age = 100;
    original.hairColor = "silver";
    original.hairStyle = "bald";
    original.eyeColor = "gray";
    original.height = "200cm";
    original.build = "muscular";
    original.clothing << "armor" << "cape" << "crown";
    original.distinctiveFeatures << "glowing aura" << "floating";
    
    QJsonObject json = JsonUtils::appearanceToJson(original);
    CharacterAppearance restored = JsonUtils::jsonToAppearance(json);
    
    QCOMPARE(restored.gender, original.gender);
    QCOMPARE(restored.age, original.age);
    QCOMPARE(restored.hairColor, original.hairColor);
    QCOMPARE(restored.hairStyle, original.hairStyle);
    QCOMPARE(restored.eyeColor, original.eyeColor);
    QCOMPARE(restored.height, original.height);
    QCOMPARE(restored.build, original.build);
    QCOMPARE(restored.clothing, original.clothing);
    QCOMPARE(restored.distinctiveFeatures, original.distinctiveFeatures);
}

// ==================== Storyboard 序列化测试 ====================

void TestJsonUtils::testStoryboardToJson()
{
    Storyboard storyboard;
    storyboard.setId("sb-001");
    storyboard.setNovelId("novel-001");
    storyboard.setTotalPages(10);
    storyboard.setPanelCount(50);
    storyboard.setVersion(2);
    
    QJsonObject json = JsonUtils::storyboardToJson(storyboard);
    
    QCOMPARE(json["id"].toString(), QString("sb-001"));
    QCOMPARE(json["novelId"].toString(), QString("novel-001"));
    QCOMPARE(json["totalPages"].toInt(), 10);
    QCOMPARE(json["panelCount"].toInt(), 50);
    QCOMPARE(json["version"].toInt(), 2);
}

void TestJsonUtils::testJsonToStoryboard()
{
    QJsonObject json;
    json["id"] = "sb-002";
    json["novelId"] = "novel-002";
    json["totalPages"] = 20;
    json["panelCount"] = 100;
    json["version"] = 3;
    
    Storyboard storyboard = JsonUtils::jsonToStoryboard(json);
    
    QCOMPARE(storyboard.id(), QString("sb-002"));
    QCOMPARE(storyboard.novelId(), QString("novel-002"));
    QCOMPARE(storyboard.totalPages(), 20);
    QCOMPARE(storyboard.panelCount(), 100);
    QCOMPARE(storyboard.version(), 3);
}

void TestJsonUtils::testStoryboardRoundTrip()
{
    Storyboard original;
    original.setId("sb-003");
    original.setNovelId("novel-003");
    original.setTotalPages(15);
    original.setPanelCount(75);
    original.setVersion(5);
    
    QJsonObject json = JsonUtils::storyboardToJson(original);
    Storyboard restored = JsonUtils::jsonToStoryboard(json);
    
    QCOMPARE(restored.id(), original.id());
    QCOMPARE(restored.novelId(), original.novelId());
    QCOMPARE(restored.totalPages(), original.totalPages());
    QCOMPARE(restored.panelCount(), original.panelCount());
    QCOMPARE(restored.version(), original.version());
}

// ==================== Panel 序列化测试 ====================

void TestJsonUtils::testPanelToJson()
{
    Panel panel;
    panel.setId("panel-001");
    panel.setStoryboardId("sb-001");
    panel.setPage(1);
    panel.setIndex(2);
    panel.setVisualPrompt("dark forest with moonlight");
    panel.setPreviewS3Key("preview/panel-001.png");
    panel.setHdS3Key("hd/panel-001.png");
    
    QJsonObject content;
    content["scene"] = QString::fromUtf8("森林深处");
    content["dialogue"] = "Hello, world!";
    panel.setContent(content);
    
    QJsonObject json = JsonUtils::panelToJson(panel);
    
    QCOMPARE(json["id"].toString(), QString("panel-001"));
    QCOMPARE(json["storyboardId"].toString(), QString("sb-001"));
    QCOMPARE(json["page"].toInt(), 1);
    QCOMPARE(json["index"].toInt(), 2);
    QCOMPARE(json["scene"].toString(), QString::fromUtf8("森林深处"));
    QCOMPARE(json["visualPrompt"].toString(), QString("dark forest with moonlight"));
    QCOMPARE(json["previewS3Key"].toString(), QString("preview/panel-001.png"));
    QCOMPARE(json["hdS3Key"].toString(), QString("hd/panel-001.png"));
}

void TestJsonUtils::testJsonToPanel()
{
    QJsonObject json;
    json["id"] = "panel-002";
    json["storyboardId"] = "sb-002";
    json["page"] = 3;
    json["index"] = 4;
    json["visualPrompt"] = "busy city street at night";
    json["previewS3Key"] = "preview/panel-002.png";
    json["hdS3Key"] = "hd/panel-002.png";
    
    QJsonObject content;
    content["scene"] = QString::fromUtf8("城市街道");
    content["description"] = "night scene";
    json["content"] = content;
    
    Panel panel = JsonUtils::jsonToPanel(json);
    
    QCOMPARE(panel.id(), QString("panel-002"));
    QCOMPARE(panel.storyboardId(), QString("sb-002"));
    QCOMPARE(panel.page(), 3);
    QCOMPARE(panel.index(), 4);
    QCOMPARE(panel.scene(), QString::fromUtf8("城市街道"));
    QCOMPARE(panel.visualPrompt(), QString("busy city street at night"));
    QCOMPARE(panel.previewS3Key(), QString("preview/panel-002.png"));
    QCOMPARE(panel.hdS3Key(), QString("hd/panel-002.png"));
}

void TestJsonUtils::testPanelRoundTrip()
{
    Panel original;
    original.setId("panel-003");
    original.setStoryboardId("sb-003");
    original.setPage(5);
    original.setIndex(10);
    original.setVisualPrompt("mysterious cave with crystals");
    original.setPreviewS3Key("preview/panel-003.png");
    original.setHdS3Key("hd/panel-003.png");
    
    QJsonObject content;
    content["scene"] = QString::fromUtf8("神秘洞穴");
    content["mood"] = "mysterious";
    content["lighting"] = "dim";
    original.setContent(content);
    
    QJsonObject json = JsonUtils::panelToJson(original);
    Panel restored = JsonUtils::jsonToPanel(json);
    
    QCOMPARE(restored.id(), original.id());
    QCOMPARE(restored.storyboardId(), original.storyboardId());
    QCOMPARE(restored.page(), original.page());
    QCOMPARE(restored.index(), original.index());
    QCOMPARE(restored.scene(), original.scene());
    QCOMPARE(restored.visualPrompt(), original.visualPrompt());
    QCOMPARE(restored.previewS3Key(), original.previewS3Key());
    QCOMPARE(restored.hdS3Key(), original.hdS3Key());
}

void TestJsonUtils::testPanelSetSceneNotOverwrittenBySetContent()
{
    Panel panel;
    
    panel.setScene(QString::fromUtf8("先设置的场景"));
    
    QJsonObject content;
    content["dialogue"] = "Hello";
    panel.setContent(content);
    
    QCOMPARE(panel.scene(), QString::fromUtf8("先设置的场景"));
}

void TestJsonUtils::testPanelSetContentUpdatesExistingFields()
{
    Panel panel;
    panel.setScene(QString::fromUtf8("旧场景"));
    
    QJsonObject content;
    content["scene"] = QString::fromUtf8("新场景");
    panel.setContent(content);
    
    QCOMPARE(panel.scene(), QString::fromUtf8("新场景"));
}

void TestJsonUtils::testPanelMergeContent()
{
    Panel panel;
    
    QJsonObject initial;
    initial["scene"] = QString::fromUtf8("初始场景");
    initial["mood"] = "dark";
    panel.setContent(initial);
    
    QJsonObject extra;
    extra["lighting"] = "dim";
    extra["extra"] = "value";
    panel.mergeContent(extra);
    
    QCOMPARE(panel.rawContent()["scene"].toString(), QString::fromUtf8("初始场景"));
    QCOMPARE(panel.rawContent()["mood"].toString(), QString("dark"));
    QCOMPARE(panel.rawContent()["lighting"].toString(), QString("dim"));
    QCOMPARE(panel.rawContent()["extra"].toString(), QString("value"));
}

// ==================== Job 序列化测试 ====================

void TestJsonUtils::testJobToJson()
{
    Job job;
    job.setId("job-001");
    job.setType("storyboard");
    job.setStatus(JobStatus::Running);
    job.setNovelId("novel-001");
    job.setStoryboardId("sb-001");
    job.setTotal(100);
    job.setCompleted(50);
    job.setFailed(2);
    job.setCreatedAt(QDateTime::fromString("2024-01-15T10:30:00", Qt::ISODate));
    job.setUpdatedAt(QDateTime::fromString("2024-01-15T11:00:00", Qt::ISODate));
    
    QJsonObject json = JsonUtils::jobToJson(job);
    
    QCOMPARE(json["id"].toString(), QString("job-001"));
    QCOMPARE(json["type"].toString(), QString("storyboard"));
    QCOMPARE(json["status"].toString(), QString("running"));
    QCOMPARE(json["novelId"].toString(), QString("novel-001"));
    QCOMPARE(json["storyboardId"].toString(), QString("sb-001"));
    QCOMPARE(json["total"].toInt(), 100);
    QCOMPARE(json["completed"].toInt(), 50);
    QCOMPARE(json["failed"].toInt(), 2);
}

void TestJsonUtils::testJsonToJob()
{
    QJsonObject json;
    json["id"] = "job-002";
    json["type"] = "analysis";
    json["status"] = "completed";
    json["novelId"] = "novel-002";
    json["storyboardId"] = "sb-002";
    json["total"] = 200;
    json["completed"] = 200;
    json["failed"] = 0;
    json["createdAt"] = "2024-01-16T09:00:00";
    json["updatedAt"] = "2024-01-16T10:00:00";
    
    Job job = JsonUtils::jsonToJob(json);
    
    QCOMPARE(job.id(), QString("job-002"));
    QCOMPARE(job.type(), QString("analysis"));
    QCOMPARE(job.status(), JobStatus::Completed);
    QCOMPARE(job.novelId(), QString("novel-002"));
    QCOMPARE(job.storyboardId(), QString("sb-002"));
    QCOMPARE(job.total(), 200);
    QCOMPARE(job.completed(), 200);
    QCOMPARE(job.failed(), 0);
}

void TestJsonUtils::testJobRoundTrip()
{
    Job original;
    original.setId("job-003");
    original.setType("export");
    original.setStatus(JobStatus::Pending);
    original.setNovelId("novel-003");
    original.setStoryboardId("sb-003");
    original.setTotal(50);
    original.setCompleted(0);
    original.setFailed(0);
    original.setCreatedAt(QDateTime::currentDateTime());
    original.setUpdatedAt(QDateTime::currentDateTime());
    
    QJsonObject json = JsonUtils::jobToJson(original);
    Job restored = JsonUtils::jsonToJob(json);
    
    QCOMPARE(restored.id(), original.id());
    QCOMPARE(restored.type(), original.type());
    QCOMPARE(restored.status(), original.status());
    QCOMPARE(restored.novelId(), original.novelId());
    QCOMPARE(restored.storyboardId(), original.storyboardId());
    QCOMPARE(restored.total(), original.total());
    QCOMPARE(restored.completed(), original.completed());
    QCOMPARE(restored.failed(), original.failed());
}

// ==================== Variant 转换测试 ====================

void TestJsonUtils::testVariantToJson()
{
    QString jsonStr = "{\"key\":\"value\",\"number\":42}";
    QVariant variant(jsonStr.toUtf8());
    
    QJsonObject result = JsonUtils::variantToJson(variant);
    
    QCOMPARE(result["key"].toString(), QString("value"));
    QCOMPARE(result["number"].toInt(), 42);
}

void TestJsonUtils::testVariantMapToJsonString()
{
    QVariantMap map;
    map["name"] = "Test";
    map["count"] = 100;
    map["active"] = true;
    
    QString jsonStr = JsonUtils::variantMapToJsonString(map);
    
    QVERIFY(jsonStr.contains("\"name\""));
    QVERIFY(jsonStr.contains("\"Test\""));
    QVERIFY(jsonStr.contains("\"count\""));
    QVERIFY(jsonStr.contains("100"));
}

void TestJsonUtils::testJsonStringToVariantMap()
{
    QString jsonStr = "{\"title\":\"Hello\",\"value\":123,\"enabled\":true}";
    
    QVariantMap map = JsonUtils::jsonStringToVariantMap(jsonStr);
    
    QCOMPARE(map["title"].toString(), QString("Hello"));
    QCOMPARE(map["value"].toInt(), 123);
    QCOMPARE(map["enabled"].toBool(), true);
}

// ==================== 异常情况测试 ====================

void TestJsonUtils::testGetIntFieldFromString()
{
    QJsonObject obj;
    obj["age"] = QString::fromUtf8("不是数字");
    obj["count"] = "25";
    obj["validNumber"] = 30;
    
    QCOMPARE(JsonUtils::getIntField(obj, "age"), 0);
    QCOMPARE(JsonUtils::getIntField(obj, "count"), 0);
    QCOMPARE(JsonUtils::getIntField(obj, "validNumber"), 30);
}

void TestJsonUtils::testGetStringFieldFromNumber()
{
    QJsonObject obj;
    obj["price"] = 99.99;
    obj["count"] = 100;
    obj["name"] = "test";
    
    QCOMPARE(JsonUtils::getStringField(obj, "price"), QString());
    QCOMPARE(JsonUtils::getStringField(obj, "count"), QString());
    QCOMPARE(JsonUtils::getStringField(obj, "name"), QString("test"));
}

void TestJsonUtils::testGetBoolFieldFromString()
{
    QJsonObject obj;
    obj["active"] = "true";
    obj["deleted"] = "false";
    obj["valid"] = true;
    obj["invalid"] = false;
    
    QCOMPARE(JsonUtils::getBoolField(obj, "active"), false);
    QCOMPARE(JsonUtils::getBoolField(obj, "deleted"), false);
    QCOMPARE(JsonUtils::getBoolField(obj, "valid"), true);
    QCOMPARE(JsonUtils::getBoolField(obj, "invalid"), false);
}

void TestJsonUtils::testStringToJsonInvalid()
{
    QString invalidJson = "{name: 'test'}";
    
    QJsonObject result = JsonUtils::stringToJson(invalidJson);
    
    QVERIFY(result.isEmpty());
}

void TestJsonUtils::testStringToJsonEmpty()
{
    QJsonObject result1 = JsonUtils::stringToJson("");
    QVERIFY(result1.isEmpty());
    
    QJsonObject result2 = JsonUtils::stringToJson("   ");
    QVERIFY(result2.isEmpty());
    
    QJsonObject result3 = JsonUtils::stringToJson("null");
    QVERIFY(result3.isEmpty());
}

void TestJsonUtils::testCharacterWithSpecialChars()
{
    Character character;
    character.setId("char-special");
    character.setName(QString::fromUtf8("名字\n换行\t制表"));
    
    QJsonObject json = JsonUtils::characterToJson(character);
    Character restored = JsonUtils::jsonToCharacter(json);
    
    QCOMPARE(restored.name(), character.name());
}

void TestJsonUtils::testCharacterWithEmoji()
{
    Character character;
    character.setId("char-emoji");
    character.setName(QString::fromUtf8("小明😊🎉🔥"));
    
    QStringList personality;
    personality << QString::fromUtf8("开朗😄") << QString::fromUtf8("热情❤️");
    character.setPersonality(personality);
    
    QJsonObject json = JsonUtils::characterToJson(character);
    Character restored = JsonUtils::jsonToCharacter(json);
    
    QCOMPARE(restored.name(), QString::fromUtf8("小明😊🎉🔥"));
    QCOMPARE(restored.personality().size(), 2);
    QCOMPARE(restored.personality()[0], QString::fromUtf8("开朗😄"));
}

void TestJsonUtils::testVeryLongString()
{
    QString longText(10000, QChar('A'));
    
    QJsonObject obj;
    obj["text"] = longText;
    
    QString result = JsonUtils::getStringField(obj, "text");
    QCOMPARE(result.length(), 10000);
    
    QString jsonStr = JsonUtils::jsonToString(obj);
    QVERIFY(jsonStr.length() > 10000);
    
    QJsonObject parsed = JsonUtils::stringToJson(jsonStr);
    QCOMPARE(parsed["text"].toString().length(), 10000);
}

void TestJsonUtils::testNullField()
{
    QJsonObject obj;
    obj["name"] = QJsonValue::Null;
    obj["age"] = QJsonValue::Null;
    obj["active"] = QJsonValue::Null;
    
    QCOMPARE(JsonUtils::getStringField(obj, "name"), QString());
    QCOMPARE(JsonUtils::getIntField(obj, "age"), 0);
    QCOMPARE(JsonUtils::getBoolField(obj, "active"), false);
}

void TestJsonUtils::testEmptyArrayField()
{
    QJsonObject obj;
    obj["items"] = QJsonArray();
    obj["tags"] = QJsonArray();
    
    QJsonArray items = JsonUtils::getArrayField(obj, "items");
    QVERIFY(items.isEmpty());
    
    QStringList tags = JsonUtils::jsonArrayToStringList(items);
    QVERIFY(tags.isEmpty());
}

void TestJsonUtils::testNestedEmptyObject()
{
    QJsonObject obj;
    QJsonObject nested;
    nested["inner"] = QJsonObject();
    obj["nested"] = nested;
    
    QJsonObject result = JsonUtils::getObjectField(obj, "nested");
    QVERIFY(result.contains("inner"));
    
    QJsonObject inner = JsonUtils::getObjectField(result, "inner");
    QVERIFY(inner.isEmpty());
}

// ==================== 高级测试 ====================

void TestJsonUtils::testEscapeCharacters()
{
    QString json = QString::fromUtf8("{\"text\":\"line1\\nline2\\ttab\\\"quote\\\"\"}");
    QJsonObject obj = JsonUtils::stringToJson(json);
    
    QCOMPARE(obj["text"].toString(), QString::fromUtf8("line1\nline2\ttab\"quote\""));
    
    Character character;
    character.setId("char-escape");
    character.setName(QString::fromUtf8("名字\n换行\t制表"));
    
    QJsonObject charJson = JsonUtils::characterToJson(character);
    Character restored = JsonUtils::jsonToCharacter(charJson);
    
    QCOMPARE(restored.name(), character.name());
}

void TestJsonUtils::testUnicodeEscape()
{
    QString json = QString::fromUtf8("{\"name\":\"\\u4E2D\\u6587\",\"emoji\":\"\\uD83D\\uDE00\"}");
    QJsonObject obj = JsonUtils::stringToJson(json);
    
    QCOMPARE(obj["name"].toString(), QString::fromUtf8("中文"));
    
    Character character;
    character.setId("char-unicode");
    character.setName(QString::fromUtf8("中文名字测试"));
    character.setRole(QString::fromUtf8("主角"));
    
    QJsonObject json1 = JsonUtils::characterToJson(character);
    Character restored = JsonUtils::jsonToCharacter(json1);
    
    QCOMPARE(restored.name(), QString::fromUtf8("中文名字测试"));
    QCOMPARE(restored.role(), QString::fromUtf8("主角"));
}

void TestJsonUtils::testScientificNotation()
{
    QString json = "{\"small\":1.5e-10,\"large\":6.022e23,\"negative\":-2.5e5}";
    QJsonObject obj = JsonUtils::stringToJson(json);
    
    QVERIFY(obj.contains("small"));
    QVERIFY(obj.contains("large"));
    QVERIFY(obj.contains("negative"));
    
    double small = obj["small"].toDouble();
    double large = obj["large"].toDouble();
    double negative = obj["negative"].toDouble();
    
    QVERIFY(small > 0);
    QVERIFY(large > 0);
    QVERIFY(negative < 0);
}

void TestJsonUtils::testExtremeNumbers()
{
    QJsonObject obj;
    obj["maxInt"] = 2147483647;
    obj["minInt"] = -2147483648;
    obj["maxInt64"] = 9223372036854775807LL;
    obj["zero"] = 0;
    obj["negativeZero"] = -0.0;
    obj["verySmallDouble"] = 1.7976931348623157e-308;
    
    QCOMPARE(JsonUtils::getIntField(obj, "maxInt"), 2147483647);
    QCOMPARE(JsonUtils::getIntField(obj, "minInt"), -2147483648);
    QCOMPARE(JsonUtils::getIntField(obj, "zero"), 0);
    
    double verySmall = obj["verySmallDouble"].toDouble();
    QVERIFY(verySmall > 0);
    QVERIFY(verySmall < 1);
}

QTEST_APPLESS_MAIN(TestJsonUtils)
#include "tst_jsonutils.moc"
