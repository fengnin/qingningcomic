#include "api/QwenPromptBuilder.h"
#include "SchemaToPrompt.h"
#include <QFile>
#include <QJsonDocument>

QString QwenPromptBuilder::buildSystemPrompt(const QString& schemaPath)
{
    QFile schemaFile(schemaPath);
    QJsonObject schema;
    
    if (schemaFile.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(schemaFile.readAll());
        schema = doc.object();
        schemaFile.close();
    }
    
    if (schema.isEmpty()) {
        return QString::fromUtf8("你是一个专业的漫画分镜师，擅长将小说文本转换为详细的视觉分镜脚本。请严格按照 JSON Schema 输出。");
    }
    
    SchemaToPrompt::Options options;
    options.customExample = buildCustomExample();
    
    return SchemaToPrompt::buildSystemPrompt(schema, options);
}

QString QwenPromptBuilder::buildUserMessageWithBible(
    const QString& text,
    const QJsonArray& existingCharacters,
    const QJsonArray& existingScenes,
    int chapterNumber)
{
    QString userMessage = text;
    
    if (existingCharacters.isEmpty() && existingScenes.isEmpty()) {
        return userMessage;
    }
    
    userMessage = QString::fromUtf8("章节 %1：\n\n").arg(chapterNumber > 0 ? QString::number(chapterNumber) : "?");
    
    if (!existingCharacters.isEmpty()) {
        userMessage += QString::fromUtf8("【现有角色圣经】请在生成的 characters 数组中包含以下所有角色，并保持其 appearance 不变：\n");
        userMessage += QString::fromUtf8(QJsonDocument(existingCharacters).toJson(QJsonDocument::Indented));
        userMessage += "\n\n";
    }
    
    if (!existingScenes.isEmpty()) {
        userMessage += QString::fromUtf8("【现有场景圣经】请在生成的 scenes 数组中包含以下所有场景，并保持其 visualCharacteristics 不变。在 panel.background.sceneId 中优先使用这些场景ID：\n");
        userMessage += QString::fromUtf8(QJsonDocument(existingScenes).toJson(QJsonDocument::Indented));
        userMessage += "\n\n";
    }
    
    userMessage += QString::fromUtf8("【新章节文本】\n") + text;
    
    return userMessage;
}

QString QwenPromptBuilder::buildChangeRequestPrompt()
{
    return QString::fromUtf8("你是一个漫画修改助手。\n\n"
        "你的任务是将用户的自然语言修改请求转换为结构化的 CR-DSL（Change Request Domain Specific Language）。\n\n"
        "CR-DSL 包含以下要素：\n"
        "- scope: 修改范围\n"
        "- targetId: 目标 ID（如果适用）\n"
        "- type: 修改类型\n"
        "- ops: 操作列表，每个操作包含 action 和 params\n\n"
        "可用的 action：\n"
        "- inpaint: 局部重绘（需要遮罩区域）\n"
        "- outpaint: 扩展画面\n"
        "- bg_swap: 替换背景\n"
        "- repose: 改变角色姿势\n"
        "- regen_panel: 重新生成整个面板\n"
        "- rewrite_dialogue: 重写对白\n"
        "- reorder: 重新排序\n\n"
        "请根据用户的自然语言请求，生成符合 Schema 的 CR-DSL JSON。");
}

QString QwenPromptBuilder::buildDialogueRewritePrompt()
{
    return QString::fromUtf8("你是一个专业的对白编辑。\n\n"
        "你的任务是根据用户的指示重写漫画对白，保持角色性格和语气，同时满足修改要求。\n\n"
        "要求：\n"
        "1. 保持对白简洁（漫画气泡空间有限）\n"
        "2. 符合角色性格\n"
        "3. 自然流畅\n"
        "4. 直接输出重写后的对白，不要添加引号或解释");
}

