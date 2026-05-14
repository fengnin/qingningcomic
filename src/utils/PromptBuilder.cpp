#include "utils/PromptBuilder.h"
#include "services/CharacterExtractor.h"
#include "utils/ChangeRequestIntentUtils.h"
#include "utils/DialogueSpeakerSideUtils.h"
#include "utils/Logger.h"
#include "utils/PromptTargetUtils.h"
#include "utils/SceneKeyUtils.h"
#include <QRegularExpression>

namespace {
    const QString JAPANESE_MANGA_DIRECTIVE =
        QString::fromUtf8("Japanese manga style, anime aesthetics, clean screentone line art, "
                          "high contrast, expressive faces, vibrant color palette");

    // ============================================================
    // Negative Prompt 工厂：统一管理所有类型的否定词，消除重复
    // ============================================================
    namespace NegativePromptFactory {

        // 所有类型共用的基础否定词（只定义一次）
        inline QString sharedBase() {
            return QString::fromUtf8(
                "nsfw, blurry, low quality, extra limbs, deformed hands, "
                "text watermark, logo, cropped face, overexposed, underexposed"
            );
        }

        // 通用风格禁止（防止AI生成错误的艺术风格）
        inline QString styleRestrictions() {
            return QStringLiteral(
                "photorealistic, realistic, photo, photograph, real person, "
                "realistic skin texture, real human, 3d render, cgi"
            );
        }

        // 背景纯净性要求（角色立绘+面板共用）
        inline QString cleanBackground() {
            return QStringLiteral(
                "dark background, black background, black backdrop, "
                "split background, half black half white, gradient background, "
                "gray background, grey background, gray backdrop, grey backdrop, "
                "gray shadow, grey shadow, background shading, soft shadow, "
                "shadow on background, texture on background, pattern on background, "
                "black border, frame"
            );
        }

        // 角色立绘专用：白底、无阴影、无背景干扰
        inline QString forCharacter() {
            return QStringLiteral(", monochrome, grayscale, black and white, ") +
                sharedBase() + ", " +
                styleRestrictions() + ", " +
                cleanBackground() + ", " +
                QStringLiteral(
                    "silhouette, drop shadow, cast shadow, ambient occlusion, "
                    "face in background, multiple people, extra person, duplicate person, "
                    "background character, background figure"
                );
        }

        // 场景图专用：无人物（纯环境参考图）
        inline QString forScene() {
            return QStringLiteral(", monochrome, grayscale, black and white, ") +
                sharedBase() + ", " +
                QStringLiteral(
                    "person, people, human, character, figure, face, body, silhouette, "
                    "portrait, anime character, manga character, girl, boy, man, woman, "
                    "standing figure, sitting figure, walking figure, "
                    "人物, 人, 角色, 女孩, 男孩, 女性, 男性, 人影"
                );
        }

        // 面板专用：无多格布局、无色块分割
        inline QString forPanel() {
            return QStringLiteral(", monochrome, grayscale, black and white, ") +
                sharedBase() + ", " +
                styleRestrictions() + ", " +
                cleanBackground() + ", " +
                QStringLiteral(
                    "double image, split image, duplicated, distorted face, "
                    "duplicate person, extra person not in description, "
                    "multiple instances of same character, clone of character, "
                    "two women when only one described, two men when only one described, "
                    "comic page, multi-panel layout, split frame, shadow on background"
                );
        }
    }

    constexpr int MAX_PROMPT_LENGTH = 1200;
    
    inline QString truncateText(const QString& text, int maxLen)
    {
        if (text.length() <= maxLen) return text;
        return text.left(maxLen) + "...";
    }
    
    using MappingInit = std::initializer_list<std::pair<QString, QString>>;
    
    QMap<QString, QString> buildMapping(MappingInit items) {
        return QMap<QString, QString>(items);
    }
    
    const QMap<QString, QString> VIEW_MAPPING = buildMapping({
        {"front", "front view, facing forward, centered composition"},
        {"three-quarter", "three-quarter view, slightly angled, dynamic perspective"},
        {"side", "side profile view, 90-degree angle, clear silhouette"},
        {"45-degree", "45-degree angle view, diagonal composition"},
        {"back", "back view, rear angle, looking away"},
        {"top-down", "top-down view, birds eye perspective"},
        {"low-angle", "low-angle view, looking up, dramatic perspective"}
    });
    
    const QMap<QString, QString> POSE_MAPPING = buildMapping({
        {"standing", "standing pose, upright posture, natural stance"},
        {"sitting", "sitting pose, relaxed position"},
        {"walking", "walking pose, mid-stride, dynamic movement"},
        {"running", "running pose, fast movement, action lines"},
        {"action", "dynamic action pose, intense movement"},
        {"fighting", "fighting pose, combat stance, aggressive posture"},
        {"defensive", "defensive pose, guarding position"},
        {"casual", "casual pose, relaxed body language"},
        {"formal", "formal pose, dignified posture"},
        {"dramatic", "dramatic pose with motion blur, cinematic composition"}
    });
    
    const QMap<QString, QString> STYLE_MAPPING = buildMapping({
        {"anime", "anime style, cel-shaded, vibrant colors, expressive features"},
        {"realistic", "photorealistic style, detailed rendering, natural lighting"},
        {"chibi", "chibi style, super deformed, cute proportions, simplified features"},
        {"comic", "comic book style, bold outlines, halftone shading, dynamic composition"},
        {"manga", "manga style, screentone shading, clean line art, high contrast"},
        {"oil-painting", "oil painting style, artistic brushstrokes, textured canvas feel"},
        {"watercolor", "watercolor style, soft edges, pastel colors, artistic wash effects"},
        {"sketch", "sketch style, rough lines, pencil texture, unfinished aesthetic"},
        {"noir", "noir style, high contrast black and white, dramatic shadows"},
        {"retro", "retro anime style, 80s-90s aesthetic, classic cel animation look"}
    });

    const QMap<QString, QString> SHOT_TYPE_MAPPING = buildMapping({
        {"close-up", "close-up shot"},
        {"extreme-close-up", "extreme close-up shot"},
        {"medium", "medium shot"},
        {"medium-close-up", "medium close-up shot"},
        {"full", "full shot"},
        {"wide", "wide shot"},
        {"extreme-wide", "extreme wide shot"},
        {"establishing", "establishing shot"},
        {"over-the-shoulder", "over-the-shoulder shot"},
        {"point-of-view", "POV shot"},
        {QString::fromUtf8("\u7279\u5199"), "close-up shot"},
        {QString::fromUtf8("\u8fd1\u666f"), "medium close-up shot"},
        {QString::fromUtf8("\u4e2d\u666f"), "medium shot"},
        {QString::fromUtf8("\u5168\u666f"), "full shot"},
        {QString::fromUtf8("\u8fdc\u666f"), "wide shot"},
        {QString::fromUtf8("\u5927\u8fdc\u666f"), "extreme wide shot"}
    });

    const QMap<QString, QString> CAMERA_ANGLE_MAPPING = buildMapping({
        {"eye-level", "eye-level angle"},
        {"low-angle", "low angle"},
        {"high-angle", "high angle"},
        {"bird's-eye", "bird's eye view"},
        {"worm's-eye", "worm's eye view"},
        {"dutch", "dutch angle"},
        {QString::fromUtf8("\u5e73\u89c6"), "eye-level angle"},
        {QString::fromUtf8("\u4f4e\u89d2"), "low angle"},
        {QString::fromUtf8("\u4ef0\u89c6"), "high angle"},
        {QString::fromUtf8("\u4fef\u89c6"), "bird's eye view"},
        {QString::fromUtf8("\u87f1\u89c6"), "worm's eye view"},
        {QString::fromUtf8("\u6b63\u9762"), "frontal view"},
        {QString::fromUtf8("\u659c\u9762"), "three-quarter view"}
    });

    bool isJsonValueEmpty(const QJsonValue& value)
    {
        if (value.isNull() || value.isUndefined()) return true;
        if (value.isString()) return value.toString().trimmed().isEmpty();
        return false;
    }

    QStringList parseClothingList(const QJsonValue& clothingValue)
    {
        QStringList clothingList;
        
        if (clothingValue.isArray()) {
            for (const auto& item : clothingValue.toArray()) {
                if (!item.toString().isEmpty()) {
                    clothingList << item.toString();
                }
            }
        } else if (clothingValue.isString()) {
            clothingList = clothingValue.toString().split(QStringLiteral("，"));
            clothingList.removeAll(QStringLiteral(""));
        }
        
        return clothingList;
    }


    static const QStringList HAIR_COLOR_CONFLICT_INDICATORS = {
        QStringLiteral("黑褐色"),
        QStringLiteral("棕色"),
        QStringLiteral("黑色头发"),
        QStringLiteral("brown hair"),
        QStringLiteral("black hair"),
        QStringLiteral("dark hair")
    };

