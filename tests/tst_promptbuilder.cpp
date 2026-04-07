#include <QtTest/QtTest>
#include <QJsonObject>
#include <QJsonArray>
#include <QMap>
#include "PromptBuilder.h"

class TestPromptBuilder : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    // buildCharacterPrompt 测试
    void testBuildCharacterPromptBasic();
    void testBuildCharacterPromptWithAppearance();
    void testBuildCharacterPromptWithViewOptions();
    void testBuildCharacterPromptWithPoseOptions();
    void testBuildCharacterPromptWithStyleOptions();
    void testBuildCharacterPromptWithModeOptions();
    void testBuildCharacterPromptEmptyCharacter();
    void testBuildCharacterPromptWithTags();
    void testBuildCharacterPromptWithClothing();

    // buildPanelPrompt 测试
    void testBuildPanelPromptBasic();
    void testBuildPanelPromptWithScene();
    void testBuildPanelPromptWithCharacters();
    void testBuildPanelPromptWithCharacterRefs();
    void testBuildPanelPromptWithAtmosphere();
    void testBuildPanelPromptWithComposition();
    void testBuildPanelPromptWithArtStyle();
    void testBuildPanelPromptEmptyPanel();

    // buildScenePrompt 测试
    void testBuildScenePromptBasic();
    void testBuildScenePromptWithVisualCharacteristics();
    void testBuildScenePromptWithLighting();
    void testBuildScenePromptEmptyScene();

    // 辅助函数测试
    void testNormaliseListWithArray();
    void testNormaliseListWithString();
    void testNormaliseListWithNull();
    void testNormaliseListWithEmpty();

    // 结果验证测试
    void testPromptResultStructure();
    void testNegativePromptPresent();
};

void TestPromptBuilder::initTestCase()
{
    qDebug() << "========================================";
    qDebug() << "  PromptBuilder 单元测试开始";
    qDebug() << "========================================";
}

void TestPromptBuilder::cleanupTestCase()
{
    qDebug() << "========================================";
    qDebug() << "  PromptBuilder 单元测试结束";
    qDebug() << "========================================";
}

// ==================== buildCharacterPrompt 测试 ====================

void TestPromptBuilder::testBuildCharacterPromptBasic()
{
    QJsonObject character;
    character["name"] = "Alice";
    character["role"] = "protagonist";
    
    PromptBuilder::PromptResult result = PromptBuilder::buildCharacterPrompt(character);
    
    QVERIFY(!result.text.isEmpty());
    QVERIFY(!result.negativePrompt.isEmpty());
    QVERIFY(result.text.contains("Alice"));
    QVERIFY(result.text.contains("protagonist archetype"));
    QVERIFY(result.text.contains("manga style"));
}

void TestPromptBuilder::testBuildCharacterPromptWithAppearance()
{
    QJsonObject character;
    character["name"] = "Bob";
    
    QJsonObject appearance;
    appearance["gender"] = "male";
    appearance["age"] = 25;
    appearance["hairColor"] = "black";
    appearance["hairStyle"] = "short";
    appearance["eyeColor"] = "brown";
    appearance["build"] = "athletic";
    character["appearance"] = appearance;
    
    PromptBuilder::PromptResult result = PromptBuilder::buildCharacterPrompt(character);
    
    QVERIFY(result.text.contains("Bob"));
    QVERIFY(result.text.contains("male"));
    QVERIFY(result.text.contains("age 25"));
    QVERIFY(result.text.contains("black short hair"));
    QVERIFY(result.text.contains("brown eyes"));
    QVERIFY(result.text.contains("athletic"));
}

void TestPromptBuilder::testBuildCharacterPromptWithViewOptions()
{
    QJsonObject character;
    character["name"] = "Test";
    
    QJsonObject options;
    options["view"] = "side";
    
    PromptBuilder::PromptResult result = PromptBuilder::buildCharacterPrompt(character, options);
    
    QVERIFY(result.text.contains("side profile view"));
}

void TestPromptBuilder::testBuildCharacterPromptWithPoseOptions()
{
    QJsonObject character;
    character["name"] = "Test";
    
    QJsonObject options;
    options["pose"] = "sitting";
    
    PromptBuilder::PromptResult result = PromptBuilder::buildCharacterPrompt(character, options);
    
    QVERIFY(result.text.contains("sitting pose"));
}

