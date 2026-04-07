#include <QtTest/QtTest>
#include "Panel.h"

class TestPanel : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testDefaultConstructor();
    void testSettersAndGetters();
    void testCharacterPose();
    void testCharacterPoseParameterized();
    void testDialogueLine();
    void testDialogueLineParameterized();
    void testAddCharacter();
    void testAddDialogue();
    void testContent();
    void testSetContent();
    void testSetContentPartial();
    void testMergeContent();
    void testPositionLabel();
    void testCharactersText();
    void testDialogueText();
    void testRawContent();
};

void TestPanel::initTestCase()
{
    qDebug() << "========================================";
    qDebug() << "  Panel Model Unit Tests Start";
    qDebug() << "========================================";
}

void TestPanel::cleanupTestCase()
{
    qDebug() << "========================================";
    qDebug() << "  Panel Model Unit Tests End";
    qDebug() << "========================================";
}

void TestPanel::testDefaultConstructor()
{
    Panel panel;
    
    QVERIFY(panel.id().isEmpty());
    QVERIFY(panel.storyboardId().isEmpty());
    QCOMPARE(panel.page(), 0);
    QCOMPARE(panel.index(), 0);
    QVERIFY(panel.scene().isEmpty());
    QVERIFY(panel.shotType().isEmpty());
    QVERIFY(panel.cameraAngle().isEmpty());
    QVERIFY(panel.visualPrompt().isEmpty());
    QVERIFY(panel.visualPromptEn().isEmpty());
    QCOMPARE(panel.status(), QString("pending"));
    QVERIFY(panel.previewS3Key().isEmpty());
    QVERIFY(panel.hdS3Key().isEmpty());
    QVERIFY(panel.previewUrl().isEmpty());
    QVERIFY(panel.hdUrl().isEmpty());
    QVERIFY(panel.characters().isEmpty());
    QVERIFY(panel.dialogue().isEmpty());
}

void TestPanel::testSettersAndGetters()
{
    Panel panel;
    
    panel.setId("panel-001");
    QCOMPARE(panel.id(), QString("panel-001"));
    
    panel.setStoryboardId("sb-001");
    QCOMPARE(panel.storyboardId(), QString("sb-001"));
    
    panel.setPage(5);
    QCOMPARE(panel.page(), 5);
    
    panel.setIndex(3);
    QCOMPARE(panel.index(), 3);
    
    panel.setScene(QString::fromUtf8("神秘森林"));
    QCOMPARE(panel.scene(), QString::fromUtf8("神秘森林"));
    
    panel.setShotType("close-up");
    QCOMPARE(panel.shotType(), QString("close-up"));
    
    panel.setCameraAngle("high-angle");
    QCOMPARE(panel.cameraAngle(), QString("high-angle"));
    
    panel.setVisualPrompt(QString::fromUtf8("一个神秘的场景"));
    QCOMPARE(panel.visualPrompt(), QString::fromUtf8("一个神秘的场景"));
    
    panel.setVisualPromptEn("A mysterious scene");
    QCOMPARE(panel.visualPromptEn(), QString("A mysterious scene"));
    
    panel.setStatus("completed");
    QCOMPARE(panel.status(), QString("completed"));
    
    panel.setPreviewS3Key("preview/key.png");
    QCOMPARE(panel.previewS3Key(), QString("preview/key.png"));
    
    panel.setHdS3Key("hd/key.png");
    QCOMPARE(panel.hdS3Key(), QString("hd/key.png"));
    
    panel.setPreviewUrl("https://example.com/preview.png");
    QCOMPARE(panel.previewUrl(), QString("https://example.com/preview.png"));
    
    panel.setHdUrl("https://example.com/hd.png");
    QCOMPARE(panel.hdUrl(), QString("https://example.com/hd.png"));
}

void TestPanel::testCharacterPose()
{
    CharacterPose pose;
    
    QVERIFY(pose.charId.isEmpty());
    QVERIFY(pose.name.isEmpty());
    QVERIFY(pose.pose.isEmpty());
    QVERIFY(pose.expression.isEmpty());
}

void TestPanel::testCharacterPoseParameterized()
{
    CharacterPose pose("char-001", QString::fromUtf8("张三"), "standing", "happy");
    
    QCOMPARE(pose.charId, QString("char-001"));
    QCOMPARE(pose.name, QString::fromUtf8("张三"));
    QCOMPARE(pose.pose, QString("standing"));
    QCOMPARE(pose.expression, QString("happy"));
}

void TestPanel::testDialogueLine()
{
    DialogueLine line;
    
    QVERIFY(line.speaker.isEmpty());
    QVERIFY(line.text.isEmpty());
    QCOMPARE(line.bubbleType, QString("speech"));
}

void TestPanel::testDialogueLineParameterized()
{
    DialogueLine line(QString::fromUtf8("李四"), QString::fromUtf8("你好"), "shout");
    
    QCOMPARE(line.speaker, QString::fromUtf8("李四"));
    QCOMPARE(line.text, QString::fromUtf8("你好"));
    QCOMPARE(line.bubbleType, QString("shout"));
}