    QString normalizeCharacterNameForRefs(const QString& name)
    {
        if (name.isEmpty()) {
            return name;
        }

        QString normalized = name.trimmed();
        static const QStringList suffixes = {
            QString::fromUtf8("\u541b"),
            QString::fromUtf8("\u5c0f\u59d0"),
            QString::fromUtf8("\u5148\u751f"),
            QString::fromUtf8("\u540c\u5b66"),
            QString::fromUtf8("\u540c\u4e8b"),
            QString::fromUtf8("\u8001\u5e08"),
            QString::fromUtf8("\u4fca"),
            QString::fromUtf8("\u5973\u58eb"),
            QString::fromUtf8("\u516c\u4e3b"),
            "(true form)",
            "(clone)",
            "(past)",
            "(future)",
            "(young)",
            "(old)"
        };

        for (const QString& suffix : suffixes) {
            if (normalized.endsWith(suffix, Qt::CaseInsensitive)) {
                normalized.chop(suffix.length());
                normalized = normalized.trimmed();
                break;
            }
        }

        return normalized;
    }

    void addIfNotEmpty(QStringList& parts, const QString& value);
    void addIfContains(QStringList& parts, const QJsonObject& obj, const QString& key, const QString& format = QString());
    QString optionOrDefault(const QJsonObject& options, const QString& key, const QString& fallback);
    QString buildEditDirective(const QString& editPrompt, const QString& target, const QString& editIntent);
    void appendCharacterFramingParts(QStringList& parts);
    void appendCharacterBackgroundParts(QStringList& parts);

    QString buildPanelCharacterDescription(const QString& name,
                                           const QString& pose,
                                           const QString& expression,
                                           const QJsonObject& appearance)
    {
        QString description = PromptBuilder::formatCharacterDescriptor(name, pose, expression);
        const QString appearanceDescription = PromptBuilder::buildCharacterAppearance(appearance);
        if (!description.isEmpty() && !appearanceDescription.isEmpty()) {
            description += ", " + appearanceDescription;
        } else if (description.isEmpty()) {
            description = appearanceDescription;
        }
        return description;
    }

    PromptBuilder::PanelCharacterPromptData collectPanelCharacterPromptData(const QJsonArray& characters,
                                                              const QMap<QString, QJsonObject>& characterRefs,
                                                              const QStringList& dialogueSpeakers)
    {
        PromptBuilder::PanelCharacterPromptData data;

        for (const auto& charVal : characters) {
            const QJsonObject charObj = charVal.toObject();
            const QString charName = charObj["name"].toString();
            data.panelCharNames.append(charName);

            QString matchedName = charName;
            if (!characterRefs.contains(charName)) {
                const QString normalized = normalizeCharacterNameForRefs(charName);
                if (characterRefs.contains(normalized)) {
                    matchedName = normalized;
                }
            }

            const QString pose = charObj["pose"].toString();
            const QString expression = charObj["expression"].toString();

            if (!characterRefs.contains(matchedName)) {
                const QString charDesc = buildPanelCharacterDescription(charName, pose, expression, QJsonObject());
                if (!charDesc.isEmpty()) {
                    data.characterDescriptions.append(charDesc);
                }
                continue;
            }

            const QJsonObject fullChar = characterRefs.value(matchedName);
            const QJsonObject appearance = fullChar["appearance"].toObject();
            const QStringList portraitPaths = PromptBuilder::normalizeList(fullChar["portraitPaths"].toArray());

            const QString charDesc = buildPanelCharacterDescription(charName, pose, expression, appearance);
            if (!charDesc.isEmpty()) {
                data.characterDescriptions.append(charDesc);
            }

            const QString role = fullChar["role"].toString().trimmed();
            const bool isDialogSpeaker = dialogueSpeakers.contains(charName) || dialogueSpeakers.contains(matchedName);
            const bool isCoreRole = role.contains(QStringLiteral("protagonist"), Qt::CaseInsensitive)
                || role.contains(QStringLiteral("main"), Qt::CaseInsensitive)
                || role.contains(QStringLiteral("lead"), Qt::CaseInsensitive)
                || role.contains(QStringLiteral("hero"), Qt::CaseInsensitive)
                || role.contains(QString::fromUtf8("\u4e3b\u89d2"), Qt::CaseInsensitive);

            const QString portraitPath = PromptBuilder::firstNonEmptyPortrait(portraitPaths);
            if (portraitPath.isEmpty()) {
                continue;
            }

            int priority = 2;
            if (isDialogSpeaker) {
                priority = 0;
                data.primaryCharacterRef = portraitPath;
            } else if (isCoreRole && data.primaryCharacterRef.isEmpty()) {
                priority = 1;
                data.primaryCharacterRef = portraitPath;
            }
            
            if (!data.allCharacterRefs.contains(portraitPath)) {
                if (priority == 0) {
                    data.allCharacterRefs.prepend(portraitPath);
                } else {
                    data.allCharacterRefs.append(portraitPath);
                }
            }
        }

        if (data.primaryCharacterRef.isEmpty() && !data.allCharacterRefs.isEmpty()) {
            data.primaryCharacterRef = data.allCharacterRefs.first();
        }

        return data;
    }

    QString selectPanelReference(const QString& promptTarget,
                                 const QString& editIntent,
                                 const QString& editPrompt,
                                 const QString& primaryCharacterRef,
                                 const QString& primarySceneRef,
                                 QString& selectedRefType)
    {
        const QString normalizedPromptTarget = promptTarget.trimmed().toLower();

        if (normalizedPromptTarget == QStringLiteral("scene")) {
            if (!primarySceneRef.isEmpty()) {
                selectedRefType = QStringLiteral("scene");
                return primarySceneRef;
            }
            if (!primaryCharacterRef.isEmpty()) {
                selectedRefType = QStringLiteral("character");
                return primaryCharacterRef;
            }
            selectedRefType.clear();
            return QString();
        }

        if (normalizedPromptTarget == QStringLiteral("character")) {
            if (!primaryCharacterRef.isEmpty()) {
                selectedRefType = QStringLiteral("character");
                return primaryCharacterRef;
            }
            if (!primarySceneRef.isEmpty()) {
                selectedRefType = QStringLiteral("scene");
                return primarySceneRef;
            }
            selectedRefType.clear();
            return QString();
        }

        const bool preferSceneReference = PromptTargetUtils::shouldPreferSceneReferenceForEditIntent(
            editIntent,
            editPrompt);

        if (preferSceneReference && !primarySceneRef.isEmpty()) {
            selectedRefType = QStringLiteral("scene");
            return primarySceneRef;
        }

        if (!primaryCharacterRef.isEmpty()) {
            selectedRefType = QStringLiteral("character");
            return primaryCharacterRef;
        }

        if (!primarySceneRef.isEmpty()) {
            selectedRefType = QStringLiteral("scene");
            return primarySceneRef;
        }

        selectedRefType.clear();
        return QString();
    }

    QStringList buildPanelReferenceUris(const QString& selectedRefType,
                                        const QString& selectedRef,
                                        const QString& primaryCharacterRef,
                                        const QString& primarySceneRef,
                                        const QStringList& allCharacterRefs)
    {
        Q_UNUSED(selectedRef);
        QStringList refUris;
        QStringList orderedRefs;
        
        if (selectedRefType == QStringLiteral("scene")) {
            orderedRefs << primarySceneRef << allCharacterRefs;
        } else if (selectedRefType == QStringLiteral("character")) {
            orderedRefs << allCharacterRefs << primarySceneRef;
        } else {
            orderedRefs << primarySceneRef << allCharacterRefs;
        }
        
        LOG_INFO("PromptBuilder", QString("构建参考图列表: selectedRefType=%1, primaryCharacterRef=%2, primarySceneRef=%3, allCharacterRefs=%4")
            .arg(selectedRefType.isEmpty() ? "(无)" : selectedRefType)
            .arg(primaryCharacterRef.isEmpty() ? "(无)" : primaryCharacterRef)
            .arg(primarySceneRef.isEmpty() ? "(无)" : primarySceneRef)
            .arg(allCharacterRefs.size()));
        
        int refIndex = 0;
        for (const QString& ref : orderedRefs) {
            if (!ref.isEmpty() && !refUris.contains(ref)) {
                refUris.append(ref);
                refIndex++;
                LOG_INFO("PromptBuilder", QString("  参考图[%1]: %2").arg(refIndex).arg(ref));
            }
        }
        
        LOG_INFO("PromptBuilder", QString("最终参考图数量: %1").arg(refUris.size()));
        return refUris;
    }

    bool sceneKeySetsMatch(const QStringList& requestedKeys, const QStringList& sceneKeys)
    {
        for (const QString& requested : requestedKeys) {
            const QString requestedNorm = requested.trimmed();
            if (requestedNorm.isEmpty()) {
                continue;
            }
            for (const QString& sceneKey : sceneKeys) {
                const QString sceneNorm = sceneKey.trimmed();
                if (sceneNorm.isEmpty()) {
                    continue;
                }
                if (requestedNorm == sceneNorm) {
                    return true;
                }
                if (requestedNorm.length() >= 2 && sceneNorm.contains(requestedNorm)) {
                    return true;
                }
                if (sceneNorm.length() >= 2 && requestedNorm.contains(sceneNorm)) {
                    return true;
                }
            }
        }
        return false;
    }