QJsonObject QwenPromptBuilder::buildExamplePanel()
{
    QJsonObject panel;
    panel["page"] = 1;
    panel["index"] = 0;
    panel["scene"] = QString::fromUtf8("夕阳西下，金色余晖洒在小镇石板路上。李明背着书包独自走在回家路上，街道两旁是低矮砖房。");
    
    QJsonObject background;
    background["sceneId"] = QString::fromUtf8("ancient_town_main_street");
    background["setting"] = QString::fromUtf8("古镇石板街道");
    background["timeOfDay"] = QString::fromUtf8("dusk");
    background["weather"] = QString::fromUtf8("clear");
    background["lighting"] = QString::fromUtf8("natural");
    background["details"] = QJsonArray{QString::fromUtf8("远处山峦"), QString::fromUtf8("街边灯笼"), QString::fromUtf8("砖墙上的爬山虎")};
    panel["background"] = background;
    
    QJsonObject atmosphere;
    atmosphere["mood"] = QString::fromUtf8("peaceful");
    atmosphere["soundEffects"] = QJsonArray{QJsonObject{{"sound", QString::fromUtf8("脚步声")}, {"style", QString::fromUtf8("subtle")}}};
    atmosphere["particleEffects"] = QJsonArray{QString::fromUtf8("光尘飘浮"), QString::fromUtf8("微风吹动树叶")};
    panel["atmosphere"] = atmosphere;
    
    panel["shotType"] = QString::fromUtf8("wide");
    panel["cameraAngle"] = QString::fromUtf8("eye-level");
    
    QJsonObject composition;
    composition["focusPoint"] = QString::fromUtf8("李明的侧影");
    composition["depthOfField"] = QString::fromUtf8("deep");
    composition["rule"] = QString::fromUtf8("rule-of-thirds");
    panel["composition"] = composition;
    
    QJsonObject artStyle;
    artStyle["genre"] = QString::fromUtf8("seinen");
    artStyle["lineWeight"] = QString::fromUtf8("medium");
    artStyle["shading"] = QString::fromUtf8("screentone");
    artStyle["colorPalette"] = QString::fromUtf8("warm sunset tones with golden oranges");
    panel["artStyle"] = artStyle;
    
    panel["characters"] = QJsonArray{QJsonObject{
        {"name", QString::fromUtf8("李明")},
        {"pose", QString::fromUtf8("缓慢行走，微微低头")},
        {"expression", QString::fromUtf8("neutral")},
        {"position", QString::fromUtf8("midground")}
    }};
    panel["dialogue"] = QJsonArray();
    panel["visualPrompt"] = QString::fromUtf8("夕阳下的古镇街道，金色阳光洒在青石板路上，穿着校服的少年背着书包独自走在街上，两旁是低矮的砖房，远处是连绵的山峦，温暖宁静的氛围，少年漫画风格，网点纸阴影");
    panel["visualPromptEn"] = QString::fromUtf8("A quiet small town street at sunset, golden light on cobblestone pavement, teenage boy in school uniform walking alone with backpack, low brick houses on both sides, distant mountains, warm peaceful atmosphere, seinen manga style, screentone shading");
    panel["narrativeFunction"] = QString::fromUtf8("establishing-shot");
    
    return panel;
}

QJsonObject QwenPromptBuilder::buildExampleCharacter()
{
    QJsonObject character;
    character["name"] = QString::fromUtf8("李明");
    character["role"] = QString::fromUtf8("protagonist");
    
    QJsonObject appearance;
    appearance["gender"] = QString::fromUtf8("male");
    appearance["age"] = 16;
    appearance["hairColor"] = QString::fromUtf8("black");
    appearance["hairStyle"] = QString::fromUtf8("short");
    appearance["eyeColor"] = QString::fromUtf8("brown");
    appearance["height"] = QString::fromUtf8("average");
    appearance["build"] = QString::fromUtf8("slim");
    appearance["clothing"] = QJsonArray{QString::fromUtf8("校服"), QString::fromUtf8("书包")};
    appearance["distinctiveFeatures"] = QJsonArray{QString::fromUtf8("圆框眼镜")};
    character["appearance"] = appearance;
    character["personality"] = QJsonArray{QString::fromUtf8("内向"), QString::fromUtf8("善良"), QString::fromUtf8("细心")};
    
    return character;
}

