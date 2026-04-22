#include "api/QwenPromptBuilder.h"
#include "services/BibleContextInjector.h"
#include "utils/SchemaToPrompt.h"
#include "utils/Logger.h"
#include <QFile>
#include <QJsonDocument>
#include <QCoreApplication>
#include <QDir>

namespace {
QJsonObject getStoryboardExample()
{
    static const QJsonObject example = QJsonDocument::fromJson(R"({
        "panels": [
            {
                "page": 1,
                "index": 0,
                "scene": "夕阳西下，金色余晖洒在小镇石板路上。李明背着书包独自走在回家路上，街道两旁是低矮砖房。",
                "background": {
                    "sceneId": "ancient_town_main_street",
                    "setting": "古镇石板街道",
                    "timeOfDay": "黄昏",
                    "weather": "晴朗",
                    "lighting": "自然光",
                    "details": ["远处山峦", "街边灯笼", "砖墙上的爬山虎"]
                },
                "atmosphere": {
                    "mood": "宁静",
                    "soundEffects": [
                        {"sound": "脚步声", "style": "轻微"},
                        {"sound": "风铃声", "style": "轻微"}
                    ],
                    "particleEffects": ["光尘飘浮", "微风吹动树叶"]
                },
                "shotType": "远景",
                "cameraAngle": "平视",
                "composition": {
                    "focusPoint": "李明的侧影",
                    "depthOfField": "深景深",
                    "rule": "三分法"
                },
                "artStyle": {
                    "genre": "青年漫画",
                    "lineWeight": "中等",
                    "shading": "网点",
                    "colorPalette": "暖色调夕阳金橙"
                },
                "characters": [
                    {
                        "name": "李明",
                        "pose": "缓慢行走，微微低头",
                        "expression": "平静",
                        "position": "中景"
                    }
                ],
                "dialogue": [],
                "visualPrompt": "A quiet small town street at sunset, golden light on cobblestone pavement, teenage boy in school uniform walking alone with backpack, low brick houses on both sides, distant mountains, warm peaceful atmosphere, seinen manga style, screentone shading",
                "narrativeFunction": "开场镜头"
            }
        ],
        "characters": [
            {
                "name": "李明",
                "role": "protagonist",
                "appearance": {
                    "gender": "男",
                    "age": 16,
                    "hairColor": "黑色",
                    "hairStyle": "短发",
                    "eyeColor": "棕色",
                    "height": "中等",
                    "build": "清瘦",
                    "clothing": ["校服", "书包"],
                    "distinctiveFeatures": ["圆框眼镜"]
                },
                "personality": ["内向", "善良", "细心"]
            }
        ],
        "scenes": [
            {
                "id": "ancient_town_main_street",
                "name": "古镇主街",
                "type": "室外",
                "description": "小镇中心的古老石板路，两旁是传统砖木结构低矮房屋，承载着小镇几代人的记忆。",
                "visualCharacteristics": {
                    "architecture": "传统砖木结构，青瓦屋顶，木质门窗，墙面有岁月痕迹",
                    "keyLandmarks": ["石拱桥", "百年老槐树", "李家茶馆"],
                    "colorScheme": "暖色调为主，砖红、木褐、青灰色",
                    "lighting": {
                        "naturalLight": "充足",
                        "artificialLight": "适中",
                        "lightSources": ["街灯", "店铺灯光", "窗户透出的光"]
                    },
                    "atmosphere": "宁静中带着烟火气，时间在这里流淌得很慢",
                    "soundscape": ["脚步声", "风铃", "远处狗吠", "茶馆里的谈笑"],
                    "textures": ["粗糙石板路", "斑驳墙面", "光滑木门"]
                },
                "spatialLayout": {
                    "size": "中等",
                    "layout": "长约200米的街道，宽约6米，两侧各有店铺和民居",
                    "keyAreas": [
                        {"name": "石拱桥", "position": "街道中段"},
                        {"name": "老槐树", "position": "街道北端"},
                        {"name": "李家茶馆", "position": "街道南侧"}
                    ]
                },
                "timeVariations": [
                    {
                        "timeOfDay": "早晨",
                        "description": "晨光透过薄雾，石板路泛着湿润光泽，早起的居民开始活动"
                    },
                    {
                        "timeOfDay": "黄昏",
                        "description": "夕阳将街道染成金色，街灯开始点亮，炊烟袅袅升起"
                    }
                ],
                "weatherVariations": [
                    {
                        "weather": "雨天",
                        "description": "雨水在石板路上形成小水洼，屋檐滴水声清脆，空气中弥漫着泥土香"
                    }
                ],
                "narrativeRole": "主要场景",
                "firstAppearance": {
                    "chapter": 1,
                    "page": 1,
                    "panelIndex": 0
                },
                "referenceImages": []
            }
        ],
        "totalPages": 1
    })").object();
    return example;
}
}

QString QwenPromptBuilder::buildSystemPrompt(const QString& schemaPath)
{
    QString absolutePath = schemaPath;
    if (QDir::isRelativePath(schemaPath)) {
        absolutePath = QDir(QCoreApplication::applicationDirPath()).filePath(schemaPath);
    }
    
    QFile schemaFile(absolutePath);
    QJsonObject schema;
    
    if (schemaFile.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(schemaFile.readAll());
        schema = doc.object();
        schemaFile.close();
    }
    
    if (schema.isEmpty()) {
        return QString::fromUtf8("\u65e0\u6cd5\u52a0\u8f7dSchema\u914d\u7f6e\u6587\u4ef6");
    }
    
    SchemaToPrompt::Options options;
    options.customExample = getStoryboardExample();
    QString prompt = SchemaToPrompt::buildSystemPrompt(schema, options);
    
    LOG_DEBUG("QwenPromptBuilder", QString("System prompt length: %1 chars, example keys: %2")
        .arg(prompt.length())
        .arg(options.customExample.keys().join(", ")));
    
    return prompt;
}

QString QwenPromptBuilder::buildUserMessageWithBible(
    const QString& text,
    const QJsonArray& existingCharacters,
    const QJsonArray& existingScenes,
    int chapterNumber)
{
    return BibleContextInjector::instance()->buildContextPrompt(
        text, existingCharacters, existingScenes, chapterNumber);
}

QString QwenPromptBuilder::buildChangeRequestPrompt()
{
    return QStringLiteral(
        "你是漫画分镜编辑助手。\n\n"
        "请将用户的自然语言修改需求转换为结构化 CR-DSL JSON。\n"
        "只输出 JSON，不要添加额外说明。\n"
    );
}

QString QwenPromptBuilder::buildDialogueRewritePrompt()
{
    return QStringLiteral(
        "你是对白改写助手。\n\n"
        "请根据用户的修改要求，重写原对白。\n"
        "要求：\n"
        "1. 保持原意和人物关系不变。\n"
        "2. 语言自然，符合漫画对白风格。\n"
        "3. 不要输出分析过程。\n"
        "4. 只返回改写后的对白文本。\n"
    );
}