    // 根据 speakerSide 字段或角色顺序推断说话者位置标记
    // 根据角色外观和位置生成完整气泡描述
    // 例: "a speech bubble with 'xxx' inside, above-left, tail pointing toward the young woman on the left"
    QString buildBubbleDesc(const QString& text,
                             const QString& speaker,
                             const QString& speakerSide,
                             const QMap<QString, QJsonObject>& characterRefs)
    {
        QString matchedName = speaker;
        if (!characterRefs.contains(speaker)) {
            const QString normalized = normalizeCharacterNameForRefs(speaker);
            if (characterRefs.contains(normalized))
                matchedName = normalized;
        }

        QString ageDesc;
        QString genderDesc;
        if (characterRefs.contains(matchedName)) {
            const QJsonObject appearance = characterRefs.value(matchedName)["appearance"].toObject();
            const int age = appearance["age"].toInt(-1);
            const QString gender = appearance["gender"].toString().trimmed();

            if (age >= 55)       ageDesc = "elderly";
            else if (age >= 40)  ageDesc = "middle-aged";
            else if (age >= 0)   ageDesc = "young";

            if (gender == QStringLiteral("女") || gender.toLower() == "female")
                genderDesc = "woman";
            else if (gender == QStringLiteral("男") || gender.toLower() == "male")
                genderDesc = "man";
        }

        const QString person = (!ageDesc.isEmpty() && !genderDesc.isEmpty())
            ? QString("%1 %2").arg(ageDesc, genderDesc)
            : (!genderDesc.isEmpty() ? genderDesc : "person");

        if (speakerSide == QStringLiteral("left")) {
            return QString("a speech bubble with \"%1\" written inside it, "
                           "positioned above-left, tail pointing toward the character on the left")
                   .arg(text);
        }
        if (speakerSide == QStringLiteral("right")) {
            return QString("a speech bubble with \"%1\" written inside it, "
                           "positioned above-right, tail pointing toward the character on the right")
                   .arg(text);
        }
        return QString("a speech bubble with \"%1\" written inside it, "
                       "tail pointing toward the %2")
               .arg(text, person);
    }

    QString formatDialogueForPrompt(const QJsonArray& dialogue,
                                     const QMap<QString, QJsonObject>& characterRefs = QMap<QString, QJsonObject>())
    {
        if (dialogue.isEmpty()) {
            LOG_WARNING("PromptBuilder", "formatDialogueForPrompt: dialogue array is EMPTY!");
            return QString();
        }

        LOG_INFO("PromptBuilder", QString("formatDialogueForPrompt: dialogue count=%1").arg(dialogue.size()));

        QStringList entries;
        for (int i = 0; i < dialogue.size() && i < 2; ++i) {
            const QJsonObject line = dialogue.at(i).toObject();
            const QString speaker     = line["speaker"].toString().trimmed();
            const QString speakerSide = line["speakerSide"].toString().trimmed();
            QString text = line["text"].toString().trimmed();

            if (text.isEmpty()) {
                LOG_WARNING("PromptBuilder", QString("  → Skipping dialogue[%1]: text is empty").arg(i));
                continue;
            }

            // 去掉书名号《》，避免模型把文字渲染到书封上
            text.remove(QRegularExpression(QStringLiteral("《[^》]*》")));
            text = text.trimmed();
            if (text.isEmpty()) continue;

            const QString bubbleDesc = buildBubbleDesc(text, speaker, speakerSide, characterRefs);
            entries.append(bubbleDesc);

            LOG_INFO("PromptBuilder", QString("  → dialogue[%1]: speaker=%2, side=%3, bubble='%4'")
                .arg(i).arg(speaker)
                .arg(speakerSide.isEmpty() ? "(none)" : speakerSide)
                .arg(bubbleDesc));
        }

        if (entries.isEmpty()) {
            LOG_WARNING("PromptBuilder", "formatDialogueForPrompt: all entries skipped, returning empty");
            return QString();
        }

        const QString result = entries.join(", ");
        LOG_INFO("PromptBuilder", QString("formatDialogueForPrompt: result='%1' (len=%2)")
            .arg(result).arg(result.length()));
        return result;
    }

    // ========== 图生图 Prompt 六层组装架构 v2.0 ==========
    // 基于 awesome-gpt-image-2 社区最佳实践 + 小红书GPT-Image-2指南
    // 核心理念: 外观注入场景描述 = 自然连贯 + 无冲突

    QString truncateSmart(const QString& text, int maxLen)
    {
        if (text.length() <= maxLen) return text;

        const QList<QString> delimiters = {"，", ",", "。", ".", "；", ";"};
        for (const auto& delim : delimiters) {
            const int lastDelim = text.lastIndexOf(delim, maxLen - 2);
            if (lastDelim > static_cast<int>(maxLen * 0.6)) {
                return text.left(lastDelim + 1);
            }
        }

        return text.left(maxLen - 2);
    }

    QStringList detectVisualPromptConflicts(const QString& visualPrompt,
                                           const QMap<QString, QJsonObject>& characterRefs)
    {
        QStringList conflicts;

        if (visualPrompt.isEmpty() || characterRefs.isEmpty()) {
            return conflicts;
        }

        LOG_INFO("PromptBuilder", QString(
            "🔍 开始visualPrompt与Bible冲突检测 (visualPrompt长度: %1字符, 角色数: %2)")
            .arg(visualPrompt.length()).arg(characterRefs.size()));
        
        LOG_DEBUG("PromptBuilder", QString(
            "visualPrompt内容: %1...")
            .arg(visualPrompt.left(150)));

        for (auto it = characterRefs.begin(); it != characterRefs.end(); ++it) {
            const QString& charName = it.key();
            const QJsonObject& charData = it.value();
            const QJsonObject appearance = charData["appearance"].toObject();

            if (!visualPrompt.contains(charName)) {
                continue;
            }

            const QString hairColor = appearance["hairColor"].toString().trimmed();
            if (!hairColor.isEmpty()) {
                for (const auto& indicator : HAIR_COLOR_CONFLICT_INDICATORS) {
                    if (visualPrompt.contains(indicator, Qt::CaseInsensitive)) {
                        conflicts << QString(QStringLiteral(
                            "⚠️ 角色'%1'发色冲突：Bible指定'%2'，但visualPrompt包含'%3'"))
                            .arg(charName, hairColor, indicator);
                        break;
                    }
                }
            }

            const QJsonValue clothingValue = appearance["clothing"];
            if (!isJsonValueEmpty(clothingValue)) {
                const QStringList bibleClothingList = parseClothingList(clothingValue);

                for (const auto& bibleClothing : bibleClothingList) {
                    if (!visualPrompt.contains(bibleClothing, Qt::CaseInsensitive)) {
                        conflicts << QString(QStringLiteral(
                            "⚠️ 角色'%1'衣服缺失：Bible指定'%2'，但visualPrompt未包含"))
                            .arg(charName, bibleClothing);
                    }
                }
            }
        }

        if (!conflicts.isEmpty()) {
            LOG_WARNING("PromptBuilder", QString("visualPrompt与Bible冲突检测 (%1项):")
                .arg(conflicts.size()));
            for (const auto& conflict : conflicts) {
                LOG_WARNING("PromptBuilder", conflict);
            }
        }

        return conflicts;
    }

    // 发色+发型合并，避免"白色、稀疏短发"语义断裂，也避免"白发齐肩直发"重复"发"字
    // hairColor="白发" + hairStyle="齐肩直发" → "白色齐肩直发"（去掉 hairColor 末尾的"发"）
    QString mergeHairCn(const QJsonObject& appearance)
    {
        const QString color = appearance["hairColor"].toString().trimmed();
        const QString style = appearance["hairStyle"].toString().trimmed();
        if (color.isEmpty()) return style;
        if (style.isEmpty()) return color;
        // hairColor 以"发"结尾时（如"白发"、"黑发"），去掉末尾"发"再拼 hairStyle
        if (color.endsWith(QStringLiteral("发"))) {
            return color.left(color.length() - 1) + style;
        }
        return color + style;
    }

    // 服装列表智能合并：将内外搭服装合并为单句描述
    // 防止AI把同一角色的多件衣服理解为多个独立角色（如"白衬衫"+"蓝围裙"→生成两个女孩）
    // 输入: ["浅蓝色棉麻围裙", "白色短袖衬衫", "及膝牛仔裙"]
    // 输出: "内搭白色短袖衬衫、外穿浅蓝色棉麻围裙及膝牛仔裙"
    QString mergeClothingForSingleEntity(const QStringList& clothingList)
    {
        if (clothingList.isEmpty()) return QString();
        if (clothingList.size() == 1) return clothingList.first();

        static const QRegularExpression innerWearPattern(
            QStringLiteral("(T恤|短袖|长袖|衬衫|内衣|打底|背心)"),
            QRegularExpression::CaseInsensitiveOption);
        static const QRegularExpression outerWearPattern(
            QStringLiteral("(围裙|外套|大衣|风衣|夹克|马甲|背心裙|连衣裙|牛仔)"),
            QRegularExpression::CaseInsensitiveOption);

        QStringList innerItems;
        QStringList outerItems;
        QStringList otherItems;

        for (const auto& item : clothingList) {
            if (innerWearPattern.match(item).hasMatch()) {
                innerItems << item;
            } else if (outerWearPattern.match(item).hasMatch()) {
                outerItems << item;
            } else {
                otherItems << item;
            }
        }

        QStringList merged;
        if (!innerItems.isEmpty() && !outerItems.isEmpty()) {
            merged << QString("内搭%1").arg(innerItems.join(QStringLiteral("、")));
            merged << QString("外穿%1").arg(outerItems.join(QStringLiteral("")));
        } else if (!innerItems.isEmpty()) {
            merged << innerItems.join(QStringLiteral("、"));
        } else if (!outerItems.isEmpty()) {
            merged << outerItems.join(QStringLiteral(""));
        }

        if (!otherItems.isEmpty()) {
            if (!merged.isEmpty()) {
                merged.last() += otherItems.join(QStringLiteral(""));
            } else {
                merged << otherItems.join(QStringLiteral("、"));
            }
        }

        return merged.join(QStringLiteral("、"));
    }

