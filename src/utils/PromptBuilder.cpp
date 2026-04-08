#include "PromptBuilder.h"

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
                          "dark background, black border, frame, shadow on background");
    
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
}

QStringList PromptBuilder::normaliseList(const QJsonValue &value)
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

QString PromptBuilder::buildHairDescription(const QJsonObject &appearance)
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
        QString poseText = POSE_MAPPING.value(pose, QString("%1 pose").arg(pose));
        segments.append(poseText);
    }
    if (!expression.isEmpty()) {
        QString exprText = EXPRESSION_MAPPING.value(expression, QString("%1 expression").arg(expression));
        segments.append(exprText);
    }
    return segments.isEmpty() ? QString() : segments.join(", ");
}

namespace {
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
    
    QString formatHair(const QString& hairColor, const QString& hairStyle) {
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
}

PromptBuilder::PromptResult PromptBuilder::buildCharacterPrompt(const QJsonObject &character,
                                                                  const QJsonObject &options)
{
    QString name = character["name"].toString();
    QString role = character["role"].toString();
    QJsonObject appearance = character["appearance"].toObject();
    QStringList tags = normaliseList(character["tags"]);
    
    QString view = options.value("view").toString("front");
    QString pose = options.value("pose").toString("standing");
    QString style = options.value("style").toString("manga");
    QString mode = options.value("mode").toString("preview");
    
    QStringList parts;
    parts << "ultra detailed manga character portrait" << JAPANESE_MANGA_DIRECTIVE;
    
    parts.append(VIEW_MAPPING.value(view, QString("%1 view").arg(view)));
    parts.append(POSE_MAPPING.value(pose, QString("%1 pose").arg(pose)));
    parts.append(STYLE_MAPPING.value(style, style));
    parts.append(mode == "hd" ? "high resolution render" : "illustrated render");
    
    addIfNotEmpty(parts, role.isEmpty() ? QString() : QString("%1 archetype").arg(role));
    addIfNotEmpty(parts, name);
    addIfContains(parts, appearance, "gender");
    addIfContains(parts, appearance, "age", "age %1");
    addIfContains(parts, appearance, "build");
    
    addIfNotEmpty(parts, buildHairDescription(appearance));
    addIfContains(parts, appearance, "eyeColor", "%1 eyes");
    
    addListIfNotEmpty(parts, normaliseList(appearance["clothing"]), "wearing");
    addListIfNotEmpty(parts, normaliseList(appearance["accessories"]), "accessories");
    addListIfNotEmpty(parts, normaliseList(appearance["distinctiveFeatures"]), "distinctive features");
    addListIfNotEmpty(parts, tags, "style keywords:");
    
    parts << "studio lighting" << "solid white background, plain background, no gradient, no shadow on background" 
         << "line art with screentone shading" << "vibrant colors" << JAPANESE_MANGA_DIRECTIVE;
    
    return { parts.join(", "), DEFAULT_NEGATIVE_PROMPT + ", monochrome, grayscale, black and white, split background, dark side", {} };
}

PromptBuilder::PromptResult PromptBuilder::buildPanelPrompt(const QJsonObject &panel,
                                                              const QMap<QString, QJsonObject> &characterRefs,
                                                              const QMap<QString, QJsonObject> &sceneRefs,
                                                              const QJsonObject &options)
{
    QString mode = options.value("mode").toString("preview");
    
    // scene 字段存储的是场景描述文本，不是场景名称
    QString sceneDescription = panel["scene"].toString();
    QString shotType = panel["shotType"].toString();
    QString cameraAngle = panel["cameraAngle"].toString();
    QJsonObject atmosphere = panel["atmosphere"].toObject();
    QJsonObject composition = panel["composition"].toObject();
    QJsonObject artStyle = panel["artStyle"].toObject();
    QJsonArray characters = panel["characters"].toArray();
    QString visualPrompt = panel["visualPrompt"].toString();
    
    // 从 background.setting 获取场景名称，用于匹配场景圣经
    QJsonObject background = panel["background"].toObject();
    QString sceneName = background["setting"].toString();
    QString sceneId = background["sceneId"].toString();
    
    QStringList parts;
    parts << "manga panel illustration" << "full-color vibrant palette, no grayscale output" << JAPANESE_MANGA_DIRECTIVE;
    parts.append(mode == "hd" ? "high resolution detailed render" : "preview quality");
    
    // 场景参考：从场景圣经获取详细信息
    // 优先使用 sceneId 匹配
    if (!sceneId.isEmpty() && sceneRefs.contains(sceneId)) {
        QJsonObject sceneRef = sceneRefs.value(sceneId);
        QJsonObject details = sceneRef["details"].toObject();
        
        // 添加场景圣经中的详细信息
        addIfNotEmpty(parts, details["description"].toString());
        addIfNotEmpty(parts, details["building"].toString());
        addIfNotEmpty(parts, details["color"].toString());
        addIfNotEmpty(parts, details["atmosphere"].toString());
        addIfNotEmpty(parts, details["landmark"].toString());
        addIfNotEmpty(parts, details["layout"].toString());
        addIfNotEmpty(parts, details["timeOfDay"].toString());
        addIfNotEmpty(parts, details["weather"].toString());
    }
    // 如果 sceneId 没有匹配到，尝试使用 sceneName 匹配
    else if (!sceneName.isEmpty() && sceneRefs.contains(sceneName)) {
        QJsonObject sceneRef = sceneRefs.value(sceneName);
        QJsonObject details = sceneRef["details"].toObject();
        
        // 添加场景圣经中的详细信息
        addIfNotEmpty(parts, details["description"].toString());
        addIfNotEmpty(parts, details["building"].toString());
        addIfNotEmpty(parts, details["color"].toString());
        addIfNotEmpty(parts, details["atmosphere"].toString());
        addIfNotEmpty(parts, details["landmark"].toString());
        addIfNotEmpty(parts, details["layout"].toString());
        addIfNotEmpty(parts, details["timeOfDay"].toString());
        addIfNotEmpty(parts, details["weather"].toString());
    }
    
    // 无论是否匹配到场景圣经，都添加面板中的场景描述
    addIfNotEmpty(parts, sceneDescription);
    
    addIfNotEmpty(parts, shotType.isEmpty() ? QString() : QString("%1 shot").arg(shotType));
    addIfNotEmpty(parts, cameraAngle.isEmpty() ? QString() : QString("camera angle %1").arg(cameraAngle));
    addIfContains(parts, atmosphere, "mood", "mood %1");
    
    if (!composition.isEmpty()) {
        addIfContains(parts, composition, "focusPoint", "focus on %1");
        addIfContains(parts, composition, "rule", "%1 composition");
        addIfContains(parts, composition, "depthOfField", "%1 depth of field");
    }
    
    if (!artStyle.isEmpty()) {
        addIfContains(parts, artStyle, "genre");
        addIfContains(parts, artStyle, "lineWeight", "%1 line weight");
        addIfContains(parts, artStyle, "shading", "%1 shading");
        addIfContains(parts, artStyle, "colorPalette", "%1 palette");
    }
    
    QStringList refUris;
    
    // 添加场景参考图（优先使用 sceneId，其次使用 sceneName）
    if (!sceneId.isEmpty() && sceneRefs.contains(sceneId)) {
        QJsonObject sceneRef = sceneRefs.value(sceneId);
        QString sceneRefPath = sceneRef["referenceImagePath"].toString();
        if (!sceneRefPath.isEmpty()) {
            refUris.append(sceneRefPath);
        }
    } else if (!sceneName.isEmpty() && sceneRefs.contains(sceneName)) {
        QJsonObject sceneRef = sceneRefs.value(sceneName);
        QString sceneRefPath = sceneRef["referenceImagePath"].toString();
        if (!sceneRefPath.isEmpty()) {
            refUris.append(sceneRefPath);
        }
    }
    
    // 添加角色参考图和角色外观描述
    for (const auto &charVal : characters) {
        QJsonObject charObj = charVal.toObject();
        QString charName = charObj["name"].toString();
        
        // 基本角色描述（名称、姿势、表情）
        addIfNotEmpty(parts, formatCharacterDescriptor(charName, charObj["pose"].toString(), charObj["expression"].toString()));
        
        // 从角色圣经获取外观信息并添加到 prompt
        if (characterRefs.contains(charName)) {
            QJsonObject fullChar = characterRefs.value(charName);
            
            // 添加角色外观描述
            QJsonObject appearance = fullChar["appearance"].toObject();
            if (!appearance.isEmpty()) {
                // 性别和年龄
                QString gender = appearance["gender"].toString();
                QString age = appearance["age"].toString();
                if (!gender.isEmpty()) {
                    addIfNotEmpty(parts, gender);
                }
                if (!age.isEmpty() && age != "0") {
                    addIfNotEmpty(parts, QString("%1 years old").arg(age));
                }
                
                // 体型
                addIfNotEmpty(parts, appearance["build"].toString());
                
                // 头发
                QString hairColor = appearance["hairColor"].toString();
                QString hairStyle = appearance["hairStyle"].toString();
                if (!hairColor.isEmpty() || !hairStyle.isEmpty()) {
                    addIfNotEmpty(parts, formatHair(hairColor, hairStyle));
                }
                
                // 眼睛颜色
                QString eyeColor = appearance["eyeColor"].toString();
                if (!eyeColor.isEmpty()) {
                    addIfNotEmpty(parts, QString("%1 eyes").arg(eyeColor));
                }
                
                // 服装
                QJsonArray clothingArr = appearance["clothing"].toArray();
                QStringList clothing;
                for (const auto& c : clothingArr) {
                    QString item = c.toString();
                    if (!item.isEmpty()) {
                        clothing.append(item);
                    }
                }
                addListIfNotEmpty(parts, clothing, "wearing");
                
                // 显著特征
                QJsonArray featuresArr = appearance["distinctiveFeatures"].toArray();
                QStringList features;
                for (const auto& f : featuresArr) {
                    QString feature = f.toString();
                    if (!feature.isEmpty()) {
                        features.append(feature);
                    }
                }
                addListIfNotEmpty(parts, features, "distinctive features");
            }
            
            // 获取角色参考图片
            QStringList portraitPaths = normaliseList(fullChar["portraitPaths"].toArray());
            for (const QString &path : portraitPaths) {
                if (!path.isEmpty()) {
                    refUris.append(path);
                }
            }
        }
    }
    
    addIfNotEmpty(parts, visualPrompt);
    
    parts << "dynamic lighting" << "high quality manga aesthetics" << JAPANESE_MANGA_DIRECTIVE;
    
    return { parts.join(", "), DEFAULT_NEGATIVE_PROMPT + ", monochrome, grayscale, black and white", refUris };
}

PromptBuilder::PromptResult PromptBuilder::buildScenePrompt(const QJsonObject &scene)
{
    QStringList parts;
    parts << "environment concept art" << "ultra detailed manga background" << "empty scene, no characters" << JAPANESE_MANGA_DIRECTIVE;
    
    QString name = scene["name"].toString();
    QString description = scene["description"].toString();
    
    if (!name.isEmpty()) {
        parts.append(name);
    }
    if (!description.isEmpty()) {
        parts.append(description);
    }
    
    QJsonObject visual = scene["visualCharacteristics"].toObject();
    if (!visual.isEmpty()) {
        QString architecture = visual["architecture"].toString();
        if (!architecture.isEmpty()) {
            parts.append(architecture);
        }
        
        QStringList landmarks = normaliseList(visual["keyLandmarks"]);
        if (!landmarks.isEmpty()) {
            parts.append(QString("landmarks: %1").arg(landmarks.join(", ")));
        }
        
        QString colorScheme = visual["colorScheme"].toString();
        if (!colorScheme.isEmpty()) {
            parts.append(QString("color palette %1").arg(colorScheme));
        }
        
        QJsonObject lighting = visual["lighting"].toObject();
        if (!lighting.isEmpty()) {
            QString naturalLight = lighting["naturalLight"].toString();
            QString artificialLight = lighting["artificialLight"].toString();
            if (!naturalLight.isEmpty()) {
                parts.append(QString("natural light %1").arg(naturalLight));
            }
            if (!artificialLight.isEmpty()) {
                parts.append(QString("artificial light %1").arg(artificialLight));
            }
        }
        
        QString atmosphere = visual["atmosphere"].toString();
        if (!atmosphere.isEmpty()) {
            parts.append(atmosphere);
        }
    }
    
    QString narrativeRole = scene["narrativeRole"].toString();
    if (!narrativeRole.isEmpty()) {
        parts.append(narrativeRole);
    }
    
    parts << "high detail, volumetric lighting, cinematic environment" << "vibrant colors, full color" << JAPANESE_MANGA_DIRECTIVE;
    
    // 场景圣经不应该出现人物
    QString sceneNegative = DEFAULT_NEGATIVE_PROMPT + ", monochrome, grayscale, black and white, "
        "no people, no characters, no humans, no figures, no person, no human, "
        "empty scene, uninhabited, no faces, no bodies";
    
    return { parts.join(", "), sceneNegative, {} };
}