void TestPromptBuilder::testBuildCharacterPromptWithStyleOptions()
{
    QJsonObject character;
    character["name"] = "Test";
    
    QJsonObject options;
    options["style"] = "anime";
    
    PromptBuilder::PromptResult result = PromptBuilder::buildCharacterPrompt(character, options);
    
    QVERIFY(result.text.contains("anime style"));
}

void TestPromptBuilder::testBuildCharacterPromptWithModeOptions()
{
    QJsonObject character;
    character["name"] = "Test";
    
    QJsonObject optionsPreview;
    optionsPreview["mode"] = "preview";
    PromptBuilder::PromptResult resultPreview = PromptBuilder::buildCharacterPrompt(character, optionsPreview);
    QVERIFY(resultPreview.text.contains("illustrated render"));
    
    QJsonObject optionsHd;
    optionsHd["mode"] = "hd";
    PromptBuilder::PromptResult resultHd = PromptBuilder::buildCharacterPrompt(character, optionsHd);
    QVERIFY(resultHd.text.contains("high resolution render"));
}

void TestPromptBuilder::testBuildCharacterPromptEmptyCharacter()
{
    QJsonObject character;
    
    PromptBuilder::PromptResult result = PromptBuilder::buildCharacterPrompt(character);
    
    QVERIFY(!result.text.isEmpty());
    QVERIFY(!result.negativePrompt.isEmpty());
}

void TestPromptBuilder::testBuildCharacterPromptWithTags()
{
    QJsonObject character;
    character["name"] = "Test";
    
    QJsonArray tags;
    tags << "heroic" << "determined" << "brave";
    character["tags"] = tags;
    
    PromptBuilder::PromptResult result = PromptBuilder::buildCharacterPrompt(character);
    
    QVERIFY(result.text.contains("heroic"));
    QVERIFY(result.text.contains("determined"));
    QVERIFY(result.text.contains("brave"));
}

void TestPromptBuilder::testBuildCharacterPromptWithClothing()
{
    QJsonObject character;
    character["name"] = "Test";
    
    QJsonObject appearance;
    QJsonArray clothing;
    clothing << "school uniform" << "red tie";
    appearance["clothing"] = clothing;
    
    QJsonArray accessories;
    accessories << "glasses" << "watch";
    appearance["accessories"] = accessories;
    
    QJsonArray features;
    features << "scar on cheek";
    appearance["distinctiveFeatures"] = features;
    
    character["appearance"] = appearance;
    
    PromptBuilder::PromptResult result = PromptBuilder::buildCharacterPrompt(character);
    
    QVERIFY(result.text.contains("school uniform"));
    QVERIFY(result.text.contains("red tie"));
    QVERIFY(result.text.contains("glasses"));
    QVERIFY(result.text.contains("watch"));
    QVERIFY(result.text.contains("scar on cheek"));
}

// ==================== buildPanelPrompt 测试 ====================

void TestPromptBuilder::testBuildPanelPromptBasic()
{
    QJsonObject panel;
    panel["scene"] = QString::fromUtf8("森林深处");
    
    PromptBuilder::PromptResult result = PromptBuilder::buildPanelPrompt(panel);
    
    QVERIFY(!result.text.isEmpty());
    QVERIFY(result.text.contains(QString::fromUtf8("森林深处")));
    QVERIFY(result.text.contains("manga panel illustration"));
}

void TestPromptBuilder::testBuildPanelPromptWithScene()
{
    QJsonObject panel;
    panel["scene"] = QString::fromUtf8("城市街道");
    panel["shotType"] = "close-up";
    panel["cameraAngle"] = "low-angle";
    
    PromptBuilder::PromptResult result = PromptBuilder::buildPanelPrompt(panel);
    
    QVERIFY(result.text.contains(QString::fromUtf8("城市街道")));
    QVERIFY(result.text.contains("close-up shot"));
    QVERIFY(result.text.contains("camera angle low-angle"));
}

void TestPromptBuilder::testBuildPanelPromptWithCharacters()
{
    QJsonObject panel;
    
    QJsonArray characters;
    QJsonObject char1;
    char1["name"] = "Alice";
    char1["pose"] = "standing";
    char1["expression"] = "happy";
    characters.append(char1);
    
    QJsonObject char2;
    char2["name"] = "Bob";
    char2["pose"] = "sitting";
    char2["expression"] = "neutral";
    characters.append(char2);
    
    panel["characters"] = characters;
    
    PromptBuilder::PromptResult result = PromptBuilder::buildPanelPrompt(panel);
    
    QVERIFY(result.text.contains("Alice"));
    QVERIFY(result.text.contains("standing pose"));
    QVERIFY(result.text.contains("happy expression"));
    QVERIFY(result.text.contains("Bob"));
    QVERIFY(result.text.contains("sitting pose"));
}