    // 构建单个角色的外观括注，格式：【发色发型、服装】
    QString buildCharAppearanceNote(const QString& charName,
                                     const QMap<QString, QJsonObject>& characterRefs,
                                     bool skipClothing)
    {
        QString matchedName = charName;
        if (!characterRefs.contains(charName)) {
            const QString normalized = normalizeCharacterNameForRefs(charName);
            if (characterRefs.contains(normalized))
                matchedName = normalized;
        }
        if (!characterRefs.contains(matchedName)) return QString();

        const QJsonObject appearance = characterRefs.value(matchedName)["appearance"].toObject();
        QStringList parts;

        const QString hair = mergeHairCn(appearance);
        if (!hair.isEmpty()) parts << hair;

        const QString eyeColor = appearance["eyeColor"].toString().trimmed();
        if (!eyeColor.isEmpty()) parts << eyeColor + "眼";

        if (!skipClothing) {
            const QStringList clothing = parseClothingList(appearance["clothing"]);
            if (!clothing.isEmpty()) parts << mergeClothingForSingleEntity(clothing);
        }

        if (parts.isEmpty()) return QString();
        return QString(QStringLiteral("【%1】")).arg(parts.join(QStringLiteral("、")));
    }

    // 将 visualPromptCn 中的角色名替换为 角色名【外观括注】，使外观与场景融为一体
    QString injectBibleIntoScene(const QString& visualPromptCn,
                                  const PromptBuilder::PanelCharacterPromptData& characterData,
                                  const QMap<QString, QJsonObject>& characterRefs,
                                  bool skipClothing)
    {
        static const QRegularExpression RE_BOOK_TITLE(QStringLiteral("《([^》]*)》"));
        static const QRegularExpression RE_SINGLE_QUOTE(QStringLiteral("'([^']*)'"));

        QString result = visualPromptCn;
        // 去掉书名号/单引号括号，保留内容，避免模型把引号内文字渲染成图像文字
        result.replace(RE_BOOK_TITLE,    QStringLiteral("\\1"));
        result.replace(RE_SINGLE_QUOTE,  QStringLiteral("\\1"));

        for (const auto& charName : characterData.panelCharNames) {
            const QString note = buildCharAppearanceNote(charName, characterRefs, skipClothing);
            if (note.isEmpty()) continue;

            // 替换所有出现的角色名，每次都注入外观括注
            // 防止模型把角色名误解为同音词（如"青柠"→水果）
            bool injected = false;
            int searchFrom = 0;
            while (true) {
                const int pos = result.indexOf(charName, searchFrom);
                if (pos < 0) break;
                const int after = pos + charName.length();
                if (after < result.length() && result.at(after) == QChar(0x3010)) {
                    // 已有【，跳过
                    searchFrom = after + 1;
                    continue;
                }
                if (!injected) {
                    result.replace(pos, charName.length(), charName + note);
                    searchFrom = pos + charName.length() + note.length();
                    injected = true;
                    LOG_INFO("PromptBuilder", QString("  注入外观: %1%2").arg(charName, note));
                } else {
                    // 后续出现只保留名字，不重复外观——同一 prompt 内模型已建立关联
                    searchFrom = pos + charName.length();
                }
            }
        }
        return result;
    }

    bool shouldSkipClothing(const QString& editIntent)
    {
        return editIntent.contains(QStringLiteral("replace_attribute"))
            || editIntent.contains(QStringLiteral("replace_subject"));
    }

    QStringList buildOptimizedImg2ImgParts(const QJsonObject& panel,
                                           const PromptBuilder::PanelCharacterPromptData& characterData,
                                           const QMap<QString, QJsonObject>& characterRefs,
                                           const QMap<QString, QJsonObject>& sceneRefs,
                                           const QString& sceneId,
                                           const QString& sceneName)
    {
        QStringList parts;
        static const int TOTAL_BUDGET = 400;

        LOG_INFO("PromptBuilder", QString("Building 2-layer prompt (budget: %1 chars)").arg(TOTAL_BUDGET));

        const QString editIntent = panel.value("editIntent").toString().trimmed();
        const bool skipClothing = shouldSkipClothing(editIntent);

        // 编辑模式 vs 批量生成: 两种完全不同的 prompt 策略
        //
        // [批量生成] 从零画新图 → 需要完整描述（用途+场景+外观+气泡）
        // [局部编辑] 在已有图上改 → 原图已包含所有上下文，只需编辑指令(+可选气泡)
        if (!editIntent.isEmpty()) {
            // 编辑模式: 直接使用原始编辑指令，不做 buildEditDirective 包装
            // 原因: ① 包装前缀(~100字)比原始指令还长，全是废话(构图/光照原图里都有)
            //       ② 原图通过参考图传入，AI 已知道画面全貌，只需说改什么
            //       ③ 越短的 prompt 在局部编辑中效果越好，减少干扰
            
            const QString editPrompt = panel["visualPrompt"].toString().trimmed();
            if (!editPrompt.isEmpty()) {
                parts << editPrompt;
            }
            
            // 编辑模式下的气泡：仅当对话内容与编辑相关时才保留
            // 大多数局部编辑不涉及对话修改，加气泡反而浪费预算
            const QJsonArray dialogue = panel["dialogue"].toArray();
            if (!dialogue.isEmpty() && editIntent.contains(QStringLiteral("set_expression"))) {
                const QString bubble = formatDialogueForPrompt(dialogue, characterRefs);
                if (!bubble.isEmpty()) {
                    parts << bubble;
                }
            }
        } else {
            // 批量生成模式: 完整的三层结构
            
            // Layer 0: 用途声明 — 告知模型这是漫画面板，帮助理解[气泡]标记
            parts << "[PURPOSE], manga panel, for readers";

            // Layer 1: 场景圣经环境 + visualPromptCn 动作描述 + 角色外观注入
            QString baseScene;
            const QString visualPromptCn = panel["visualPromptCn"].toString().trimmed();
            const QString sceneText = panel["scene"].toString().trimmed();
            baseScene = !visualPromptCn.isEmpty() ? visualPromptCn : sceneText;

            // 场景圣经：building + atmosphere + anchorPoints + consistencyRules 前置拼入
            // 和角色外观注入方式一致——都融入同一段文本
            const QString sceneDesc = [&]() -> QString {
                LOG_DEBUG("PromptBuilder", QString("  场景圣经查找: sceneId='%1', sceneName='%2', sceneRefs.keys=%3")
                    .arg(sceneId, sceneName, QStringList(sceneRefs.keys()).join(",")));
                QJsonObject matched;
                if (!sceneId.isEmpty() && sceneRefs.contains(sceneId)) {
                    matched = sceneRefs.value(sceneId)["details"].toObject();
                } else if (!sceneName.isEmpty() && sceneRefs.contains(sceneName)) {
                    matched = sceneRefs.value(sceneName)["details"].toObject();
                } else {
                    // 精确匹配失败，尝试模糊匹配：找包含关系的 key
                    for (const QString& key : sceneRefs.keys()) {
                        if (key.contains(sceneName) || sceneName.contains(key)) {
                            matched = sceneRefs.value(key)["details"].toObject();
                            break;
                        }
                    }
                }
                if (matched.isEmpty()) return QString();
                QStringList p;
                const QString building = matched["building"].toString().trimmed();
                const QString atmosphere = matched["atmosphere"].toString().trimmed();
                if (!building.isEmpty()) p << building;
                if (!atmosphere.isEmpty()) p << atmosphere;
                const QJsonArray anchors = matched["anchorPoints"].toArray();
                for (const auto& a : anchors) {
                    const QString s = a.toString().trimmed();
                    if (!s.isEmpty()) p << s;
                }
                const QJsonArray rules = matched["consistencyRules"].toArray();
                for (const auto& r : rules) {
                    const QString s = r.toString().trimmed();
                    if (!s.isEmpty()) p << s;
                }
                return p.join("，");
            }();
            if (!sceneDesc.isEmpty() && !baseScene.isEmpty()) {
                const QString scenePrefix = truncateSmart(sceneDesc, 60);
                baseScene = scenePrefix + "，" + baseScene;
                LOG_INFO("PromptBuilder", QString("  注入场景圣经: %1").arg(scenePrefix));
            }

            if (!baseScene.isEmpty()) {
                const QString injected = injectBibleIntoScene(baseScene, characterData, characterRefs, skipClothing);
                parts << truncateSmart(injected, TOTAL_BUDGET - 50);
            }

            // Layer 2: 对话气泡
            const QString dialogueStr = formatDialogueForPrompt(panel["dialogue"].toArray(),
                                                                 characterRefs);
            if (!dialogueStr.isEmpty()) {
                parts << dialogueStr;
            }
        }

        const QString joined = parts.join("，");
        LOG_INFO("PromptBuilder", QString(
            "2-layer complete: panelId=%1, layers=%2, length=%3/%4 (%5%)")
            .arg(panel.value("id").toString())
            .arg(parts.size())
            .arg(joined.length())
            .arg(TOTAL_BUDGET)
            .arg(qRound(static_cast<double>(joined.length()) / TOTAL_BUDGET * 100)));

        return parts;
    }

