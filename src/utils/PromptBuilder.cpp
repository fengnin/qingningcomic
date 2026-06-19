#include "utils/PromptBuilder.h"
#include "services/CharacterExtractor.h"
#include "utils/ChangeRequestIntentUtils.h"
#include "utils/DialogueSpeakerSideUtils.h"
#include "utils/Logger.h"
#include "utils/PromptTargetUtils.h"
#include "utils/SceneKeyUtils.h"
#include <QRegularExpression>
#include <QSet>

namespace {
    const QString JAPANESE_MANGA_DIRECTIVE =
        QString::fromUtf8("Japanese manga style, anime aesthetics, clean screentone line art, "
                          "high contrast, expressive faces, vibrant color palette");

    // ============================================================
    // Negative Prompt 工厂：统一管理所有类型的否定词，消除重复
    // ============================================================
    namespace NegativePromptFactory {

        inline QString sharedBase() {
            return QString::fromUtf8(
                "nsfw, blurry, low quality, extra limbs, deformed hands, "
                "text watermark, logo, cropped face, overexposed, underexposed"
            );
        }

        inline QString colorRestrictions() {
            return QStringLiteral(", monochrome, grayscale, black and white, ");
        }

        inline QString styleRestrictions() {
            return QStringLiteral(
                "photorealistic, realistic, photo, photograph, real person, "
                "realistic skin texture, real human, 3d render, cgi"
            );
        }

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

        inline QString forCharacter() {
            return colorRestrictions() +
                sharedBase() + ", " +
                styleRestrictions() + ", " +
                cleanBackground() + ", " +
                QStringLiteral(
                    "silhouette, drop shadow, cast shadow, ambient occlusion, "
                    "face in background, multiple people, extra person, duplicate person, "
                    "background character, background figure, giant face, large face in background, "
                    "close-up face, bust portrait, background portrait, duplicate same character, "
                    "two scales of same character, character sheet, layered character composition, "
                    "face behind character, 背景大脸, 背景头像, 双重人物, 人物设定图, 前景全身后景头像"
                );
        }

        inline QString forScene() {
            return colorRestrictions() +
                sharedBase() + ", " +
                QStringLiteral(
                    "person, people, human, character, figure, face, body, silhouette, "
                    "portrait, anime character, manga character, girl, boy, man, woman, "
                    "standing figure, sitting figure, walking figure, "
                    "人物, 人, 角色, 女孩, 男孩, 女性, 男性, 人影"
                );
        }

