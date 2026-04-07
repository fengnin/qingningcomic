#include <QtTest/QtTest>
#include <QJsonArray>
#include <QJsonObject>
#include "CharacterExtractor.h"
#include "Character.h"

class TestCharacterExtractor : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    // ExtractedCharacter 测试
    void testExtractedCharacterToJson();
    void testExtractedCharacterFromJson();
    void testExtractedCharacterRoundTrip();

    // extractFromPanels 测试
    void testExtractFromPanelsBasic();
    void testExtractFromPanelsDuplicate();
    void testExtractFromPanelsEmpty();
    void testExtractFromPanelsWithStringCharacter();
    void testExtractFromPanelsWithObjectCharacter();

    // extractFromCharacters 测试
    void testExtractFromCharactersBasic();
    void testExtractFromCharactersEmpty();
    void testExtractFromCharactersWithAppearance();

private:
    QJsonObject createTestPanel(const QString& scene, const QJsonArray& characters);
    QJsonObject createTestCharacter(const QString& name, const QString& expression, const QString& pose);
};

void TestCharacterExtractor::initTestCase()
{
    qDebug() << "========================================";
    qDebug() << "  CharacterExtractor 单元测试开始";
    qDebug() << "========================================";
}

void TestCharacterExtractor::cleanupTestCase()
{
    qDebug() << "========================================";
    qDebug() << "  CharacterExtractor 单元测试结束";
    qDebug() << "========================================";
}

// ==================== ExtractedCharacter 测试 ====================

void TestCharacterExtractor::testExtractedCharacterToJson()
{
    ExtractedCharacter c;
    c.name = "Alice";
    c.gender = "female";
    c.age = 18;
    c.hairColor = "blonde";
    c.hairStyle = "long";
    c.eyeColor = "blue";
    c.bodyType = "slim";
    c.clothing = "dress, boots";
    c.features = "scar on cheek";
    c.personality << "brave" << "kind";
    c.tags << "protagonist";
    
    QJsonObject json = c.toJson();
    
    QCOMPARE(json["name"].toString(), QString("Alice"));
    QCOMPARE(json["gender"].toString(), QString("female"));
    QCOMPARE(json["age"].toInt(), 18);
    QCOMPARE(json["hairColor"].toString(), QString("blonde"));
    QCOMPARE(json["personality"].toArray().size(), 2);
    QCOMPARE(json["tags"].toArray().size(), 1);
}

void TestCharacterExtractor::testExtractedCharacterFromJson()
{
    QJsonObject json;
    json["name"] = "Bob";
    json["gender"] = "male";
    json["age"] = 25;
    json["hairColor"] = "black";
    json["hairStyle"] = "short";
    json["eyeColor"] = "brown";
    json["bodyType"] = "athletic";
    json["clothing"] = "suit, tie";
    json["features"] = "glasses";
    
    QJsonArray personality;
    personality << "serious" << "intelligent";
    json["personality"] = personality;
    
    QJsonArray tags;
    tags << "mentor";
    json["tags"] = tags;
    
    ExtractedCharacter c = ExtractedCharacter::fromJson(json);
    
    QCOMPARE(c.name, QString("Bob"));
    QCOMPARE(c.gender, QString("male"));
    QCOMPARE(c.age, 25);
    QCOMPARE(c.hairColor, QString("black"));
    QCOMPARE(c.personality.size(), 2);
    QCOMPARE(c.tags.size(), 1);
}

void TestCharacterExtractor::testExtractedCharacterRoundTrip()
{
    ExtractedCharacter original;
    original.name = "Charlie";
    original.gender = "other";
    original.age = 30;
    original.hairColor = "red";
    original.personality << "mysterious";
    original.tags << "secondary";
    
    QJsonObject json = original.toJson();
    ExtractedCharacter restored = ExtractedCharacter::fromJson(json);
    
    QCOMPARE(restored.name, original.name);
    QCOMPARE(restored.gender, original.gender);
    QCOMPARE(restored.age, original.age);
    QCOMPARE(restored.hairColor, original.hairColor);
    QCOMPARE(restored.personality, original.personality);
    QCOMPARE(restored.tags, original.tags);
}

// ==================== extractFromPanels 测试 ====================

QJsonObject TestCharacterExtractor::createTestPanel(const QString& scene, const QJsonArray& characters)
{
    QJsonObject panel;
    panel["scene"] = scene;
    panel["characters"] = characters;
    return panel;
}

QJsonObject TestCharacterExtractor::createTestCharacter(const QString& name, const QString& expression, const QString& pose)
{
    QJsonObject c;
    c["name"] = name;
    c["expression"] = expression;
    c["pose"] = pose;
    return c;
}

void TestCharacterExtractor::testExtractFromPanelsBasic()
{
    QJsonArray panels;
    
    QJsonArray chars1;
    chars1 << createTestCharacter("Alice", "happy", "standing");
    chars1 << createTestCharacter("Bob", "serious", "sitting");
    panels.append(createTestPanel("Scene 1", chars1));
    
    QList<ExtractedCharacter> result = CharacterExtractor::instance()->extractFromPanels(panels);
    
    QCOMPARE(result.size(), 2);
    QCOMPARE(result[0].name, QString("Alice"));
    QCOMPARE(result[1].name, QString("Bob"));
}