    void logPanelPromptSelection(const QJsonObject& panel,
                                 const QString& promptTarget,
                                 const QString& selectedRefType,
                                 const QString& selectedRef,
                                 const QString& primarySceneRef,
                                 const QString& primaryCharacterRef,
                                 const QStringList& panelCharNames,
                                 const QStringList& dialogueSpeakers)
    {
        const QString shotType = panel.value("shotType").toString().trimmed();
        const QString cameraAngle = panel.value("cameraAngle").toString().trimmed();
        const QString sceneText = panel.value("scene").toString().trimmed();
        const QString editIntent = panel.value("editIntent").toString().trimmed();
        const QString visualPrompt = panel.value("visualPrompt").toString().trimmed();
        const QString visualPromptEn = panel.value("visualPromptEn").toString().trimmed();
        const QString composition = panel.value("composition").toObject().value("rule").toString().trimmed();
        const QString focusPoint = panel.value("composition").toObject().value("focusPoint").toString().trimmed();
        const QString mood = panel.value("atmosphere").toObject().value("mood").toString().trimmed();
        const QString panelId = panel.value("id").toString();

        LOG_INFO("PromptBuilder", QString("buildPanelPrompt input: panelId=%1, shotType=%2, cameraAngle=%3, editIntent=%4, promptTarget=%5")
            .arg(panelId)
            .arg(shotType.isEmpty() ? "(empty)" : shotType)
            .arg(cameraAngle.isEmpty() ? "(empty)" : cameraAngle)
            .arg(editIntent.isEmpty() ? "(empty)" : editIntent)
            .arg(promptTarget.isEmpty() ? "(empty)" : promptTarget));

        LOG_INFO("PromptBuilder", QString("buildPanelPrompt text: scene=%1, visualPrompt=%2, visualPromptEn=%3, mood=%4, composition=%5, focusPoint=%6")
            .arg(sceneText.isEmpty() ? "(empty)" : sceneText.left(120))
            .arg(visualPrompt.isEmpty() ? "(empty)" : visualPrompt.left(120))
            .arg(visualPromptEn.isEmpty() ? "(empty)" : visualPromptEn.left(120))
            .arg(mood.isEmpty() ? "(empty)" : mood)
            .arg(composition.isEmpty() ? "(empty)" : composition)
            .arg(focusPoint.isEmpty() ? "(empty)" : focusPoint));

        LOG_INFO("PromptBuilder", QString("buildPanelPrompt refs: panelChars=[%1], charRefs=%2, sceneRef=%3, primaryRef=%4")
            .arg(panelCharNames.join(", "))
            .arg(panel.contains("characters") ? panel["characters"].toArray().size() : 0)
            .arg(primarySceneRef.isEmpty() ? "(none)" : primarySceneRef.left(30))
            .arg(primaryCharacterRef.isEmpty() ? "(none)" : primaryCharacterRef.left(30)));

        LOG_INFO("PromptBuilder", QString("buildPanelPrompt selection: promptTarget=%1, selectedRefType=%2, selectedRef=%3, dialogueSpeakers=[%4]")
            .arg(promptTarget.isEmpty() ? "(empty)" : promptTarget)
            .arg(selectedRefType.isEmpty() ? "(none)" : selectedRefType)
            .arg(selectedRef.isEmpty() ? "(none)" : selectedRef.left(30))
            .arg(dialogueSpeakers.join(", ")));
        Q_UNUSED(panel);
    }
    
    const QMap<QString, QString> EXPRESSION_MAPPING = buildMapping({
        {"neutral", "neutral expression, calm face"},
        {"happy", "happy expression, bright smile"},
        {"sad", "sad expression, downcast eyes"},
        {"angry", "angry expression, furrowed brows, intense glare"},
        {"surprised", "surprised expression, wide eyes, open mouth"},
        {"scared", "scared expression, fearful eyes"},
        {"determined", "determined expression, focused gaze, resolute face"},
        {"confused", "confused expression, tilted head, questioning look"},
        {"embarrassed", "embarrassed expression, blushing, averted eyes"},
        {"smirking", "smirking expression, slight grin, confident look"}
    });

    const QMap<QString, QString> TIME_OF_DAY_MAPPING = buildMapping({
        {"dawn", "dawn light, early morning glow, soft pink and orange sky"},
        {"morning", "morning light, bright and fresh atmosphere"},
        {"noon", "noon sunlight, bright overhead lighting, strong shadows"},
        {"afternoon", "afternoon light, warm golden tones, long shadows"},
        {"dusk", "dusk light, twilight, orange and purple sky"},
        {"evening", "evening atmosphere, dimming light, warm tones"},
        {"night", "nighttime, moonlight, dark sky with stars"},
        {"midnight", "midnight, deep darkness, minimal lighting, dramatic shadows"}
    });
    
    const QMap<QString, QString> WEATHER_MAPPING = buildMapping({
        {"clear", "clear sky, bright sunlight, no clouds"},
        {"cloudy", "cloudy sky, overcast, diffused soft light"},
        {"rainy", "rainy weather, wet surfaces, puddles, rain drops"},
        {"snowy", "snowy weather, white blanket, snowflakes falling"},
        {"foggy", "foggy atmosphere, mist, low visibility, ethereal mood"},
        {"stormy", "stormy weather, dark clouds, lightning, dramatic sky"},
        {"windy", "windy conditions, blowing debris, dynamic movement"}
    });
    
    const QMap<QString, QString> SPACE_SIZE_MAPPING = buildMapping({
        {"cramped", "cramped space, tight quarters, claustrophobic"},
        {"small", "small space, intimate setting"},
        {"medium", "medium-sized space, moderate room"},
        {"large", "large space, expansive area, grand scale"},
        {"vast", "vast space, sweeping expanse, monumental scale"}
    });

    void addIfNotEmpty(QStringList& parts, const QString& value) {
        if (!value.isEmpty()) {
            parts.append(value);
        }
    }
    
    void addIfContains(QStringList& parts, const QJsonObject& obj, const QString& key, const QString& format) {
        if (obj.contains(key)) {
            QString value = obj[key].isString() ? obj[key].toString() : QString::number(obj[key].toInt());
            parts.append(format.isEmpty() ? value : QString(format).arg(value));
        }
    }

    QString optionOrDefault(const QJsonObject& options, const QString& key, const QString& fallback)
    {
        const QString value = options.value(key).toString().trimmed();
        return value.isEmpty() ? fallback : value;
    }
    
    void addListIfNotEmpty(QStringList& parts, const QStringList& list, const QString& prefix) {
        if (!list.isEmpty()) {
            parts.append(QString("%1 %2").arg(prefix, list.join(", ")));
        }
    }

    QString combineEditPrompt(const QString& promptCn, const QString& promptEn)
    {
        QStringList prompts;
        if (!promptCn.isEmpty()) {
            prompts.append(promptCn);
        }
        if (!promptEn.isEmpty() && !prompts.contains(promptEn)) {
            prompts.append(promptEn);
        }
        return prompts.join(" / ");
    }

    QString sanitizeSceneText(const QString& text)
    {
        if (text.isEmpty()) {
            return text;
        }

        QString result = text;
        static const QStringList keywords = {
            "wearing", "costume", "actor", "actress", "performer",
            "person", "people", "human", "character", "figure"
        };

        for (const QString& keyword : keywords) {
            QRegularExpression re(QString(R"(\b%1\b)").arg(QRegularExpression::escape(keyword)),
                                  QRegularExpression::CaseInsensitiveOption);
            result.remove(re);
        }

        static const QRegularExpression multiSep(R"([,，;；\s]{2,})");
        result.replace(multiSep, ", ");
        result.replace(QRegularExpression(R"(^[\s,，;；]+)"), "");
        result.replace(QRegularExpression(R"([\s,，;；]+$)"), "");
        return result.trimmed();
    }