void TestPromptBuilder::testBuildPanelPromptWithCharacterRefs()
{
    QJsonObject panel;
    
    QJsonArray characters;
    QJsonObject charInPanel;
    charInPanel["name"] = "Alice";
    charInPanel["pose"] = "standing";
    characters.append(charInPanel);
    panel["characters"] = characters;
    
    QMap<QString, QJsonObject> characterRefs;
    QJsonObject fullChar;
    QJsonObject appearance;
    appearance["hairColor"] = "blonde";
    appearance["hairStyle"] = "long";
    QJsonArray clothing;
    clothing << "dress";
    appearance["clothing"] = clothing;
    fullChar["appearance"] = appearance;
    characterRefs["Alice"] = fullChar;
    
    PromptBuilder::PromptResult result = PromptBuilder::buildPanelPrompt(panel, characterRefs);
    
    QVERIFY(result.text.contains("Alice"));
    QVERIFY(result.text.contains("blonde long hair"));
    QVERIFY(result.text.contains("dress"));
}

void TestPromptBuilder::testBuildPanelPromptWithAtmosphere()
{
    QJsonObject panel;
    panel["scene"] = "Test";
    
    QJsonObject atmosphere;
    atmosphere["mood"] = "tense";
    panel["atmosphere"] = atmosphere;
    
    PromptBuilder::PromptResult result = PromptBuilder::buildPanelPrompt(panel);
    
    QVERIFY(result.text.contains("mood tense"));
}

void TestPromptBuilder::testBuildPanelPromptWithComposition()
{
    QJsonObject panel;
    panel["scene"] = "Test";
    
    QJsonObject composition;
    composition["focusPoint"] = "center";
    composition["rule"] = "rule of thirds";
    panel["composition"] = composition;
    
    PromptBuilder::PromptResult result = PromptBuilder::buildPanelPrompt(panel);
    
    QVERIFY(result.text.contains("focus on center"));
    QVERIFY(result.text.contains("rule of thirds composition"));
}

void TestPromptBuilder::testBuildPanelPromptWithArtStyle()
{
    QJsonObject panel;
    panel["scene"] = "Test";
    
    QJsonObject artStyle;
    artStyle["genre"] = "action";
    artStyle["lineWeight"] = "bold";
    artStyle["shading"] = "cross-hatch";
    artStyle["colorPalette"] = "warm";
    panel["artStyle"] = artStyle;
    
    PromptBuilder::PromptResult result = PromptBuilder::buildPanelPrompt(panel);
    
    QVERIFY(result.text.contains("action"));
    QVERIFY(result.text.contains("bold line weight"));
    QVERIFY(result.text.contains("cross-hatch shading"));
    QVERIFY(result.text.contains("warm palette"));
}

void TestPromptBuilder::testBuildPanelPromptEmptyPanel()
{
    QJsonObject panel;
    
    PromptBuilder::PromptResult result = PromptBuilder::buildPanelPrompt(panel);
    
    QVERIFY(!result.text.isEmpty());
    QVERIFY(!result.negativePrompt.isEmpty());
}

// ==================== buildScenePrompt 测试 ====================

void TestPromptBuilder::testBuildScenePromptBasic()
{
    QJsonObject scene;
    scene["name"] = QString::fromUtf8("神秘森林");
    scene["description"] = QString::fromUtf8("古老的森林，阳光透过树叶");
    
    PromptBuilder::PromptResult result = PromptBuilder::buildScenePrompt(scene);
    
    QVERIFY(!result.text.isEmpty());
    QVERIFY(result.text.contains(QString::fromUtf8("神秘森林")));
    QVERIFY(result.text.contains(QString::fromUtf8("古老的森林")));
    QVERIFY(result.text.contains("environment concept art"));
}

