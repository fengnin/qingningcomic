#include "utils/PromptBuilder.h"
#include "services/CharacterExtractor.h"
#include "utils/Logger.h"
#include <QRegularExpression>

namespace {
    const QString JAPANESE_MANGA_DIRECTIVE = 
        QString::fromUtf8("Japanese manga style, anime aesthetics, clean screentone line art, "
                          "high contrast, expressive faces, vibrant color palette");

    const QString DEFAULT_NEGATIVE_PROMPT = 
        QString::fromUtf8("nsfw, blurry, low quality, extra limbs, deformed hands, "
                          "text watermark, logo, cropped face, overexposed, underexposed, "
                          "photorealistic, realistic, photo, photograph, real person, "
                          "realistic skin texture, real human, 3d render, cgi, "
                          "split background, half black half white, gradient background, "
                          "dark background, black border, frame, shadow on background, "
                          "double image, split image, duplicated, distorted face");

    const QString CHARACTER_NEGATIVE_EXTRA = 
        ", monochrome, grayscale, black and white, dark side, "
        "multiple people, two people, group photo, extra person, clone, "
        "multiple faces, extra faces, duplicate face, "
        "background face, face in background, "
        "background character, another person, other person, secondary person, "
        "background figure, small figure, tiny person, miniature, "
        "composite image, collage, double exposure, split composition, "
        "multiple views, dual perspective, overlay, superimposed";

    const QString SCENE_NEGATIVE_EXTRA =
        ", monochrome, grayscale, black and white, "
        "person, people, human, character, figure, face, body, "
        "portrait, anime character, manga character, girl, boy, man, woman";

    const QString PANEL_NEGATIVE_EXTRA = 
        ", monochrome, grayscale, black and white";
    
    constexpr int MAX_PROMPT_LENGTH = 1200;
    
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

    QString normalizeCharacterNameLocal(const QString& name)
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
    
    void addIfContains(QStringList& parts, const QJsonObject& obj, const QString& key, const QString& format = QString()) {
        if (obj.contains(key)) {
            QString value = obj[key].isString() ? obj[key].toString() : QString::number(obj[key].toInt());
            parts.append(format.isEmpty() ? value : QString(format).arg(value));
        }
    }
    
    void addListIfNotEmpty(QStringList& parts, const QStringList& list, const QString& prefix) {
        if (!list.isEmpty()) {
            parts.append(QString("%1 %2").arg(prefix, list.join(", ")));
        }
    }

    bool containsAnyKeyword(const QString& text, const QStringList& keywords)
    {
        const QString lower = text.toLower();
        for (const QString& keyword : keywords) {
            if (lower.contains(keyword.toLower())) {
                return true;
            }
        }
        return false;
    }

    bool isSceneFocusedPrompt(const QString& text)
    {
        static const QStringList keywords = {
            "scene", "background", "environment", "setting", "location",
            "room", "street", "building", "architecture", "landscape",
            "sky", "weather", "light", "lighting", "atmosphere",
            QString::fromUtf8("\u573a\u666f"), QString::fromUtf8("\u80cc\u666f"), QString::fromUtf8("\u73af\u5883"),
            QString::fromUtf8("\u8bbe\u5b9a"), QString::fromUtf8("\u4f4d\u7f6e"), QString::fromUtf8("\u5efa\u7b51"),
            QString::fromUtf8("\u5929\u7a7a"), QString::fromUtf8("\u6c14\u5019"), QString::fromUtf8("\u5149\u7ebf"),
            QString::fromUtf8("\u6c1b\u56f4"), QString::fromUtf8("\u98ce\u683c"), QString::fromUtf8("\u8272\u8c03")
        };
        return containsAnyKeyword(text, keywords);
    }