    QString buildEditDirective(const QString& editPrompt, const QString& target, const QString& editIntent)
    {
        if (editPrompt.isEmpty()) {
            return QString();
        }

        QString normalizedTarget = target.trimmed().toLower();
        const QString normalizedEditIntent = editIntent.trimmed().toLower();

        if (PromptTargetUtils::isLocalObjectMovementIntent(normalizedEditIntent)) {
            return QString("localized object movement edit only, move the specified object from its original position into the requested container or location, remove the original occurrence, rebuild the vacated area naturally, keep the object identity, scene, framing, lighting, and composition stable, do not duplicate the object or turn this into a character edit, apply this edit strictly: %1")
                .arg(editPrompt);
        }

        if (normalizedEditIntent == QStringLiteral("replace_subject")) {
            if (normalizedTarget == QStringLiteral("character") || PromptTargetUtils::isCharacterFocusedPrompt(editPrompt)) {
                return QString("character replacement edit only, replace the existing subject with the specified identity, keep the scene, framing, pose, lighting, and composition stable, lock the subject identity to the provided character reference, do not redraw the surrounding background, apply this edit strictly: %1")
                    .arg(editPrompt);
            }
            return QString("localized object replacement edit only, replace the specific local object or subject named in the request, keep the surrounding scene, framing, lighting, and composition stable, do not expand the edit to the whole character or whole image, apply this edit strictly: %1")
                .arg(editPrompt);
        }

        if (normalizedEditIntent == QStringLiteral("replace_attribute")) {
            return QString("localized character attribute edit only, change only the requested clothing, hair, accessory, material, texture, or color detail, keep the character identity, pose, expression, scene, framing, lighting, and composition stable, do not replace the whole character, apply this edit strictly: %1")
                .arg(editPrompt);
        }

        if (PromptTargetUtils::isRemovalIntentText(editPrompt)) {
            return QString("scene edit only, remove the specified subject, rebuild the covered background naturally, keep all remaining characters, composition, perspective, lighting, and color continuity stable, apply this edit strictly: %1")
                .arg(editPrompt);
        }

        if (normalizedTarget == "scene") {
            return QString("scene edit only, keep character identity and pose stable, apply this edit strictly: %1")
                .arg(editPrompt);
        }

        if (normalizedTarget == "character") {
            return QString("face-level local edit only, change only the requested facial expression detail, limit changes to eyes, eyebrows, mouth corners, and facial features, do not redraw the whole character or whole image, keep pose, body, clothing, background, camera, composition, and aspect ratio stable, apply this edit strictly: %1")
                .arg(editPrompt);
        }

        return QString("apply this edit strictly while preserving the existing composition: %1")
            .arg(editPrompt);
    }

    void appendCharacterFramingParts(QStringList& parts)
    {
        // ✅ 职责分离：全部使用正面描述（无否定词）
        // 原来使用 "avoid xxx"，现在改为正向引导
        parts << "full body shot, head-to-toe character framing, allow slight crop at edges if needed";
        parts << "character fills most of the frame, subject is large and prominent, compact composition with minimal empty space, keep full body visible";
    }

    void appendCharacterBackgroundParts(QStringList& parts)
    {
        parts << "flat even lighting";
        parts << "pure white background only, isolated character on pure white seamless background, plain white background only, no black area, no dark area, no gradient, no shadow, no backdrop, no studio background, completely empty background, clean white studio cutout";
        parts << "line art with screentone shading, vibrant colors";
    }

    [[maybe_unused]] QString buildAspectRatioDirective(const QJsonObject& options)
    {
        const QString aspectRatio = optionOrDefault(options, "aspectRatio", QString());
        const QString composition = optionOrDefault(options, "composition", QString());

        if (aspectRatio.isEmpty() && composition.isEmpty()) {
            return QString();
        }

        QStringList parts;
        const QString normalizedAspectRatio = aspectRatio.trimmed().toLower();
        if (normalizedAspectRatio == QStringLiteral("1x1") || normalizedAspectRatio == QStringLiteral("1:1")) {
            parts << QStringLiteral("square composition")
                  << QStringLiteral("centered framing")
                  << QStringLiteral("equal width and height");
        } else if (normalizedAspectRatio == QStringLiteral("16:9")
                   || normalizedAspectRatio == QStringLiteral("16x9")) {
            parts << QStringLiteral("widescreen composition");
        } else if (normalizedAspectRatio == QStringLiteral("3:2")
                   || normalizedAspectRatio == QStringLiteral("3x2")) {
            parts << QStringLiteral("landscape composition");
        } else if (!aspectRatio.isEmpty()) {
            parts << QString("strictly maintain %1 aspect ratio").arg(aspectRatio);
        }

        if (!composition.isEmpty()) {
            if (composition.compare(QStringLiteral("square"), Qt::CaseInsensitive) == 0) {
                if (!parts.contains(QStringLiteral("square composition"))) {
                    parts.prepend(QStringLiteral("square composition"));
                }
                parts << QStringLiteral("balanced framing");
            } else {
                parts << QString("%1 composition").arg(composition);
            }
        }

        return parts.join(", ");
    }

    QString truncatePrompt(const QStringList &parts, int maxLength) {
        QString result = parts.join(", ");
        if (result.length() <= maxLength) {
            return result;
        }
        
        QStringList truncated;
        int currentLen = 0;
        for (const QString &part : parts) {
            int addLen = part.length() + 2;
            if (currentLen + addLen > maxLength) {
                break;
            }
            truncated.append(part);
            currentLen += addLen;
        }
        
        QString truncatedResult = truncated.join(", ");
        
        // 如果截断后为空，但原始内容不为空，则强制截取前 maxLength 个字符
        if (truncatedResult.isEmpty() && !result.isEmpty()) {
            return result.left(maxLength);
        }
        
        return truncatedResult;
    }
}

QString PromptBuilder::firstNonEmptyPortrait(const QStringList &portraitPaths)
{
    for (const QString &path : portraitPaths) {
        if (!path.isEmpty()) return path;
    }
    return QString();
}


QStringList PromptBuilder::normalizeList(const QJsonValue &value)
{
    if (value.isNull() || value.isUndefined()) {
        return {};
    }
    if (value.isArray()) {
        QStringList result;
        for (const auto &v : value.toArray()) {
            if (!v.toString().isEmpty()) {
                result.append(v.toString());
            }
        }
        return result;
    }
    QString str = value.toString();
    return str.isEmpty() ? QStringList{} : QStringList{str};
}

QString PromptBuilder::formatHair(const QJsonObject &appearance)
{
    QString hairColor = appearance["hairColor"].toString();
    QString hairStyle = appearance["hairStyle"].toString();

    if (hairColor.isEmpty() && hairStyle.isEmpty()) {
        return QString();
    }
    if (!hairColor.isEmpty() && !hairStyle.isEmpty()) {
        return QString("%1 %2 hair").arg(hairColor, hairStyle);
    }
    return hairColor.isEmpty()
        ? QString("%1 hair").arg(hairStyle)
        : QString("%1 hair").arg(hairColor);
}

QString PromptBuilder::formatCharacterDescriptor(const QString &name,
                                                   const QString &pose,
                                                   const QString &expression)
{
    QStringList segments;
    if (!name.isEmpty()) {
        segments.append(name);
    }
    if (!pose.isEmpty()) {
        segments.append(POSE_MAPPING.value(pose, QString("%1 pose").arg(pose)));
    }
    if (!expression.isEmpty()) {
        segments.append(EXPRESSION_MAPPING.value(expression, QString("%1 expression").arg(expression)));
    }
    return segments.isEmpty() ? QString() : segments.join(", ");
}

QString PromptBuilder::buildCharacterAppearance(const QJsonObject &appearance)
{
    if (appearance.isEmpty()) {
        return QString();
    }

    QStringList parts;

    addIfNotEmpty(parts, appearance["gender"].toString());
    QString age = appearance["age"].toString();
    if (!age.isEmpty() && age != "0") {
        bool ok = false;
        age.toInt(&ok);
        if (ok) {
            parts.append(QString("%1 years old").arg(age));
        } else {
            parts.append(age);
        }
    }
    addIfNotEmpty(parts, appearance["build"].toString());
    addIfNotEmpty(parts, formatHair(appearance));
    addIfContains(parts, appearance, "eyeColor", "%1 eyes");

    QStringList clothing = normalizeList(appearance["clothing"]);
    if (!clothing.isEmpty()) {
        parts.append(QString("wearing %1").arg(clothing.join(", ")));
    }

    QStringList accessories = normalizeList(appearance["accessories"]);
    if (!accessories.isEmpty()) {
        parts.append(QString("accessories: %1").arg(accessories.join(", ")));
    }

    QStringList features = normalizeList(appearance["distinctiveFeatures"]);
    if (!features.isEmpty()) {
        parts.append(QString("distinctive features: %1").arg(features.join(", ")));
    }

    return parts.join(", ");
}

QString PromptBuilder::filterHumanKeywords(const QString &text)
{
    if (text.isEmpty()) return text;
    
    static const QStringList strictKeywords = {
        QString::fromUtf8("\u4eba"), QString::fromUtf8("\u4eba\u7269"), QString::fromUtf8("\u7537\u4eba"),
        QString::fromUtf8("\u5973\u4eba"), QString::fromUtf8("\u7537\u6027"), QString::fromUtf8("\u5973\u6027"),
        QString::fromUtf8("\u5c11\u5973"), QString::fromUtf8("\u5c11\u5e74"), QString::fromUtf8("\u8001\u4eba"),
        QString::fromUtf8("\u5b69\u5b50"), QString::fromUtf8("\u9752\u5e74"), QString::fromUtf8("\u6210\u4eba"),
        QString::fromUtf8("\u6f14\u5458"), QString::fromUtf8("\u660e\u661f"),
        QString::fromUtf8("\u6a21\u7279"), QString::fromUtf8("\u7f8e\u5973"),
        QString::fromUtf8("\u5e25\u54e5"), QString::fromUtf8("\u7f8e\u5973"), QString::fromUtf8("\u5c0f\u9c9c"),
        QString::fromUtf8("\u6b63\u59b9"), QString::fromUtf8("\u7ed7\u58eb"), QString::fromUtf8("wearing"),
        "costume", "actor", "actress", "performer"
    };
    
    static const QStringList contextualKeywords = {
        "person", "people", "human", "character", "figure"
    };
    
    QString result = text;
    
    for (const QString& keyword : strictKeywords) {
        QRegularExpression re(QString("(%1[^,，；;]*)").arg(QRegularExpression::escape(keyword)),
                              QRegularExpression::CaseInsensitiveOption);
        result.remove(re);
    }
    
    for (const QString& keyword : contextualKeywords) {
        QRegularExpression re(QString(R"(\b%1s?\b[^,，；;]{0,20})").arg(QRegularExpression::escape(keyword)),
                              QRegularExpression::CaseInsensitiveOption);
        result.remove(re);
    }
    
    static const QRegularExpression multiSep(R"([,，；;\s]{2,})");
    result.replace(multiSep, ", ");
    result.replace(QRegularExpression(R"(^[\s,，；;]+)"), "");
    result.replace(QRegularExpression(R"([\s,，；;]+$)"), "");
    
    return result.trimmed();
}