void TestCharacterExtractor::testExtractFromPanelsDuplicate()
{
    QJsonArray panels;
    
    QJsonArray chars1;
    chars1 << createTestCharacter("Alice", "happy", "standing");
    panels.append(createTestPanel("Scene 1", chars1));
    
    QJsonArray chars2;
    chars2 << createTestCharacter("Alice", "sad", "sitting");
    chars2 << createTestCharacter("Bob", "serious", "standing");
    panels.append(createTestPanel("Scene 2", chars2));
    
    QList<ExtractedCharacter> result = CharacterExtractor::instance()->extractFromPanels(panels);
    
    QCOMPARE(result.size(), 2);
}

void TestCharacterExtractor::testExtractFromPanelsEmpty()
{
    QJsonArray panels;
    
    QList<ExtractedCharacter> result = CharacterExtractor::instance()->extractFromPanels(panels);
    
    QVERIFY(result.isEmpty());
}

void TestCharacterExtractor::testExtractFromPanelsWithStringCharacter()
{
    QJsonArray panels;
    
    QJsonArray chars;
    chars << QJsonValue("Alice");
    chars << QJsonValue(QString::fromUtf8("张三 (左脸颊有疤痕)"));
    panels.append(createTestPanel("Scene 1", chars));
    
    QList<ExtractedCharacter> result = CharacterExtractor::instance()->extractFromPanels(panels);
    
    QCOMPARE(result.size(), 2);
    QCOMPARE(result[0].name, QString("Alice"));
    QCOMPARE(result[1].name, QString::fromUtf8("张三"));
}

void TestCharacterExtractor::testExtractFromPanelsWithObjectCharacter()
{
    QJsonArray panels;
    
    QJsonArray chars;
    chars << createTestCharacter("Charlie", "angry", "standing");
    panels.append(createTestPanel("Scene 1", chars));
    
    QList<ExtractedCharacter> result = CharacterExtractor::instance()->extractFromPanels(panels);
    
    QCOMPARE(result.size(), 1);
    QCOMPARE(result[0].name, QString("Charlie"));
    QVERIFY(result[0].features.contains("angry"));
    QVERIFY(result[0].features.contains("standing"));
}

// ==================== extractFromCharacters 测试 ====================

void TestCharacterExtractor::testExtractFromCharactersBasic()
{
    QJsonArray characters;
    
    QJsonObject char1;
    char1["name"] = "Alice";
    characters.append(char1);
    
    QJsonObject char2;
    char2["name"] = "Bob";
    characters.append(char2);
    
    QList<ExtractedCharacter> result = CharacterExtractor::instance()->extractFromCharacters(characters);
    
    QCOMPARE(result.size(), 2);
    QCOMPARE(result[0].name, QString("Alice"));
    QCOMPARE(result[1].name, QString("Bob"));
}

void TestCharacterExtractor::testExtractFromCharactersEmpty()
{
    QJsonArray characters;
    
    QList<ExtractedCharacter> result = CharacterExtractor::instance()->extractFromCharacters(characters);
    
    QVERIFY(result.isEmpty());
}

void TestCharacterExtractor::testExtractFromCharactersWithAppearance()
{
    QJsonArray characters;
    
    QJsonObject charObj;
    charObj["name"] = "Bob";
    
    QJsonObject appearance;
    appearance["gender"] = "male";
    appearance["age"] = 25;
    appearance["hairColor"] = "black";
    appearance["hairStyle"] = "short";
    appearance["eyeColor"] = "brown";
    appearance["build"] = "athletic";
    
    QJsonArray clothing;
    clothing << "suit" << "tie";
    appearance["clothing"] = clothing;
    
    QJsonArray features;
    features << "glasses";
    appearance["distinctiveFeatures"] = features;
    
    charObj["appearance"] = appearance;
    
    QJsonArray personality;
    personality << "serious";
    charObj["personality"] = personality;
    
    QJsonArray tags;
    tags << "mentor";
    charObj["tags"] = tags;
    
    characters.append(charObj);
    
    QList<ExtractedCharacter> result = CharacterExtractor::instance()->extractFromCharacters(characters);
    
    QCOMPARE(result.size(), 1);
    QCOMPARE(result[0].name, QString("Bob"));
    QCOMPARE(result[0].gender, QString("male"));
    QCOMPARE(result[0].age, 25);
    QCOMPARE(result[0].hairColor, QString("black"));
    QCOMPARE(result[0].clothing, QString("suit, tie"));
    QCOMPARE(result[0].features, QString("glasses"));
    QCOMPARE(result[0].personality.size(), 1);
    QCOMPARE(result[0].tags.size(), 1);
}

QTEST_APPLESS_MAIN(TestCharacterExtractor)
#include "tst_characterextractor.moc"