        inline QString forPanel() {
            return colorRestrictions() +
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

    // 火山引擎 prompt 字数估算：中文字 1 单元，连续英文字母/数字 1 单元，
    // 标点、空格、其他符号不计。比纯字符数更贴近模型的实际 token 消耗。
    int countPromptUnits(const QString& text)
    {
        int units = 0;
        bool inWord = false;
        for (const QChar& c : text) {
            if (c.isLetterOrNumber()) {
                if (c.unicode() >= 0x4E00 && c.unicode() <= 0x9FFF) {
                    units++;
                    inWord = false;
                } else if (!inWord) {
                    units++;
                    inWord = true;
                }
            } else {
                inWord = false;
            }
        }
        return units;
    }

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
        {"standing", "站立姿势"},
        {"sitting", "坐姿"},
        {"walking", "行走姿势"},
        {"running", "奔跑姿势"},
        {"action", "动感姿势"},
        {"fighting", "战斗姿势"},
        {"defensive", "防御姿势"},
        {"casual", "休闲姿势"},
        {"formal", "正式姿势"},
        {"dramatic", "戏剧性姿势"}
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
        {"close-up", "\u7279\u5199\u955c\u5934"},
        {"extreme-close-up", "\u6781\u7aef\u7279\u5199"},
        {"medium", "\u4e2d\u666f\u955c\u5934"},
        {"medium-close-up", "\u8fd1\u666f\u955c\u5934"},
        {"full", "\u5168\u8eab\u955c\u5934"},
        {"wide", "\u5e7f\u89d2\u955c\u5934"},
        {"extreme-wide", "\u8d85\u5e7f\u89d2\u955c\u5934"},
        {"establishing", "\u5efa\u7acb\u955c\u5934"},
        {"over-the-shoulder", "\u8fc7\u80a9\u955c\u5934"},
        {"point-of-view", "\u4e3b\u89c2\u955c\u5934"},
        {QString::fromUtf8("\u7279\u5199"), "\u7279\u5199\u955c\u5934"},
        {QString::fromUtf8("\u8fd1\u666f"), "\u8fd1\u666f\u955c\u5934"},
        {QString::fromUtf8("\u4e2d\u666f"), "\u4e2d\u666f\u955c\u5934"},
        {QString::fromUtf8("\u5168\u666f"), "\u5168\u8eab\u955c\u5934"},
        {QString::fromUtf8("\u8fdc\u666f"), "\u5e7f\u89d2\u955c\u5934"},
        {QString::fromUtf8("\u5927\u8fdc\u666f"), "\u8d85\u5e7f\u89d2\u955c\u5934"}
    });

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
            description += "，" + appearanceDescription;
        } else if (description.isEmpty()) {
            description = appearanceDescription;
        }
        return description;
    }

    QString resolveCharacterRefKey(const QString& charName,
                                   const QMap<QString, QJsonObject>& characterRefs)
    {
        if (characterRefs.contains(charName)) {
            return charName;
        }
        const QString normalized = normalizeCharacterNameForRefs(charName);
        if (characterRefs.contains(normalized)) {
            return normalized;
        }
        return QString();
    }

    bool isCoreCharacterRole(const QString& role)
    {
        return role.contains(QStringLiteral("protagonist"), Qt::CaseInsensitive)
            || role.contains(QStringLiteral("main"), Qt::CaseInsensitive)
            || role.contains(QStringLiteral("lead"), Qt::CaseInsensitive)
            || role.contains(QStringLiteral("hero"), Qt::CaseInsensitive)
            || role.contains(QString::fromUtf8("主角"), Qt::CaseInsensitive);
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

            const QString matchedKey = resolveCharacterRefKey(charName, characterRefs);
            const QString pose = charObj["pose"].toString();
            const QString expression = charObj["expression"].toString();

            if (matchedKey.isEmpty()) {
                const QString charDesc = buildPanelCharacterDescription(charName, pose, expression, QJsonObject());
                if (!charDesc.isEmpty()) {
                    data.characterDescriptions.append(charDesc);
                }
                continue;
            }

            const QJsonObject fullChar = characterRefs.value(matchedKey);
            const QJsonObject appearance = fullChar["appearance"].toObject();
            const QStringList portraitPaths = PromptBuilder::normalizeList(fullChar["portraitPaths"].toArray());

            const QString charDesc = buildPanelCharacterDescription(charName, pose, expression, appearance);
            if (!charDesc.isEmpty()) {
                data.characterDescriptions.append(charDesc);
            }

            const QString portraitPath = PromptBuilder::firstNonEmptyPortrait(portraitPaths);
            if (portraitPath.isEmpty()) {
                continue;
            }

            const bool isDialogSpeaker = dialogueSpeakers.contains(charName) || dialogueSpeakers.contains(matchedKey);
            const bool coreRole = isCoreCharacterRole(fullChar["role"].toString().trimmed());
            const int priority = (isDialogSpeaker || coreRole) ? 0 : 2;

            if (isDialogSpeaker || (coreRole && data.primaryCharacterRef.isEmpty())) {
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

        auto returnRef = [&](const QString& ref, const QString& type) -> QString {
            selectedRefType = type;
            return ref;
        };
        auto returnNone = [&]() -> QString {
            selectedRefType.clear();
            return QString();
        };

        // For local-object edits (move/insert/remove), the panel source image is the
        // real primary reference — ImageService prepends it after PromptBuilder returns.
        // Picking a scene reference here would push it ahead of the panel image and
        // confuse the model about what to edit. Use character ref as secondary instead.
        if (PromptTargetUtils::isLocalObjectMovementIntent(editIntent)
            || editIntent.trimmed().toLower() == QStringLiteral("remove_subject")) {
            if (!primaryCharacterRef.isEmpty()) {
                return returnRef(primaryCharacterRef, QStringLiteral("character"));
            }
            return returnNone();
        }

        if (normalizedPromptTarget == QStringLiteral("scene")) {
            if (!primarySceneRef.isEmpty()) {
                return returnRef(primarySceneRef, QStringLiteral("scene"));
            }
            if (!primaryCharacterRef.isEmpty()) {
                return returnRef(primaryCharacterRef, QStringLiteral("character"));
            }
            return returnNone();
        }

        if (normalizedPromptTarget == QStringLiteral("character")) {
            if (!primaryCharacterRef.isEmpty()) {
                return returnRef(primaryCharacterRef, QStringLiteral("character"));
            }
            if (!primarySceneRef.isEmpty()) {
                return returnRef(primarySceneRef, QStringLiteral("scene"));
            }
            return returnNone();
        }

        const bool preferSceneReference = PromptTargetUtils::shouldPreferSceneReferenceForEditIntent(
            editIntent, editPrompt);

        if (preferSceneReference && !primarySceneRef.isEmpty()) {
            return returnRef(primarySceneRef, QStringLiteral("scene"));
        }
        if (!primaryCharacterRef.isEmpty()) {
            return returnRef(primaryCharacterRef, QStringLiteral("character"));
        }
        if (!primarySceneRef.isEmpty()) {
            return returnRef(primarySceneRef, QStringLiteral("scene"));
        }
        return returnNone();
    }

    QStringList buildPanelReferenceUris(const QString& selectedRefType,
                                        const QString& primaryCharacterRef,
                                        const QString& primarySceneRef,
                                        const QStringList& allCharacterRefs,
                                        QMap<QString, QString>& outRefTypes)
    {
        QStringList refUris;
        QStringList orderedRefs;

        // character-first: put character refs before scene so they survive maxRefImages truncation.
        // scene-first: scene ref leads (e.g. background-edit intent).
        if (selectedRefType == QStringLiteral("scene")) {
            orderedRefs << primarySceneRef << allCharacterRefs;
        } else {
            orderedRefs << allCharacterRefs << primarySceneRef;
        }

        LOG_INFO("PromptBuilder", QString("构建参考图列表: selectedRefType=%1, primaryCharacterRef=%2, primarySceneRef=%3, allCharacterRefs=%4")
            .arg(selectedRefType.isEmpty() ? "(无)" : selectedRefType)
            .arg(primaryCharacterRef.isEmpty() ? "(无)" : primaryCharacterRef)
            .arg(primarySceneRef.isEmpty() ? "(无)" : primarySceneRef)
            .arg(allCharacterRefs.size()));

        QSet<QString> characterSet;
        for (const QString& ref : allCharacterRefs) {
            if (!ref.isEmpty()) {
                characterSet.insert(ref);
            }
        }

        for (const QString& ref : orderedRefs) {
            if (ref.isEmpty() || refUris.contains(ref)) {
                continue;
            }
            refUris.append(ref);
            LOG_INFO("PromptBuilder", QString("  参考图[%1]: %2").arg(refUris.size()).arg(ref));

            if (ref == primarySceneRef) {
                outRefTypes.insert(ref, QStringLiteral("STYLE"));
            } else if (characterSet.contains(ref)) {
                outRefTypes.insert(ref, QStringLiteral("IP"));
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
        QString sideHint;
        if (speakerSide == QStringLiteral("left")) {
            sideHint = QStringLiteral("左侧");
        } else if (speakerSide == QStringLiteral("right")) {
            sideHint = QStringLiteral("右侧");
        }

        QString speakerDesc = speaker;
        if (!speaker.isEmpty() && !characterRefs.isEmpty()) {
            QString matchedName = speaker;
            if (!characterRefs.contains(speaker)) {
                const QString normalized = normalizeCharacterNameForRefs(speaker);
                if (characterRefs.contains(normalized))
                    matchedName = normalized;
            }
            if (characterRefs.contains(matchedName)) {
                const QJsonObject appearance = characterRefs.value(matchedName)["appearance"].toObject();
                QStringList noteParts;
                const QString hairColor = appearance["hairColor"].toString().trimmed();
                const QString hairStyle = appearance["hairStyle"].toString().trimmed();
                if (!hairColor.isEmpty() && !hairStyle.isEmpty())
                    noteParts << hairColor + hairStyle;
                else if (!hairColor.isEmpty())
                    noteParts << hairColor + "发";
                else if (!hairStyle.isEmpty())
                    noteParts << hairStyle;
                const QString eyeColor = appearance["eyeColor"].toString().trimmed();
                if (!eyeColor.isEmpty()) noteParts << eyeColor + "眼";
                if (!noteParts.isEmpty())
                    speakerDesc = speaker + QString("【%1】").arg(noteParts.join("、"));
            }
        }

        if (!speakerDesc.isEmpty() && !sideHint.isEmpty()) {
            return QString::fromUtf8("对话气泡，内容「%1」，气泡尾指向%2的%3")
                   .arg(text, sideHint, speakerDesc);
        }
        if (!speakerDesc.isEmpty()) {
            return QString::fromUtf8("对话气泡，内容「%1」，气泡尾指向%2")
                   .arg(text, speakerDesc);
        }
        return QString::fromUtf8("对话气泡，内容「%1」").arg(text);
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

    // [场景] 层：圣经 + visualPromptCn（中文优先），互斥去重
    QString buildSceneLayer(const QString& sceneDesc,
                            const QString& panelSceneText,
                            const QString& visualPromptCn,
                            const QString& editPrompt,
                            const QString& promptTarget,
                            const QString& editIntent)
    {
        QStringList parts;
        addIfNotEmpty(parts, sceneDesc);
        if (sceneDesc.isEmpty()) addIfNotEmpty(parts, panelSceneText);
        if (!visualPromptCn.isEmpty())
            parts << visualPromptCn;
        else if (!editPrompt.isEmpty())
            parts << buildEditDirective(editPrompt, promptTarget, editIntent);
        return parts.isEmpty() ? QString() : QString("[场景] %1").arg(parts.join("，"));
    }

    // [构图] 层：镜头类型 + 氛围
    QString buildCompositionLayer(const QString& shotType, const QString& mood)
    {
        QStringList parts;
        if (!shotType.isEmpty())
            parts << SHOT_TYPE_MAPPING.value(shotType, QString("%1镜头").arg(shotType));
        if (!mood.isEmpty())
            parts << QString("%1氛围").arg(mood);
        return parts.isEmpty() ? QString() : QString("[构图] %1").arg(parts.join("，"));
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
        {"neutral", "平静表情"},
        {"happy", "开心微笑"},
        {"sad", "悲伤表情"},
        {"angry", "愤怒表情"},
        {"surprised", "惊讶表情"},
        {"scared", "恐惧表情"},
        {"determined", "坚定神情"},
        {"confused", "困惑表情"},
        {"embarrassed", "害羞表情"},
        {"smirking", "得意微笑"}
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
        parts << "exactly one full-body character only, one person in the entire image";
        parts << "centered standing pose, head-to-toe visible, clean white margin around the character";
        parts << "single scale character reference, no portrait in background, no giant face, no bust shot, no duplicate character, no character sheet, no layered composition";
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

    QString truncatePrompt(const QStringList &parts, int maxUnits) {
        QString result = parts.join(", ");
        if (countPromptUnits(result) <= maxUnits) {
            return result;
        }

        QStringList truncated;
        int currentUnits = 0;
        for (const QString &part : parts) {
            int addUnits = countPromptUnits(part);
            if (currentUnits + addUnits > maxUnits) {
                break;
            }
            truncated.append(part);
            currentUnits += addUnits;
        }

        QString truncatedResult = truncated.join(", ");

        // 兜底：如果按单元截断后为空，按字符数粗暴截（避免完全丢失内容）
        if (truncatedResult.isEmpty() && !result.isEmpty()) {
            return result.left(maxUnits);
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
        segments.append(POSE_MAPPING.value(pose, QString("%1姿势").arg(pose)));
    }
    if (!expression.isEmpty()) {
        segments.append(EXPRESSION_MAPPING.value(expression, QString("%1表情").arg(expression)));
    }
    return segments.isEmpty() ? QString() : segments.join("，");
}

QString PromptBuilder::buildCharacterAppearance(const QJsonObject &appearance)
{
    if (appearance.isEmpty()) {
        return QString();
    }

    QStringList parts;

    const QString hairColor = appearance["hairColor"].toString().trimmed();
    const QString hairStyle = appearance["hairStyle"].toString().trimmed();
    if (!hairColor.isEmpty() && !hairStyle.isEmpty()) {
        parts.append(QString("%1%2").arg(hairColor, hairStyle));
    } else if (!hairColor.isEmpty()) {
        parts.append(hairColor + "发");
    } else if (!hairStyle.isEmpty()) {
        parts.append(hairStyle);
    }

    const QString eyeColor = appearance["eyeColor"].toString().trimmed();
    if (!eyeColor.isEmpty()) {
        parts.append(eyeColor + "眼睛");
    }

    QStringList clothing = normalizeList(appearance["clothing"]);
    if (!clothing.isEmpty()) {
        parts.append(QString("穿着%1").arg(clothing.join("、")));
    }

    QStringList accessories = normalizeList(appearance["accessories"]);
    if (!accessories.isEmpty()) {
        parts.append(QString("配饰：%1").arg(accessories.join("、")));
    }

    QStringList features = normalizeList(appearance["distinctiveFeatures"]);
    if (!features.isEmpty()) {
        parts.append(QString("明显特征：%1").arg(features.join("、")));
    }

    return parts.join("，");
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
    // 多级回退查找，对齐 fetchSceneRefs 的多形态索引
    auto findDetails = [&](const QString& key) -> QJsonObject {
        if (key.isEmpty()) return {};
        if (sceneRefs.contains(key)) return sceneRefs.value(key)["details"].toObject();
        return {};
    };

    QJsonObject matched;
    // 1. 精确匹配 sceneId / sceneName
    matched = findDetails(sceneId);
    if (matched.isEmpty()) matched = findDetails(sceneName);
    // 2. 标准化后匹配
    if (matched.isEmpty()) matched = findDetails(SceneKeyUtils::normalizeSceneLabel(sceneId));
    if (matched.isEmpty()) matched = findDetails(SceneKeyUtils::normalizeSceneLabel(sceneName));
    // 3. coreKey 匹配
    if (matched.isEmpty()) matched = findDetails(SceneKeyUtils::buildSceneCoreKey(sceneId));
    if (matched.isEmpty()) matched = findDetails(SceneKeyUtils::buildSceneCoreKey(sceneName));
    // 4. contains 兜底（处理子位置归并，如"拾光书店门口"→"拾光书店"）
    if (matched.isEmpty()) {
        const QString needle = SceneKeyUtils::normalizeSceneLabel(sceneName).trimmed();
        if (!needle.isEmpty()) {
            for (auto it = sceneRefs.constBegin(); it != sceneRefs.constEnd(); ++it) {
                const QString& k = it.key();
                if (k.contains(needle) || needle.contains(k)) {
                    matched = it.value()["details"].toObject();
                    break;
                }
            }
        }
    }

    if (matched.isEmpty()) return QString();

    // 核心视觉字段，限制总长度避免挤压其他 prompt 内容
    QStringList parts;
    addIfNotEmpty(parts, sanitizeSceneText(matched["building"].toString()));
    addIfNotEmpty(parts, sanitizeSceneText(matched["atmosphere"].toString()));

    // 从 JSON 数组取前 maxCount 条非空项
    auto takeFirst = [&](const char* key, int maxCount) {
        int count = 0;
        for (const auto& v : matched[key].toArray()) {
            if (count >= maxCount) break;
            const QString s = sanitizeSceneText(v.toString().trimmed());
            if (!s.isEmpty()) { parts << s; ++count; }
        }
    };
    takeFirst("anchorPoints",      2);
    takeFirst("consistencyRules",  2);
    takeFirst("signatureObjects",  1);
    takeFirst("fixedColorBlocks",  1);

    addIfNotEmpty(parts, sanitizeSceneText(matched["spaceSize"].toString()));

    // 整体截断：场景圣经前缀最多 120 字符，留余量给 visualPrompt 和角色
    QString result = parts.join("，");
    if (result.length() > 120) result = result.left(120);
    return result;
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
    // narrativeRole是叙事性描述，不应加入场景图片生成prompt，避免AI根据叙事性词汇生成人物
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
    return { prompt, NegativePromptFactory::forCharacter(), {}, {} };
}

PromptBuilder::PromptResult PromptBuilder::buildPanelPrompt(const QJsonObject &panel,
                                                              const QMap<QString, QJsonObject> &characterRefs,
                                                              const QMap<QString, QJsonObject> &sceneRefs,
                                                              const QJsonObject &options)
{
    QString mode = optionOrDefault(options, "mode", "preview");

    LOG_DEBUG("PromptBuilder", QString("buildPanelPrompt entered: panelId=%1, mode=%2, charRefs=%3, sceneRefs=%4")
        .arg(panel.value("id").toString())
        .arg(mode)
        .arg(characterRefs.size())
        .arg(sceneRefs.size()));

    QJsonObject background = panel["background"].toObject();
    QString sceneId = background["sceneId"].toString();
    QString sceneName = background["setting"].toString();

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

    const PromptBuilder::PanelCharacterPromptData characterData = collectPanelCharacterPromptData(
        panel["characters"].toArray(), characterRefs, dialogueSpeakers);

    QString selectedRefType;
    QString selectedRef = selectPanelReference(promptTarget, editIntent, editPrompt,
                                               characterData.primaryCharacterRef,
                                               primarySceneRef, selectedRefType);

    // 多图组合生成（即梦AI4.0）六层结构
    QStringList parts;
    parts << "单幅漫画分镜";

    addIfNotEmpty(parts, buildSceneLayer(
        matchSceneDetails(sceneRefs, sceneId, sceneName),
        panel["scene"].toString(),
        panel["visualPromptCn"].toString().trimmed(),
        editPrompt, promptTarget, editIntent));

    if (!characterData.characterDescriptions.isEmpty())
        parts << QString("[角色] %1").arg(characterData.characterDescriptions.join("；"));

    addIfNotEmpty(parts, buildCompositionLayer(
        panel["shotType"].toString(),
        panel["atmosphere"].toObject()["mood"].toString()));

    addIfNotEmpty(parts, formatDialogueForPrompt(panel["dialogue"].toArray(), characterRefs));

    parts << "[风格] 日漫风格，全彩，高质量渲染";

    logPanelPromptSelection(panel,
                            promptTarget,
                            selectedRefType,
                            selectedRef,
                            primarySceneRef,
                            characterData.primaryCharacterRef,
                            characterData.panelCharNames,
                            dialogueSpeakers);

    // prompt 字数按 countPromptUnits 计：中文字 1 单元，连续英文字母/数字 1 单元，标点空格不计
    // 即梦AI 4.0 官方限制800字符，600单元留有余量（前缀注入约50字符）
    const int maxLen = 600;
    QString prompt = truncatePrompt(parts, maxLen);
    QMap<QString, QString> refTypes;
    QStringList refUris = buildPanelReferenceUris(selectedRefType,
                                                   characterData.primaryCharacterRef,
                                                   primarySceneRef,
                                                   characterData.allCharacterRefs,
                                                   refTypes);
    return { prompt,
             NegativePromptFactory::forPanel(),
             refUris,
             refTypes };
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
    return { prompt, NegativePromptFactory::forScene(), {}, {} };
}