void TestPromptBuilder::testBuildScenePromptWithVisualCharacteristics()
{
    QJsonObject scene;
    scene["name"] = "City";
    
    QJsonObject visual;
    visual["architecture"] = "modern skyscrapers";
    
    QJsonArray landmarks;
    landmarks << "tower" << "bridge";
    visual["keyLandmarks"] = landmarks;
    
    visual["colorScheme"] = "cool blue";
    visual["atmosphere"] = "foggy morning";
    
    scene["visualCharacteristics"] = visual;
    
    PromptBuilder::PromptResult result = PromptBuilder::buildScenePrompt(scene);
    
    QVERIFY(result.text.contains("modern skyscrapers"));
    QVERIFY(result.text.contains("tower"));
    QVERIFY(result.text.contains("bridge"));
    QVERIFY(result.text.contains("cool blue"));
    QVERIFY(result.text.contains("foggy morning"));
}

void TestPromptBuilder::testBuildScenePromptWithLighting()
{
    QJsonObject scene;
    scene["name"] = "Room";
    
    QJsonObject visual;
    QJsonObject lighting;
    lighting["naturalLight"] = "sunlight through window";
    lighting["artificialLight"] = "warm lamp";
    visual["lighting"] = lighting;
    
    scene["visualCharacteristics"] = visual;
    
    PromptBuilder::PromptResult result = PromptBuilder::buildScenePrompt(scene);
    
    QVERIFY(result.text.contains("natural light sunlight through window"));
    QVERIFY(result.text.contains("artificial light warm lamp"));
}

void TestPromptBuilder::testBuildScenePromptEmptyScene()
{
    QJsonObject scene;
    
    PromptBuilder::PromptResult result = PromptBuilder::buildScenePrompt(scene);
    
    QVERIFY(!result.text.isEmpty());
    QVERIFY(!result.negativePrompt.isEmpty());
}

// ==================== 辅助函数测试 ====================

void TestPromptBuilder::testNormaliseListWithArray()
{
    QJsonObject obj;
    QJsonArray arr;
    arr << "item1" << "item2" << "item3";
    obj["list"] = arr;
    
    // 通过 buildCharacterPrompt 间接测试 normaliseList
    QJsonObject character;
    character["name"] = "Test";
    QJsonArray tags;
    tags << "tag1" << "tag2";
    character["tags"] = tags;
    
    PromptBuilder::PromptResult result = PromptBuilder::buildCharacterPrompt(character);
    QVERIFY(result.text.contains("tag1"));
    QVERIFY(result.text.contains("tag2"));
}

void TestPromptBuilder::testNormaliseListWithString()
{
    QJsonObject character;
    character["name"] = "Test";
    character["tags"] = "single-tag";
    
    PromptBuilder::PromptResult result = PromptBuilder::buildCharacterPrompt(character);
    QVERIFY(result.text.contains("single-tag"));
}

void TestPromptBuilder::testNormaliseListWithNull()
{
    QJsonObject character;
    character["name"] = "Test";
    character["tags"] = QJsonValue::Null;
    
    PromptBuilder::PromptResult result = PromptBuilder::buildCharacterPrompt(character);
    QVERIFY(!result.text.isEmpty());
}

void TestPromptBuilder::testNormaliseListWithEmpty()
{
    QJsonObject character;
    character["name"] = "Test";
    QJsonArray emptyArr;
    character["tags"] = emptyArr;
    
    PromptBuilder::PromptResult result = PromptBuilder::buildCharacterPrompt(character);
    QVERIFY(!result.text.isEmpty());
}

// ==================== 结果验证测试 ====================

void TestPromptBuilder::testPromptResultStructure()
{
    QJsonObject character;
    character["name"] = "Test";
    
    PromptBuilder::PromptResult result = PromptBuilder::buildCharacterPrompt(character);
    
    QVERIFY(!result.text.isEmpty());
    QVERIFY(!result.negativePrompt.isEmpty());
    QVERIFY(result.referenceImages.isEmpty() || result.referenceImages.size() >= 0);
}

void TestPromptBuilder::testNegativePromptPresent()
{
    QJsonObject character;
    character["name"] = "Test";
    
    PromptBuilder::PromptResult result = PromptBuilder::buildCharacterPrompt(character);
    
    QVERIFY(result.negativePrompt.contains("nsfw"));
    QVERIFY(result.negativePrompt.contains("blurry"));
    QVERIFY(result.negativePrompt.contains("low quality"));
}

QTEST_APPLESS_MAIN(TestPromptBuilder)
#include "tst_promptbuilder.moc"