QString PromptBuilder::matchSceneDetails(const QMap<QString, QJsonObject> &sceneRefs,
                                           const QString &sceneId,
                                           const QString &sceneName)
{
    QJsonObject matched;
    if (!sceneId.isEmpty() && sceneRefs.contains(sceneId)) {
        matched = sceneRefs.value(sceneId)["details"].toObject();
    } else if (!sceneName.isEmpty() && sceneRefs.contains(sceneName)) {
        matched = sceneRefs.value(sceneName)["details"].toObject();
    }
    
    if (matched.isEmpty()) return QString();
    
    QStringList parts;
    addIfNotEmpty(parts, sanitizeSceneText(matched["description"].toString()));
    addIfNotEmpty(parts, sanitizeSceneText(matched["building"].toString()));
    addIfNotEmpty(parts, sanitizeSceneText(matched["color"].toString()));
    addIfNotEmpty(parts, sanitizeSceneText(matched["atmosphere"].toString()));
    addIfNotEmpty(parts, sanitizeSceneText(matched["landmark"].toString()));
    addIfNotEmpty(parts, sanitizeSceneText(matched["layout"].toString()));
    addIfNotEmpty(parts, matched["timeOfDay"].toString());
    addIfNotEmpty(parts, matched["weather"].toString());
    
    return parts.join(", ");
}

QStringList PromptBuilder::extractDialogueSpeakers(const QJsonArray &dialogue)
{
    QStringList speakers;
    for (const auto& line : dialogue) {
        QString speaker = line.toObject()["speaker"].toString().trimmed();
        if (speaker.isEmpty()) {
            continue;
        }

        const QString normalizedSpeaker = speaker.toLower();
        if (normalizedSpeaker == QStringLiteral("narration")
            || normalizedSpeaker == QStringLiteral("旁白")
            || normalizedSpeaker == QStringLiteral("叙述")) {
            continue;
        }

        if (!speakers.contains(speaker)) {
            speakers.append(speaker);
        }
    }
    return speakers;
}

QString PromptBuilder::resolveSceneRefPath(const QMap<QString, QJsonObject> &sceneRefs,
                                             const QString &sceneId,
                                             const QString &sceneName)
{
    QString refPath;
    if (!sceneId.isEmpty() && sceneRefs.contains(sceneId)) {
        refPath = sceneRefs.value(sceneId)["referenceImagePath"].toString();
    } else if (!sceneName.isEmpty() && sceneRefs.contains(sceneName)) {
        refPath = sceneRefs.value(sceneName)["referenceImagePath"].toString();
    } else {
        const QStringList requestedKeys = SceneKeyUtils::buildSceneIdentityKeys(
            sceneId,
            sceneName,
            QString(),
            QString(),
            QStringList(),
            QStringList());
        for (auto it = sceneRefs.constBegin(); it != sceneRefs.constEnd(); ++it) {
            const QJsonObject sceneObj = it.value();
            QStringList sceneKeys = SceneKeyUtils::buildSceneIdentityKeys(sceneObj);
            sceneKeys.prepend(it.key());
            if (sceneKeySetsMatch(requestedKeys, sceneKeys)) {
                const QString candidateRefPath = sceneObj["referenceImagePath"].toString();
                if (!candidateRefPath.isEmpty()) {
                    refPath = candidateRefPath;
                    break;
                }
            }
        }
    }

    if (refPath.isEmpty()) {
        QStringList uniqueRefPaths;
        for (auto it = sceneRefs.constBegin(); it != sceneRefs.constEnd(); ++it) {
            const QString candidateRefPath = it.value()["referenceImagePath"].toString().trimmed();
            if (!candidateRefPath.isEmpty() && !uniqueRefPaths.contains(candidateRefPath)) {
                uniqueRefPaths.append(candidateRefPath);
            }
        }

        if (uniqueRefPaths.size() == 1) {
            refPath = uniqueRefPaths.first();
        }
    }

    return refPath;
}

void PromptBuilder::appendVisualCharacteristics(QStringList &parts, const QJsonObject &visual)
{
    if (visual.isEmpty()) return;
    
    addIfNotEmpty(parts, sanitizeSceneText(visual["architecture"].toString()));
    
    QStringList landmarks = normalizeList(visual["keyLandmarks"]);
    if (!landmarks.isEmpty()) {
        QStringList filtered;
        for (const QString& landmark : landmarks) {
            QString f = sanitizeSceneText(landmark);
            if (!f.isEmpty()) filtered.append(f);
        }
        if (!filtered.isEmpty()) {
            parts.append(QString("landmarks: %1").arg(filtered.join(", ")));
        }
    }
    
    QString colorScheme = sanitizeSceneText(visual["colorScheme"].toString());
    if (!colorScheme.isEmpty()) {
        parts.append(QString("color palette %1").arg(colorScheme));
    }
    
    QJsonObject lighting = visual["lighting"].toObject();
    if (!lighting.isEmpty()) {
        QString naturalLight = sanitizeSceneText(lighting["naturalLight"].toString());
        QString artificialLight = sanitizeSceneText(lighting["artificialLight"].toString());
        if (!naturalLight.isEmpty()) {
            parts.append(QString("natural light %1").arg(naturalLight));
        }
        if (!artificialLight.isEmpty()) {
            parts.append(QString("artificial light %1").arg(artificialLight));
        }
    }
    
    addIfNotEmpty(parts, sanitizeSceneText(visual["atmosphere"].toString()));
}

void PromptBuilder::appendSpatialLayout(QStringList &parts, const QJsonObject &spatialLayout)
{
    if (spatialLayout.isEmpty()) return;
    
    QString size = spatialLayout["size"].toString();
    if (!size.isEmpty()) {
        parts.append(SPACE_SIZE_MAPPING.value(size, QString("%1 space").arg(size)));
    }
    QString layout = sanitizeSceneText(spatialLayout["layout"].toString());
    if (!layout.isEmpty()) {
        parts.append(QString("spatial layout: %1").arg(layout));
    }
    QJsonArray keyAreas = spatialLayout["keyAreas"].toArray();
    if (!keyAreas.isEmpty()) {
        QStringList areaDescs;
        for (const auto& areaVal : keyAreas) {
            QJsonObject area = areaVal.toObject();
            QString areaName = area["name"].toString();
            QString position = area["position"].toString();
            if (!areaName.isEmpty()) {
                areaDescs.append(position.isEmpty() ? areaName : QString("%1 (%2)").arg(areaName, position));
            }
        }
        if (!areaDescs.isEmpty()) {
            parts.append(QString("key areas: %1").arg(areaDescs.join(", ")));
        }
    }
}

void PromptBuilder::appendVariationDescs(QStringList &parts,
                                           const QJsonArray &variations,
                                           const QMap<QString, QString> &mapping,
                                           const QString &keyField,
                                           const QString &label)
{
    if (variations.isEmpty()) return;
    
    QStringList descs;
    for (const auto& val : variations) {
        QJsonObject obj = val.toObject();
        QString key = obj[keyField].toString();
        QString desc = obj["description"].toString();
        if (!key.isEmpty()) {
            QString mapped = mapping.value(key, key);
            if (!desc.isEmpty()) {
                mapped += ", " + desc;
            }
            descs.append(mapped);
        }
    }
    if (!descs.isEmpty()) {
        parts.append(QString("%1: %2").arg(label, descs.join("; ")));
    }
}

