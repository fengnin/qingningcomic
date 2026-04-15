#include "api/QwenPromptBuilder.h"
#include "SchemaToPrompt.h"
#include <QFile>
#include <QJsonDocument>
#include <QCoreApplication>
#include <QDir>

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
    
    userMessage = QString::fromUtf8("\u4ee5\u4e0b\u662f\u7b2c%1\u7ae0\u7684\u8bbe\u5b9a\u8d44\u6599\uff1a\n\n").arg(chapterNumber > 0 ? QString::number(chapterNumber) : "?");
    
    if (!existingCharacters.isEmpty()) {
        userMessage += QString::fromUtf8("\u3010\u89d2\u8272\u8bbe\u5b9a\u3011\n");
        userMessage += QString::fromUtf8(QJsonDocument(existingCharacters).toJson(QJsonDocument::Indented));
        userMessage += "\n\n";
    }
    
    if (!existingScenes.isEmpty()) {
        userMessage += QString::fromUtf8("\u3010\u573a\u666f\u8bbe\u5b9a\u3011\n");
        userMessage += QString::fromUtf8(QJsonDocument(existingScenes).toJson(QJsonDocument::Indented));
        userMessage += "\n\n";
    }
    
    userMessage += QString::fromUtf8("\u3010\u6b63\u6587\u5185\u5bb9\u3011\n") + text;
    
    return userMessage;
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

QJsonObject QwenPromptBuilder::buildExamplePanel()
{
    QJsonObject panel;
    panel["page"] = 1;
    panel["index"] = 0;
    panel["scene"] = QString::fromUtf8("\u6821\u56ed\u8d70\u5eca");

    QJsonObject background;
    background["sceneId"] = QString::fromUtf8("scene_001");
    background["setting"] = QString::fromUtf8("\u5ba4\u5185");
    background["timeOfDay"] = QString::fromUtf8("\u5348\u540e");
    background["weather"] = QString::fromUtf8("\u6674\u6717");
    background["lighting"] = QString::fromUtf8("\u81ea\u7136\u5149");
    background["details"] = QJsonArray{QString::fromUtf8("\u6728\u8d28\u5730\u677f"), QString::fromUtf8("\u7a97\u6237\u900f\u5149"), QString::fromUtf8("\u516c\u544a\u680f")};
    panel["background"] = background;

    QJsonObject atmosphere;
    atmosphere["mood"] = QString::fromUtf8("\u5b81\u9759\u6e29\u99a8");
    atmosphere["soundEffects"] = QJsonArray{QJsonObject{{"sound", QString::fromUtf8("\u811a\u6b65\u58f0")}, {"style", QString::fromUtf8("\u8f7b\u5fae")}}};
    atmosphere["particleEffects"] = QJsonArray{QString::fromUtf8("\u5c18\u57c3\u6f02\u6d6e"), QString::fromUtf8("\u9633\u5149\u65d1\u9a7b")};
    panel["atmosphere"] = atmosphere;

    panel["shotType"] = QString::fromUtf8("\u4e2d\u666f");
    panel["cameraAngle"] = QString::fromUtf8("\u5e73\u89c6");

    QJsonObject composition;
    composition["focusPoint"] = QString::fromUtf8("\u4eba\u7269\u4e2d\u5fc3");
    composition["depthOfField"] = QString::fromUtf8("\u6d45\u666f\u6df1");
    composition["rule"] = QString::fromUtf8("\u4e09\u5206\u6cd5");
    panel["composition"] = composition;

    QJsonObject artStyle;
    artStyle["genre"] = QString::fromUtf8("\u65e5\u7cfb\u6f2b\u753b");
    artStyle["lineWeight"] = QString::fromUtf8("\u7ec6\u7ebf\u6761");
    artStyle["shading"] = QString::fromUtf8("\u8d5b\u749f\u73af\u98ce\u683c");
    artStyle["colorPalette"] = QString::fromUtf8("\u6696\u8272\u8c03");
    panel["artStyle"] = artStyle;

    panel["characters"] = QJsonArray{QJsonObject{
        {"name", QString::fromUtf8("\u6797\u5c0f\u96e8")},
        {"pose", QString::fromUtf8("\u7ad9\u7acb\uff0c\u53cc\u624b\u4ea4\u53e0")},
        {"expression", QString::fromUtf8("\u5fae\u7b11")},
        {"position", QString::fromUtf8("\u753b\u9762\u4e2d\u592e\u504f\u53f3")}
    }};
    panel["dialogue"] = QJsonArray();
    panel["visualPrompt"] = QString::fromUtf8("\u4e00\u4f4d\u5c11\u5973\u7ad9\u5728\u9633\u5149\u6d12\u843d\u7684\u6821\u56ed\u8d70\u5eca\u4e2d\uff0c\u88ab\u7a97\u5916\u6296\u52a8\u7684\u6811\u5f71\u6620\u7167\u7740\u3002");
    panel["visualPromptEn"] = QString::fromUtf8("A girl standing in a sunlit school corridor, with tree shadows dancing outside windows.");
    panel["narrativeFunction"] = QString::fromUtf8("\u5f00\u573a\u5f15\u5165");
    
    return panel;
}