void TestPanel::testAddCharacter()
{
    Panel panel;
    
    CharacterPose pose1("char-001", QString::fromUtf8("角色1"));
    CharacterPose pose2("char-002", QString::fromUtf8("角色2"), "sitting", "serious");
    
    panel.addCharacter(pose1);
    panel.addCharacter(pose2);
    
    QCOMPARE(panel.characters().size(), 2);
    QCOMPARE(panel.characters()[0].name, QString::fromUtf8("角色1"));
    QCOMPARE(panel.characters()[1].name, QString::fromUtf8("角色2"));
    QCOMPARE(panel.characters()[1].pose, QString("sitting"));
}

void TestPanel::testAddDialogue()
{
    Panel panel;
    
    DialogueLine line1(QString::fromUtf8("张三"), QString::fromUtf8("今天天气真好"));
    DialogueLine line2(QString::fromUtf8("李四"), QString::fromUtf8("是啊"), "thought");
    
    panel.addDialogue(line1);
    panel.addDialogue(line2);
    
    QCOMPARE(panel.dialogue().size(), 2);
    QCOMPARE(panel.dialogue()[0].speaker, QString::fromUtf8("张三"));
    QCOMPARE(panel.dialogue()[1].bubbleType, QString("thought"));
}

void TestPanel::testContent()
{
    Panel panel;
    panel.setScene(QString::fromUtf8("测试场景"));
    panel.setShotType("medium");
    panel.setCameraAngle("low-angle");
    
    QJsonObject content = panel.content();
    
    QVERIFY(content.contains("scene"));
    QCOMPARE(content["scene"].toString(), QString::fromUtf8("测试场景"));
    QCOMPARE(content["shotType"].toString(), QString("medium"));
    QCOMPARE(content["cameraAngle"].toString(), QString("low-angle"));
}

void TestPanel::testSetContent()
{
    Panel panel;
    
    QJsonObject content;
    content["scene"] = QString::fromUtf8("新场景");
    content["shotType"] = "wide";
    content["cameraAngle"] = "eye-level";
    
    panel.setContent(content);
    
    QCOMPARE(panel.scene(), QString::fromUtf8("新场景"));
    QCOMPARE(panel.shotType(), QString("wide"));
    QCOMPARE(panel.cameraAngle(), QString("eye-level"));
}

void TestPanel::testSetContentPartial()
{
    Panel panel;
    panel.setScene(QString::fromUtf8("原有场景"));
    panel.setShotType("close-up");
    
    QJsonObject content;
    content["shotType"] = "long";
    
    panel.setContent(content);
    
    QCOMPARE(panel.scene(), QString::fromUtf8("原有场景"));
    QCOMPARE(panel.shotType(), QString("long"));
}

void TestPanel::testMergeContent()
{
    Panel panel;
    
    QJsonObject initial;
    initial["scene"] = QString::fromUtf8("原始场景");
    panel.setContent(initial);
    
    QJsonObject extra;
    extra["visualPrompt"] = QString::fromUtf8("额外提示词");
    extra["customField"] = "customValue";
    
    panel.mergeContent(extra);
    
    QJsonObject raw = panel.rawContent();
    QCOMPARE(raw["scene"].toString(), QString::fromUtf8("原始场景"));
    QVERIFY(raw.contains("visualPrompt"));
    QVERIFY(raw.contains("customField"));
}

void TestPanel::testPositionLabel()
{
    Panel panel;
    panel.setPage(2);
    panel.setIndex(5);
    
    QString label = panel.positionLabel();
    
    QVERIFY(label.contains("2"));
    QVERIFY(label.contains("5"));
}

void TestPanel::testCharactersText()
{
    Panel panel;
    
    CharacterPose pose1("char-001", QString::fromUtf8("张三"), "standing", "happy");
    CharacterPose pose2("char-002", QString::fromUtf8("李四"), "sitting", "serious");
    
    panel.addCharacter(pose1);
    panel.addCharacter(pose2);
    
    QString text = panel.charactersText();
    
    QVERIFY(text.contains(QString::fromUtf8("张三")));
    QVERIFY(text.contains(QString::fromUtf8("李四")));
}

void TestPanel::testDialogueText()
{
    Panel panel;
    
    DialogueLine line1(QString::fromUtf8("张三"), QString::fromUtf8("你好"));
    DialogueLine line2(QString::fromUtf8("李四"), QString::fromUtf8("再见"));
    
    panel.addDialogue(line1);
    panel.addDialogue(line2);
    
    QString text = panel.dialogueText();
    
    QVERIFY(text.contains(QString::fromUtf8("张三")));
    QVERIFY(text.contains(QString::fromUtf8("你好")));
    QVERIFY(text.contains(QString::fromUtf8("李四")));
    QVERIFY(text.contains(QString::fromUtf8("再见")));
}

void TestPanel::testRawContent()
{
    Panel panel;
    
    QJsonObject raw;
    raw["customField"] = "customValue";
    raw["anotherField"] = 123;
    
    panel.setRawContent(raw);
    
    QJsonObject retrieved = panel.rawContent();
    QCOMPARE(retrieved["customField"].toString(), QString("customValue"));
    QCOMPARE(retrieved["anotherField"].toInt(), 123);
}

QTEST_APPLESS_MAIN(TestPanel)
#include "tst_panel.moc"