void PromptBuilder::appendScenePromptDetails(QStringList& parts, const QJsonObject& scene)
{
    addIfNotEmpty(parts, sanitizeSceneText(scene["description"].toString()));

    appendVisualCharacteristics(parts, scene["visualCharacteristics"].toObject());
    appendSpatialLayout(parts, scene["spatialLayout"].toObject());
    addListIfNotEmpty(parts, normalizeList(scene["anchorPoints"]), "fixed anchors:");
    addListIfNotEmpty(parts, normalizeList(scene["signatureObjects"]), "signature objects:");
    addListIfNotEmpty(parts, normalizeList(scene["fixedColorBlocks"]), "fixed color blocks:");
    addListIfNotEmpty(parts, normalizeList(scene["consistencyRules"]), "consistency rules:");

    const QString currentInterpretation = scene["currentInterpretation"].toString().trimmed();
    if (!currentInterpretation.isEmpty()) {
        parts.append(QString("current interpretation: %1").arg(currentInterpretation));
    }

    const QString status = scene["status"].toString().trimmed();
    if (!status.isEmpty()) {
        parts.append(QString("status: %1").arg(status));
    }

    const QString confidence = scene["confidence"].toString().trimmed();
    if (!confidence.isEmpty()) {
        parts.append(QString("confidence: %1").arg(confidence));
    }

    addListIfNotEmpty(parts, normalizeList(scene["evidence"]), "evidence:");
    addListIfNotEmpty(parts, normalizeList(scene["aliases"]), "aliases:");
    addListIfNotEmpty(parts, normalizeList(scene["history"]), "history:");
    appendVariationDescs(parts, scene["timeVariations"].toArray(), TIME_OF_DAY_MAPPING, "timeOfDay", "time variations");
    appendVariationDescs(parts, scene["weatherVariations"].toArray(), WEATHER_MAPPING, "weather", "weather variations");
    // narrativeRole是叙事性描述，不应加入场景图片生成prompt
    // 避免AI根据"温柔重逢"等叙事性词汇生成人物
    // addIfNotEmpty(parts, sanitizeSceneText(scene["narrativeRole"].toString()));
}

PromptBuilder::PromptResult PromptBuilder::buildCharacterPrompt(const QJsonObject &character,
                                                                  const QJsonObject &options)
{
    QString name = character["name"].toString();
    QString role = character["role"].toString();
    QJsonObject appearance = character["appearance"].toObject();
    QStringList tags = normalizeList(character["tags"]);
    
    QString view = optionOrDefault(options, "view", "front");
    QString pose = optionOrDefault(options, "pose", "standing");
    QString style = optionOrDefault(options, "style", "manga");
    QString mode = optionOrDefault(options, "mode", "preview");
    
    QStringList parts;
    parts << "single character only, one person";
    addIfNotEmpty(parts, role.isEmpty() ? QString() : QString("%1 archetype").arg(role));
    addIfNotEmpty(parts, name);
    appendCharacterFramingParts(parts);
    parts << "ultra detailed manga character portrait" << JAPANESE_MANGA_DIRECTIVE;
    
    parts.append(VIEW_MAPPING.value(view, QString("%1 view").arg(view)));
    parts.append(POSE_MAPPING.value(pose, QString("%1 pose").arg(pose)));
    parts.append(STYLE_MAPPING.value(style, style));
    parts.append(mode == "preview" ? "illustrated render" : "high resolution render");
    
    addIfNotEmpty(parts, buildCharacterAppearance(appearance));
    addListIfNotEmpty(parts, tags, "style keywords:");
    
    appendCharacterBackgroundParts(parts);
    
    QString prompt = truncatePrompt(parts, MAX_PROMPT_LENGTH);
    return { prompt, NegativePromptFactory::forCharacter(), {} };
}

PromptBuilder::PromptResult PromptBuilder::buildPanelPrompt(const QJsonObject &panel,
                                                              const QMap<QString, QJsonObject> &characterRefs,
                                                              const QMap<QString, QJsonObject> &sceneRefs,
                                                              const QJsonObject &options)
{
    QString mode = optionOrDefault(options, "mode", "preview");
    const bool isImg2Img = options.value("isImg2Img").toBool();
    
    LOG_DEBUG("PromptBuilder", QString("buildPanelPrompt entered: panelId=%1, mode=%2, isImg2Img=%3, charRefs=%4, sceneRefs=%5")
        .arg(panel.value("id").toString())
        .arg(mode)
        .arg(isImg2Img)
        .arg(characterRefs.size())
        .arg(sceneRefs.size()));
    
    const bool hasDialogue = !panel["dialogue"].toArray().isEmpty();
    Q_UNUSED(hasDialogue);
    
    QJsonObject background = panel["background"].toObject();
    QString sceneId = background["sceneId"].toString();
    QString sceneName = background["setting"].toString();
    QString panelSceneText = panel["scene"].toString();

    QString visualPrompt = panel["visualPrompt"].toString().trimmed();
    QString visualPromptEn = panel["visualPromptEn"].toString().trimmed();
    QString editPrompt = combineEditPrompt(visualPrompt, visualPromptEn);
    QString editIntent = panel["editIntent"].toString().trimmed();
    const QString promptTarget = PromptTargetUtils::resolveEditPromptTarget(
        panel,
        options.value("promptTarget").toString(),
        editPrompt,
        panel["scene"].toString());

    QStringList dialogueSpeakers = extractDialogueSpeakers(panel["dialogue"].toArray());
    QString primarySceneRef = resolveSceneRefPath(sceneRefs, sceneId, sceneName);

    const PromptBuilder::PanelCharacterPromptData characterData = collectPanelCharacterPromptData(panel["characters"].toArray(),
                                                                                  characterRefs,
                                                                                  dialogueSpeakers);
    
    QString selectedRefType;
    QString selectedRef = selectPanelReference(promptTarget,
                                               editIntent,
                                               editPrompt,
                                               characterData.primaryCharacterRef,
                                               primarySceneRef,
                                               selectedRefType);

    QStringList parts;

    const QString visualPromptForCheck = panel["visualPrompt"].toString().trimmed();
    detectVisualPromptConflicts(visualPromptForCheck, characterRefs);

    if (isImg2Img) {
        // 图生图使用五层优化架构（≤280字符，充分利用数据库字段）
        parts = buildOptimizedImg2ImgParts(panel, characterData, characterRefs, sceneRefs, sceneId, sceneName);
    } else {
        // 文生图使用完整模式（≤300汉字）
        
        // 1. PURPOSE - 用途
        parts << "[PURPOSE] 单幅漫画分镜";
        
        // 2. SUBJECT - 主题/场景
        QStringList subjectParts;
        if (!editPrompt.isEmpty()) {
            subjectParts << buildEditDirective(editPrompt, promptTarget, editIntent);
        }
        addIfNotEmpty(subjectParts, matchSceneDetails(sceneRefs, sceneId, sceneName));
        addIfNotEmpty(subjectParts, panelSceneText);
        if (!subjectParts.isEmpty()) {
            parts << QString("[场景] %1").arg(subjectParts.join("，"));
        }
        
        // 3. ELEMENTS - 角色
        if (!characterData.characterDescriptions.isEmpty()) {
            parts << QString("[角色] %1").arg(characterData.characterDescriptions.join("；"));
        }
        
        // 4. COMPOSITION - 构图
        QStringList compParts;
        const QString shotType = panel["shotType"].toString();
        if (!shotType.isEmpty()) {
            compParts << SHOT_TYPE_MAPPING.value(shotType, QString("%1镜头").arg(shotType));
        }
        const QString mood = panel["atmosphere"].toObject()["mood"].toString();
        if (!mood.isEmpty()) {
            compParts << QString("%1氛围").arg(mood);
        }
        if (!compParts.isEmpty()) {
            parts << QString("[构图] %1").arg(compParts.join("，"));
        }
        
        // 5. TEXT - 文字
        addIfNotEmpty(parts, formatDialogueForPrompt(panel["dialogue"].toArray(),
                                                      characterRefs));
        
        // 6. STYLE - 风格
        parts << "[风格] 日漫风格，全彩，高质量渲染";
    }

    logPanelPromptSelection(panel,
                            promptTarget,
                            selectedRefType,
                            selectedRef,
                            primarySceneRef,
                            characterData.primaryCharacterRef,
                            characterData.panelCharNames,
                            dialogueSpeakers);

    // 图生图限制350中文字（官方建议300左右，留50字符缓冲），文生图限制380中文字
    // 参考: https://www.volcengine.com/docs/85128/1798096 (seed3l_multi_ip 模型)
    const int maxLen = isImg2Img ? 350 : 380;
    QString prompt = truncatePrompt(parts, maxLen);
    return { prompt,
             NegativePromptFactory::forPanel(),
             buildPanelReferenceUris(selectedRefType,
                                     selectedRef,
                                     characterData.primaryCharacterRef,
                                     primarySceneRef,
                                     characterData.allCharacterRefs) };
}

PromptBuilder::PromptResult PromptBuilder::buildScenePrompt(const QJsonObject &scene,
                                                               const QJsonObject &options)
{
    Q_UNUSED(options)
    QStringList parts;
    
    parts << "environment concept art" << "ultra detailed manga background"
          << "empty scene, no characters, no people, no human figures, no silhouettes, 纯场景无人物"
          << "use as a strict environment reference, preserve architecture and atmosphere"
          << JAPANESE_MANGA_DIRECTIVE;
    
    QString name = scene["name"].toString();
    if (!name.isEmpty()) {
        parts.append(QString("scene name: %1").arg(name));
    }
    QString visualPrompt = scene["visualPrompt"].toString().trimmed();
    QString visualPromptEn = scene["visualPromptEn"].toString().trimmed();
    QString editPrompt = combineEditPrompt(visualPrompt, visualPromptEn);
    if (!editPrompt.isEmpty()) {
        parts.append(QString("scene edit only, preserve layout and atmosphere, apply this edit strictly: %1")
            .arg(editPrompt));
    }
    appendScenePromptDetails(parts, scene);
    
    parts << "high detail, volumetric lighting, cinematic environment, vibrant colors, full color";
    
    QString prompt = truncatePrompt(parts, MAX_PROMPT_LENGTH);
    return { prompt, NegativePromptFactory::forScene(), {} };
}