QJsonObject QwenPromptBuilder::buildExampleCharacter()
{
    QJsonObject character;
    character["name"] = QString::fromUtf8("\u6797\u5c0f\u96e8");
    character["role"] = QString::fromUtf8("\u4e3b\u89d2");

    QJsonObject appearance;
    appearance["gender"] = QString::fromUtf8("\u5973");
    appearance["age"] = 16;
    appearance["hairColor"] = QString::fromUtf8("\u9ed1\u8272");
    appearance["hairStyle"] = QString::fromUtf8("\u957f\u53d1\u53ca\u8170");
    appearance["eyeColor"] = QString::fromUtf8("\u68d5\u8272");
    appearance["height"] = QString::fromUtf8("165cm");
    appearance["build"] = QString::fromUtf8("\u82d7\u6761");
    appearance["clothing"] = QJsonArray{QString::fromUtf8("\u6821\u670d\u88d9\u88c5"), QString::fromUtf8("\u767d\u8272\u886c\u886b")};
    appearance["distinctiveFeatures"] = QJsonArray{QString::fromUtf8("\u53d1\u672b\u5fae\u5377")};
    character["appearance"] = appearance;
    character["personality"] = QJsonArray{QString::fromUtf8("\u6e29\u67d4"), QString::fromUtf8("\u52c7\u6562"), QString::fromUtf8("\u5584\u826f")};
    
    return character;
}