    bool isCharacterFocusedPrompt(const QString& text)
    {
        static const QStringList keywords = {
            "character", "person", "face", "expression", "outfit",
            "clothing", "hair", "pose", "body", "identity",
            QString::fromUtf8("\u89d2\u8272"), QString::fromUtf8("\u4eba\u7269"), QString::fromUtf8("\u9762\u90e8"),
            QString::fromUtf8("\u8868\u60c5"), QString::fromUtf8("\u670d\u88c5"), QString::fromUtf8("\u53d1\u578b"),
            QString::fromUtf8("\u59ff\u52bf"), QString::fromUtf8("\u5916\u89c2"), QString::fromUtf8("\u8eab\u6750"),
            QString::fromUtf8("\u7279\u5f81"), QString::fromUtf8("\u6027\u683c"), QString::fromUtf8("\u795e\u6001")
        };
        return containsAnyKeyword(text, keywords);
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

    QString buildEditDirective(const QString& editPrompt, const QString& target)
    {
        if (editPrompt.isEmpty()) {
            return QString();
        }

        QString normalizedTarget = target.trimmed().toLower();
        if (normalizedTarget == "scene") {
            return QString("scene edit only, keep character identity and pose stable, apply this edit strictly: %1")
                .arg(editPrompt);
        }

        if (normalizedTarget == "character") {
            return QString("character edit only, keep background and camera stable, apply this edit strictly: %1")
                .arg(editPrompt);
        }

        return QString("apply this edit strictly while preserving the existing composition: %1")
            .arg(editPrompt);
    }

    QString firstNonEmptyPortrait(const QStringList &portraitPaths) {
        for (const QString &path : portraitPaths) {
            if (!path.isEmpty()) return path;
        }
        return QString();
    }
    
    bool isNumeric(const QString &text) {
        bool ok = false;
        text.toInt(&ok);
        return ok;
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
        
        return truncated.join(", ");
    }
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
        if (isNumeric(age)) {
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
        if (!speaker.isEmpty() && !speakers.contains(speaker)) {
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

PromptBuilder::PromptResult PromptBuilder::buildCharacterPrompt(const QJsonObject &character,
                                                                  const QJsonObject &options)
{
    QString name = character["name"].toString();
    QString role = character["role"].toString();
    QJsonObject appearance = character["appearance"].toObject();
    QStringList tags = normalizeList(character["tags"]);
    
    QString view = options.value("view").toString("front");
    QString pose = options.value("pose").toString("standing");
    QString style = options.value("style").toString("manga");
    QString mode = options.value("mode").toString("preview");
    
    bool isFullBodyPose = (pose == "running" || pose == "walking" || pose == "action" || pose == "fighting");
    
    QStringList parts;
    parts << "single character only, one person";
    addIfNotEmpty(parts, role.isEmpty() ? QString() : QString("%1 archetype").arg(role));
    addIfNotEmpty(parts, name);
    if (isFullBodyPose) {
        parts << "full body shot, dynamic pose";
    } else {
        parts << "bust portrait, upper body shot";
    }
    parts << "ultra detailed manga character portrait" << JAPANESE_MANGA_DIRECTIVE;
    
    parts.append(VIEW_MAPPING.value(view, QString("%1 view").arg(view)));
    parts.append(POSE_MAPPING.value(pose, QString("%1 pose").arg(pose)));
    parts.append(STYLE_MAPPING.value(style, style));
    parts.append(mode == "preview" ? "illustrated render" : "high resolution render");
    
    addIfNotEmpty(parts, buildCharacterAppearance(appearance));
    addListIfNotEmpty(parts, tags, "style keywords:");
    
    parts << "studio lighting" 
          << "solid pure white background, plain background, no gradient, no shadow on background, completely empty background" 
          << "line art with screentone shading, vibrant colors";
    
    QString prompt = truncatePrompt(parts, MAX_PROMPT_LENGTH);
    return { prompt, DEFAULT_NEGATIVE_PROMPT + CHARACTER_NEGATIVE_EXTRA, {} };
}

PromptBuilder::PromptResult PromptBuilder::buildPanelPrompt(const QJsonObject &panel,
                                                              const QMap<QString, QJsonObject> &characterRefs,
                                                              const QMap<QString, QJsonObject> &sceneRefs,
                                                              const QJsonObject &options)
{
    QString mode = options.value("mode").toString("preview");
    
    QStringList parts;
    parts << "manga panel illustration";
    parts << "strictly preserve character identity, outfit, and scene continuity";
    
    QJsonObject background = panel["background"].toObject();
    QString sceneId = background["sceneId"].toString();
    QString sceneName = background["setting"].toString();

    QString visualPrompt = panel["visualPrompt"].toString().trimmed();
    QString visualPromptEn = panel["visualPromptEn"].toString().trimmed();
    QString editPrompt = combineEditPrompt(visualPrompt, visualPromptEn);
    QString panelSceneText = panel["scene"].toString();
    QString promptTarget = options.value("promptTarget").toString().trimmed().toLower();
    if (promptTarget.isEmpty()) {
        bool sceneFocused = isSceneFocusedPrompt(editPrompt) || isSceneFocusedPrompt(panelSceneText);
        bool characterFocused = isCharacterFocusedPrompt(editPrompt);
        if (sceneFocused && !characterFocused) {
            promptTarget = "scene";
        } else if (characterFocused && !sceneFocused) {
            promptTarget = "character";
        }
    }

    if (!editPrompt.isEmpty()) {
        parts.append(buildEditDirective(editPrompt, promptTarget));
    }
    
    addIfNotEmpty(parts, matchSceneDetails(sceneRefs, sceneId, sceneName));
    addIfNotEmpty(parts, panelSceneText);

    QString shotType = panel["shotType"].toString();
    if (!shotType.isEmpty()) {
        parts.append(SHOT_TYPE_MAPPING.value(shotType, QString("%1 shot").arg(shotType)));
    }
    addIfNotEmpty(parts, panel["cameraAngle"].toString().isEmpty() ? QString() : QString("camera angle %1").arg(panel["cameraAngle"].toString()));
    addIfContains(parts, panel["atmosphere"].toObject(), "mood", "mood %1");
    
    QJsonObject composition = panel["composition"].toObject();
    if (!composition.isEmpty()) {
        addIfContains(parts, composition, "focusPoint", "focus on %1");
        addIfContains(parts, composition, "rule", "%1 composition");
        addIfContains(parts, composition, "depthOfField", "%1 depth of field");
    }
    
    QJsonObject artStyle = panel["artStyle"].toObject();
    if (!artStyle.isEmpty()) {
        addIfContains(parts, artStyle, "genre");
        addIfContains(parts, artStyle, "lineWeight", "%1 line weight");
        addIfContains(parts, artStyle, "shading", "%1 shading");
        addIfContains(parts, artStyle, "colorPalette", "%1 palette");
    }
    
    QStringList dialogueSpeakers = extractDialogueSpeakers(panel["dialogue"].toArray());
    QString primarySceneRef = resolveSceneRefPath(sceneRefs, sceneId, sceneName);
    
    QStringList characterDescriptions;
    QString primaryCharacterRef;
    QStringList panelCharNames;
    
    for (const auto& charVal : panel["characters"].toArray()) {
        QJsonObject charObj = charVal.toObject();
        QString charName = charObj["name"].toString();
        panelCharNames.append(charName);
        
        QString matchedName = charName;
        if (!characterRefs.contains(charName)) {
            QString normalized = normalizeCharacterNameLocal(charName);
            if (characterRefs.contains(normalized)) {
                matchedName = normalized;
            }
        }
        
        if (characterRefs.contains(matchedName)) {
            QJsonObject fullChar = characterRefs.value(matchedName);
            QJsonObject appearance = fullChar["appearance"].toObject();
            
            QStringList portraitPaths = normalizeList(fullChar["portraitPaths"].toArray());
            
            QString charDesc = formatCharacterDescriptor(charName, charObj["pose"].toString(), charObj["expression"].toString());
            QString appearanceDesc = buildCharacterAppearance(appearance);
            
            if (!charDesc.isEmpty() && !appearanceDesc.isEmpty()) {
                charDesc += ", " + appearanceDesc;
            } else if (!appearanceDesc.isEmpty()) {
                charDesc = appearanceDesc;
            }
            
            if (!charDesc.isEmpty()) {
                characterDescriptions.append(charDesc);
            }
            
            QString role = fullChar["role"].toString().trimmed();
            bool isDialogSpeaker = dialogueSpeakers.contains(charName) || dialogueSpeakers.contains(matchedName);
            bool isCoreRole = role.contains(QStringLiteral("protagonist"), Qt::CaseInsensitive)
                || role.contains(QStringLiteral("main"), Qt::CaseInsensitive)
                || role.contains(QStringLiteral("lead"), Qt::CaseInsensitive)
                || role.contains(QStringLiteral("hero"), Qt::CaseInsensitive)
                || role.contains(QString::fromUtf8("\u4e3b\u89d2"), Qt::CaseInsensitive);

            if (primaryCharacterRef.isEmpty() && (isDialogSpeaker || isCoreRole)) {
                primaryCharacterRef = firstNonEmptyPortrait(portraitPaths);
            }
            if (primaryCharacterRef.isEmpty()) {
                primaryCharacterRef = firstNonEmptyPortrait(portraitPaths);
            }
        } else {
            QString charDesc = formatCharacterDescriptor(charName, charObj["pose"].toString(), charObj["expression"].toString());
            if (!charDesc.isEmpty()) {
                characterDescriptions.append(charDesc);
            }
        }
    }
    
    if (!characterDescriptions.isEmpty()) {
        parts.append(QString("character identity lock: %1").arg(characterDescriptions.join("; ")));
    }

    QString selectedRef = primaryCharacterRef;
    QString selectedRefType = "character";
    if (!primarySceneRef.isEmpty() && (promptTarget == "scene" || primaryCharacterRef.isEmpty()) && promptTarget != "character") {
        selectedRef = primarySceneRef;
        selectedRefType = "scene";
    }
    if (selectedRef.isEmpty() && !primarySceneRef.isEmpty()) {
        selectedRef = primarySceneRef;
        selectedRefType = "scene";
    }

    if (selectedRefType == "scene") {
        parts.append("use the scene reference as the environment anchor");
    } else if (!selectedRef.isEmpty()) {
        parts.append("use the character reference as the identity anchor");
    }

    parts << "full-color vibrant palette, no grayscale output";
    parts << (mode == "preview" ? "preview quality" : "high resolution detailed render");
    parts << JAPANESE_MANGA_DIRECTIVE;
    parts << "dynamic lighting, high quality manga aesthetics";
    
    QStringList refUris;
    if (!selectedRef.isEmpty()) {
        refUris.append(selectedRef);
    }
    
    LOG_INFO("PromptBuilder", QString("buildPanelPrompt: panelChars=[%1], charRefs=%2, sceneRef=%3, primaryRef=%4")
        .arg(panelCharNames.join(", "))
        .arg(characterRefs.size())
        .arg(primarySceneRef.isEmpty() ? "(none)" : primarySceneRef.left(30))
        .arg(primaryCharacterRef.isEmpty() ? "(none)" : primaryCharacterRef.left(30)));

    QString prompt = truncatePrompt(parts, MAX_PROMPT_LENGTH);
    return { prompt, DEFAULT_NEGATIVE_PROMPT + PANEL_NEGATIVE_EXTRA, refUris };
}

PromptBuilder::PromptResult PromptBuilder::buildScenePrompt(const QJsonObject &scene,
                                                               const QJsonObject &options)
{
    Q_UNUSED(options)
    QStringList parts;
    
    parts << "environment concept art" << "ultra detailed manga background"
          << "empty scene, no characters"
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
    addIfNotEmpty(parts, sanitizeSceneText(scene["description"].toString()));
    
    appendVisualCharacteristics(parts, scene["visualCharacteristics"].toObject());
    appendSpatialLayout(parts, scene["spatialLayout"].toObject());
    addListIfNotEmpty(parts, normalizeList(scene["anchorPoints"]), "fixed anchors:");
    addListIfNotEmpty(parts, normalizeList(scene["signatureObjects"]), "signature objects:");
    addListIfNotEmpty(parts, normalizeList(scene["fixedColorBlocks"]), "fixed color blocks:");
    addListIfNotEmpty(parts, normalizeList(scene["consistencyRules"]), "consistency rules:");
    QString currentInterpretation = scene["currentInterpretation"].toString().trimmed();
    if (!currentInterpretation.isEmpty()) {
        parts.append(QString("current interpretation: %1").arg(currentInterpretation));
    }
    QString status = scene["status"].toString().trimmed();
    if (!status.isEmpty()) {
        parts.append(QString("status: %1").arg(status));
    }
    QString confidence = scene["confidence"].toString().trimmed();
    if (!confidence.isEmpty()) {
        parts.append(QString("confidence: %1").arg(confidence));
    }
    addListIfNotEmpty(parts, normalizeList(scene["evidence"]), "evidence:");
    addListIfNotEmpty(parts, normalizeList(scene["aliases"]), "aliases:");
    addListIfNotEmpty(parts, normalizeList(scene["history"]), "history:");
    appendVariationDescs(parts, scene["timeVariations"].toArray(), TIME_OF_DAY_MAPPING, "timeOfDay", "time variations");
    appendVariationDescs(parts, scene["weatherVariations"].toArray(), WEATHER_MAPPING, "weather", "weather variations");
    
    addIfNotEmpty(parts, sanitizeSceneText(scene["narrativeRole"].toString()));
    
    parts << "high detail, volumetric lighting, cinematic environment, vibrant colors, full color";
    
    QString prompt = truncatePrompt(parts, MAX_PROMPT_LENGTH);
    return { prompt, DEFAULT_NEGATIVE_PROMPT + SCENE_NEGATIVE_EXTRA, {} };
}