QJsonObject QwenPromptBuilder::buildExampleScene()
{
    QJsonObject scene;
    scene["id"] = QString::fromUtf8("ancient_town_main_street");
    scene["name"] = QString::fromUtf8("古镇主街");
    scene["type"] = QString::fromUtf8("outdoor");
    scene["description"] = QString::fromUtf8("小镇中心的古老石板路，两旁是传统砖木结构低矮房屋，承载着小镇几代人的记忆。");
    
    QJsonObject visualChars;
    visualChars["architecture"] = QString::fromUtf8("传统砖木结构，青瓦屋顶，木质门窗，墙面有岁月痕迹");
    visualChars["keyLandmarks"] = QJsonArray{QString::fromUtf8("石拱桥"), QString::fromUtf8("百年老槐树"), QString::fromUtf8("李家茶馆")};
    visualChars["colorScheme"] = QString::fromUtf8("暖色调为主，砖红、木褐、青灰色");
    
    QJsonObject lighting;
    lighting["naturalLight"] = QString::fromUtf8("abundant");
    lighting["artificialLight"] = QString::fromUtf8("moderate");
    lighting["lightSources"] = QJsonArray{QString::fromUtf8("街灯"), QString::fromUtf8("店铺灯光"), QString::fromUtf8("窗户透出的光")};
    visualChars["lighting"] = lighting;
    
    visualChars["atmosphere"] = QString::fromUtf8("宁静中带着烟火气，时间在这里流淌得很慢");
    visualChars["soundscape"] = QJsonArray{QString::fromUtf8("脚步声"), QString::fromUtf8("风铃"), QString::fromUtf8("远处狗吠"), QString::fromUtf8("茶馆里的谈笑")};
    visualChars["textures"] = QJsonArray{QString::fromUtf8("粗糙石板路"), QString::fromUtf8("斑驳墙面"), QString::fromUtf8("光滑木门")};
    scene["visualCharacteristics"] = visualChars;
    
    QJsonObject spatialLayout;
    spatialLayout["size"] = QString::fromUtf8("medium");
    spatialLayout["layout"] = QString::fromUtf8("长约200米的街道，宽约6米，两侧各有店铺和民居");
    spatialLayout["keyAreas"] = QJsonArray{
        QJsonObject{{"name", QString::fromUtf8("石拱桥")}, {"position", QString::fromUtf8("街道中段")}},
        QJsonObject{{"name", QString::fromUtf8("老槐树")}, {"position", QString::fromUtf8("街道北端")}},
        QJsonObject{{"name", QString::fromUtf8("李家茶馆")}, {"position", QString::fromUtf8("街道南侧")}}
    };
    scene["spatialLayout"] = spatialLayout;
    
    scene["timeVariations"] = QJsonArray{
        QJsonObject{
            {"timeOfDay", QString::fromUtf8("morning")},
            {"description", QString::fromUtf8("晨光透过薄雾，石板路泛着湿润光泽，早起的居民开始活动")}
        },
        QJsonObject{
            {"timeOfDay", QString::fromUtf8("dusk")},
            {"description", QString::fromUtf8("夕阳将街道染成金色，街灯开始点亮，炊烟袅袅升起")}
        }
    };
    
    scene["weatherVariations"] = QJsonArray{
        QJsonObject{
            {"weather", QString::fromUtf8("rainy")},
            {"description", QString::fromUtf8("雨水在石板路上形成小水洼，屋檐滴水声清脆，空气中弥漫着泥土香")}
        }
    };
    
    scene["narrativeRole"] = QString::fromUtf8("primary-setting");
    
    QJsonObject firstAppearance;
    firstAppearance["chapter"] = 1;
    firstAppearance["page"] = 1;
    firstAppearance["panelIndex"] = 0;
    scene["firstAppearance"] = firstAppearance;
    
    scene["referenceImages"] = QJsonArray();
    
    return scene;
}

QJsonObject QwenPromptBuilder::buildCustomExample()
{
    QJsonObject example;
    example["panels"] = QJsonArray{buildExamplePanel()};
    example["characters"] = QJsonArray{buildExampleCharacter()};
    example["scenes"] = QJsonArray{buildExampleScene()};
    example["totalPages"] = 1;
    return example;
}