QJsonObject QwenPromptBuilder::buildExampleScene()
{
    QJsonObject scene;
    scene["id"] = QString::fromUtf8("scene_001");
    scene["name"] = QString::fromUtf8("\u9752\u4e91\u4e2d\u5b66\u00b7\u4e3b\u6559\u5b66\u697c\u8d70\u5eca");
    scene["type"] = QString::fromUtf8("\u5ba4\u5185");
    scene["description"] = QString::fromUtf8("\u4e00\u6761\u5bbd\u655e\u660e\u4eae\u7684\u8d70\u5eca\uff0c\u4e24\u4fa7\u662f\u6559\u5ba4\u95e8\u548c\u516c\u544a\u680f\uff0c9633\u5149\u4ece\u7a97\u623b\u6d12\u8fdb\u6765\uff0c\u5730\u9762\u662f\u5149\u6ed1\u7684\u7817\u787e\u5730\u677f\u3002");

    QJsonObject visualChars;
    visualChars["architecture"] = QString::fromUtf8("\u73b0\u4ee3\u6559\u5b66\u697c\u98ce\u683c");
    visualChars["keyLandmarks"] = QJsonArray{QString::fromUtf8("\u697c\u68af\u53e3"), QString::fromUtf8("\u516c\u544a\u680f"), QString::fromUtf8("\u996e\u6c34\u673a")};
    visualChars["colorScheme"] = QString::fromUtf8("\u7c73\u767d\u4e0e\u6d45\u6728\u8272");

    QJsonObject lighting;
    lighting["naturalLight"] = QString::fromUtf8("\u5348\u540e\u659c\u9633");
    lighting["artificialLight"] = QString::fromUtf8("\u65e5\u5149\u706f\u7167\u660e");
    lighting["lightSources"] = QJsonArray{QString::fromUtf8("\u7a97\u6237\u9633\u5149"), QString::fromUtf8("\u5929\u82b1\u677f\u706f\u7ba1"), QString::fromUtf8("\u5e94\u6025\u51fa\u53e3\u6307\u793a\u706f")};
    visualChars["lighting"] = lighting;

    visualChars["atmosphere"] = QString::fromUtf8("\u5b89\u9759\u7965\u548c");
    visualChars["soundscape"] = QJsonArray{QString::fromUtf8("\u8fdc\u5904\u7684\u8bfb\u4e66\u58f0"), QString::fromUtf8("\u5076\u5c14\u7684\u811a\u6b65\u58f0"), QString::fromUtf8("\u7a97\u5916\u9e1f\u9e23"), QString::fromUtf8("\u7a7a\u8c03\u8fd0\u8f6c\u58f0")};
    visualChars["textures"] = QJsonArray{QString::fromUtf8("\u5149\u6ed1\u74f7\u7801\u5730\u9762"), QString::fromUtf8("\u78e8\u7801\u73bb\u7483\u7a97"), QString::fromUtf8("\u6728\u8d28\u6276\u624b")};
    scene["visualCharacteristics"] = visualChars;

    QJsonObject spatialLayout;
    spatialLayout["size"] = QString::fromUtf8("\u957f\u7ea630\u7c73\uff0c\u5bbd\u7ea64\u7c73");
    spatialLayout["layout"] = QString::fromUtf8("\u76f4\u7ebf\u578b\u8d70\u5eca\uff0c\u4e24\u4fa7\u5bf9\u79f0\u5206\u5e03\u6559\u5ba4");
    spatialLayout["keyAreas"] = QJsonArray{
        QJsonObject{{"name", QString::fromUtf8("\u697c\u68af\u53e3")}, {"position", QString::fromUtf8("\u8d70\u5eca\u5de7\u7aef")}},
        QJsonObject{{"name", QString::fromUtf8("\u516c\u544a\u680f")}, {"position", QString::fromUtf8("\u53f3\u4fa7\u5899\u58c1")}},
        QJsonObject{{"name", QString::fromUtf8("\u996e\u6c34\u673a")}, {"position", QString::fromUtf8("\u5de6\u4fa7\u5899\u89d2")}}
    };
    scene["spatialLayout"] = spatialLayout;

    scene["timeVariations"] = QJsonArray{
        QJsonObject{
            {"timeOfDay", QString::fromUtf8("\u65e9\u6668")},
            {"description", QString::fromUtf8("\u65e9\u6668\u7684\u9633\u5149\u900f\u8fc7\u7a97\u6237\u6295\u5c04\u5728\u8d70\u5eca\u4e0a\uff0c\u5b66\u751f\u4eec\u52b2\u52c8\u5730\u8d70\u8fc7\u3002")}
        },
        QJsonObject{
            {"timeOfDay", QString::fromUtf8("\u508d\u665a")},
            {"description", QString::fromUtf8("\u5915\u9633\u897f\u4e0b\uff0c\u8d70\u5eca\u88ab\u67d3\u6210\u91d1\u8272\uff0c\u5bd2\u51b7\u7684\u706f\u5149\u5f00\u59cb\u4eae\u8d77\u3002")}
        }
    };

    scene["weatherVariations"] = QJsonArray{
        QJsonObject{
            {"weather", QString::fromUtf8("\u96e8\u5929")},
            {"description", QString::fromUtf8("\u96e8\u6ef4\u4ece\u7a97\u5916\u6252\u5728\u73bb\u7483\u4e0a\uff0c\u5b66\u751f\u4eec\u6491\u4f1e\u7a7f\u884c\u3002")}
        }
    };

    scene["narrativeRole"] = QString::fromUtf8("\u4e3b\u8981\u6d3b\u52a8\u573a\u6240");
    
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

