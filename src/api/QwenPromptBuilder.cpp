#include "api/QwenPromptBuilder.h"
#include "services/BibleContextInjector.h"
#include "utils/SchemaToPrompt.h"
#include "utils/SchemaFileUtils.h"
#include "utils/ChangeRequestParseUtils.h"
#include "utils/Logger.h"

namespace {
QString joinPromptLines(const QStringList& lines)
{
    return lines.join(QStringLiteral("\n"));
}

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

QString buildSystemPromptFromSchema(const QString& schemaPath)
{
    const QJsonObject schema = SchemaFileUtils::readSchemaObject(schemaPath);
    if (schema.isEmpty()) {
        return QString::fromUtf8("无法加载Schema配置文件");
    }

    SchemaToPrompt::Options options;
    options.customExample = getStoryboardExample();
    options.additionalInstructions = QString::fromUtf8(
        "📌 **角色输出补充要求**：\n\n"
        "- characters 数组中的每个角色都必须尽量填写完整的 appearance 对象，不要只输出 name / role。\n"
        "- 只要正文中有明确依据，就填写 gender、age、hairColor、hairStyle、eyeColor、height、build、clothing、distinctiveFeatures。\n"
        "- 无法从正文确认的字段可以留空，但不要省略 appearance 字段本身。\n"
        "- 优先使用正文中的直接描述，不要凭空补充未明示的外观细节。\n\n"

        "🎯 **visualPrompt 生成规范（必须严格遵守）**：\n\n"

        "**0. 必须同时输出 visualPrompt（英文）和 visualPromptCn（中文）两个字段**\n"
        "- visualPrompt：英文图像生成指令，供图像模型使用\n"
        "- visualPromptCn：与 visualPrompt 内容完全对应的中文版本，供用户在界面上阅读和编辑\n"
        "- 两个字段描述的内容必须一致，只是语言不同\n"
        "- 示例：visualPrompt='young woman standing behind counter, light blue apron'，visualPromptCn='年轻女性站在柜台后，浅蓝色围裙'\n\n"

        "**1. 禁止文学性表达，使用视觉语言**\n"
        "- ❌ 错误：'立于逆光门框中'、'柔光漫溢'、'视线交汇'\n"
        "- ✅ 正确：'standing in doorway, backlit'、'soft light from window'、'two characters facing each other'\n"
        "- 必须使用具体的、可执行的视觉描述，不要使用抽象的文学修辞\n\n"

        "**2. 场景位置必须精确**\n"
        "- ❌ 错误：'门口'、'窗边'、'室内'（太模糊）\n"
        "- ✅ 正确：'从店内朝向玻璃门方向'、'sitting behind counter facing door'\n"
        "- 必须明确：内外视角、拍摄方向、人物朝向\n\n"

        "**3. 人物位置使用空间坐标语言**\n"
        "- ❌ 错误：'在画面中央'、'相对而立'（太抽象）\n"
        "- ✅ 正确：'character A on left side, character B on right side'、'两人呈L形构图'\n"
        "- 必须说明：左右分布、前后关系、距离远近\n\n"

        "**4. 使用标准化的姿态和表情描述**\n"
        "- 姿态示例：'standing pose, upright posture' / 'sitting pose, relaxed position' / 'walking pose, mid-stride'\n"
        "- 表情示例：'happy expression, bright smile' / 'sad expression, downcast eyes' / 'angry expression, furrowed brows'\n"
        "- 不要使用模糊的情感词，要用具体的面部表情描述\n\n"

        "**5. 对话场景必须包含气泡元素和说话者位置**\n"
        "- 如果 panel 的 dialogue 数组不为空，visualPrompt 必须包含：'speech bubble with text: \"xxx\"'\n"
        "- 必须描述对话双方的位置和视线方向\n"
        "- 示例：'young woman sitting behind counter, speech bubble: \"爷爷，您找什么书？\" said by 青柠'\n"
        "- dialogue 数组中每条对白必须设置 speakerSide 字段：说话者在画面左侧填 'left'，右侧填 'right'，居中填 'center'\n"
        "- 示例：{\"speaker\": \"青柠\", \"text\": \"爷爷，您找什么书？\", \"speakerSide\": \"right\"}\n\n"

        "**6. 光影描述要具体可执行**\n"
        "- ❌ 禁止：'柔光'、'逆光'、'光影交错'（过于抽象）\n"
        "- ✅ 要求：'soft light from left window'、'backlit, light from behind'、'natural light from glass door'\n"
        "- 必须说明：光源方向、光线强度、整体色调\n\n"

        "**7. 构图使用几何语言**\n"
        "- ❌ 避免：'氛围温馨'、'情感丰富'（无法量化）\n"
        "- ✅ 使用：'diagonal composition'、'rule-of-thirds'、'L-shaped composition'\n"
        "- 说明主体在画面中的占比和空间关系\n\n"

        "**8. 参考图一致性检查**\n"
        "- visualPrompt中的角色外观必须与Bible完全一致\n"
        "- 场景元素要与已有场景参考图保持一致\n"
        "- 如果有参考图，prompt中提到的特征必须在参考图中能找到对应\n\n"

        "**9. ⚠️ Bible角色外观绝对优先（最高优先级）**\n"
        "- ❌ **严格禁止**：在visualPrompt中修改或忽略Bible中的角色外观设定\n"
        "- ✅ **必须遵守**：发色、发型、眼睛颜色、衣服必须与Bible完全一致\n"
        "- **示例**：如果Bible指定'白发'，visualPrompt中禁止写'黑褐色头发'或'棕色头发'\n"
        "- **示例**：如果Bible指定'浅蓝色围裙, 白色棉质衬衫, 及膝牛仔裙'，visualPrompt必须包含这3件衣服\n"
        "- **示例**：如果Bible指定'浅琥珀色眼睛'，visualPrompt禁止写'黑色眼睛'或'蓝色眼睛'\n"
        "- **违规检测**：如果visualPrompt中的外观描述与Bible冲突，AI模型会生成不一致的角色形象\n"
        "- **解决方案**：生成visualPrompt前，先检查characters数组中的appearance字段，确保所有视觉描述都基于Bible数据\n\n"

        "🎬 **镜头类型（shotType）限制**：\n\n"
        "**只允许使用以下镜头类型（必须严格遵守）**：\n"
        "- 特写 (close-up / extreme close-up)\n"
        "- 近景 (medium close-up)\n"
        "- 中景 (medium shot)\n"
        "- 全景 (full shot)\n"
        "- 远景 (wide shot / establishing shot)\n"
        "- ❌ 禁止使用：过肩(over-the-shoulder)、主观视角(POV)、跟拍(following)等特殊镜头\n\n"

        "📐 **拍摄角度（cameraAngle）限制**：\n\n"
        "**只允许使用以下拍摄角度（必须严格遵守）**：\n"
        "- 平视 (eye-level / frontal)\n"
        "- 低角度/仰视 (low-angle / worm's eye view)\n"
        "- 高角度/俯视 (high-angle / bird's eye view)\n"
        "- 斜侧面 (three-quarter view)\n"
        "- ❌ 禁止使用：过肩、Dutch angle、鱼眼等特殊角度\n\n"

        "**如果需要表达特殊的叙事效果，请在composition字段中说明，不要修改shotType或cameraAngle**\n"
    );
    const QString prompt = SchemaToPrompt::buildSystemPrompt(schema, options);

    LOG_DEBUG("QwenPromptBuilder", QString("System prompt length: %1 chars, example keys: %2")
        .arg(prompt.length())
        .arg(options.customExample.keys().join(", ")));

    return prompt;
}
}

QString QwenPromptBuilder::buildSystemPrompt(const QString& schemaPath)
{
    return buildSystemPromptFromSchema(schemaPath);
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

QString QwenPromptBuilder::buildChangeRequestSystemPrompt()
{
    return ChangeRequestParseUtils::buildChangeRequestSystemPrompt();
}

QString QwenPromptBuilder::buildChangeRequestPrompt()
{
    return buildChangeRequestSystemPrompt();
}

QString QwenPromptBuilder::buildDialogueRewritePrompt()
{
    return joinPromptLines({
        QStringLiteral("你是对白改写助手。"),
        QString(),
        QStringLiteral("请根据用户的修改要求，重写原对白。"),
        QStringLiteral("要求："),
        QStringLiteral("1. 保持原意和人物关系不变。"),
        QStringLiteral("2. 语言自然，符合漫画对白风格。"),
        QStringLiteral("3. 不要输出分析过程。"),
        QStringLiteral("4. 只返回改写后的对白文本。")
    });
}
