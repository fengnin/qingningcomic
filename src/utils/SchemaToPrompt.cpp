#include "SchemaToPrompt.h"
#include <QJsonDocument>
#include <QStringList>

QString SchemaToPrompt::buildSystemPrompt(const QJsonObject &schema, const Options &options)
{
    QString prompt = QString::fromUtf8(
        "你是一个专业的漫画分镜师，擅长将小说文本转换为详细的视觉分镜脚本。\n\n"
        "📖 **跨章节连续性规则**（如果提供了现有圣经）：\n\n"
        "- **复用已有角色**：如果用户提供了 existingCharacters，必须在 characters 数组中包含所有现有角色（保持原有属性不变），并添加新出现的角色\n"
        "- **复用已有场景**：如果用户提供了 existingScenes，必须在 scenes 数组中包含所有现有场景（保持原有属性不变），并添加新出现的场景\n"
        "- **引用场景ID**：在 panel.background.sceneId 中优先使用现有场景的 id，确保重复出现的地点使用相同的场景定义\n"
        "- **补全新内容**：遇到新角色或新场景时，按照正常规则创建新条目，但保持与现有风格一致\n"
        "- **禁止修改**：不要修改现有角色的 appearance 或现有场景的 visualCharacteristics，保持视觉连续性\n\n"
        "🎬 **核心任务规则**：\n\n"
        "1. **场景描写**：详细描述每个面板的视觉画面，包括环境、氛围、人物动作\n"
        "2. **背景设定**：明确场景地点、时间、天气、光照\n"
        "3. **镜头设计**：选择合适的景别和机位强化叙事\n"
        "4. **氛围营造**：定义情绪基调、音效、粒子效果\n"
        "5. **画风控制**：指定艺术风格包括类型、线条、阴影、配色\n"
        "6. **构图原则**：运用构图法则确定焦点、景深、视觉引导\n"
        "7. **角色刻画**：准确描述姿态、表情、位置\n"
        "8. **对白处理**：提取对话，标注说话者、气泡类型、情感\n"
        "9. **叙事功能**：明确面板的故事作用\n"
        "10. **分页规范**：每个面板的 index 从 0-5 循环（每页最多 6 个面板），page 自动递增。根据小说内容需要，可以生成任意数量的面板，不要人为限制数量。\n"
        "11. **场景圣经**：为重复出现的场景创建统一视觉定义，确保跨章节一致性\n\n"
        "🌐 **语言要求**：\n\n"
        "- 所有文字类字段必须使用**简体中文**\n"
        "- Imagen Prompt 也需要使用简体中文描述画面\n"
        "- 除变量名、接口字段名或必要的专有名词外，请避免英文单词或拼音\n\n"
        "📋 **JSON Schema 字段说明**：\n\n"
        "以下是完整的输出结构定义。所有字段说明都直接来自 JSON Schema，是唯一的事实来源。\n\n"
    );
    
    prompt += generateFieldDocs(schema);
    
    prompt += "\n📝 **完整示例**：\n\n";
    
    QJsonObject example = options.customExample.isEmpty() ? generateExample(schema) : options.customExample;
    prompt += "```json\n";
    prompt += QString::fromUtf8(QJsonDocument(example).toJson(QJsonDocument::Indented));
    prompt += "\n```\n";
    
    if (!options.additionalInstructions.isEmpty()) {
        prompt += "\n" + options.additionalInstructions + "\n";
    }
    
    return prompt;
}

QString SchemaToPrompt::generateFieldDocs(const QJsonObject &schema, int indentLevel)
{
    QString indent = QString("  ").repeated(indentLevel);
    QString docs;
    
    if (schema.isEmpty()) {
        return docs;
    }
    
    QJsonObject properties = schema["properties"].toObject();
    QJsonArray required = schema["required"].toArray();
    
    for (auto it = properties.begin(); it != properties.end(); ++it) {
        QString fieldName = it.key();
        QJsonObject fieldSchema = it.value().toObject();
        
        bool isRequired = false;
        for (const QJsonValue &req : required) {
            if (req.toString() == fieldName) {
                isRequired = true;
                break;
            }
        }
        
        QString requiredMark = isRequired ? QString::fromUtf8("**必填**") : QString::fromUtf8("可选");
        QString fieldType = getFieldType(fieldSchema);
        
        docs += QString("%1- **%2** (%3)").arg(indent, fieldName, requiredMark);
        
        if (!fieldType.isEmpty()) {
            docs += QString(" [%1]").arg(fieldType);
        }
        docs += "\n";
        
        if (fieldSchema.contains("description")) {
            docs += QString("%1  %2\n").arg(indent, fieldSchema["description"].toString());
        }
        
        if (fieldSchema.contains("enum")) {
            QJsonArray enumValues = fieldSchema["enum"].toArray();
            QStringList enumStrings;
            for (const QJsonValue &v : enumValues) {
                enumStrings.append(v.toString());
            }
            docs += QString("%1  %2: %3\n").arg(indent, QString::fromUtf8("可选值"), enumStrings.join(", "));
        }
        
        QStringList constraints = getFieldConstraints(fieldSchema);
        if (!constraints.isEmpty()) {
            docs += QString("%1  %2: %3\n").arg(indent, QString::fromUtf8("约束"), constraints.join(", "));
        }
        
        if (fieldSchema["type"].toString() == "object" && fieldSchema.contains("properties")) {
            docs += generateFieldDocs(fieldSchema, indentLevel + 1);
        }
        
        if (fieldSchema["type"].toString() == "array" && fieldSchema.contains("items")) {
            QJsonObject items = fieldSchema["items"].toObject();
            if (items["type"].toString() == "object" && items.contains("properties")) {
                docs += QString("%1  %2:\n").arg(indent, QString::fromUtf8("数组元素结构"));
                docs += generateFieldDocs(items, indentLevel + 2);
            } else if (items.contains("type")) {
                docs += QString("%1  %2: %3\n").arg(indent, QString::fromUtf8("数组元素类型"), items["type"].toString());
            }
        }
        
        docs += "\n";
    }
    
    return docs;
}

QJsonObject SchemaToPrompt::generateExample(const QJsonObject &schema)
{
    if (schema.isEmpty()) {
        return QJsonObject();
    }
    
    if (schema["type"].toString() == "object" && schema.contains("properties")) {
        QJsonObject example;
        QJsonObject properties = schema["properties"].toObject();
        
        for (auto it = properties.begin(); it != properties.end(); ++it) {
            example[it.key()] = generateFieldExample(it.value().toObject());
        }
        
        return example;
    }
    
    QJsonValue fieldValue = generateFieldExample(schema);
    if (fieldValue.isObject()) {
        return fieldValue.toObject();
    }
    return QJsonObject();
}

QJsonValue SchemaToPrompt::generateFieldExample(const QJsonObject &fieldSchema)
{
    if (fieldSchema.contains("example")) {
        return fieldSchema["example"];
    }
    
    QString type = fieldSchema["type"].toString();
    
    if (type == "string") {
        if (fieldSchema.contains("enum")) {
            QJsonArray enumValues = fieldSchema["enum"].toArray();
            if (!enumValues.isEmpty()) {
                return enumValues.first();
            }
        }
        if (fieldSchema.contains("description")) {
            return fieldSchema["description"].toString();
        }
        return QString("string value");
    }
    
    if (type == "number" || type == "integer") {
        if (fieldSchema.contains("minimum")) {
            return fieldSchema["minimum"].toInt();
        }
        return 0;
    }
    
    if (type == "boolean") {
        return true;
    }
    
    if (type == "array") {
        if (fieldSchema.contains("items")) {
            QJsonArray arr;
            arr.append(generateFieldExample(fieldSchema["items"].toObject()));
            return arr;
        }
        return QJsonArray();
    }
    
    if (type == "object" && fieldSchema.contains("properties")) {
        QJsonObject obj;
        QJsonObject properties = fieldSchema["properties"].toObject();
        for (auto it = properties.begin(); it != properties.end(); ++it) {
            obj[it.key()] = generateFieldExample(it.value().toObject());
        }
        return obj;
    }
    
    return QJsonValue();
}

QString SchemaToPrompt::getFieldType(const QJsonObject &fieldSchema)
{
    if (fieldSchema.contains("type")) {
        return fieldSchema["type"].toString();
    }
    
    if (fieldSchema.contains("oneOf")) {
        QJsonArray oneOf = fieldSchema["oneOf"].toArray();
        QStringList types;
        for (const QJsonValue &v : oneOf) {
            if (v.isObject() && v.toObject().contains("type")) {
                types.append(v.toObject()["type"].toString());
            }
        }
        return types.join(" | ");
    }
    
    return QString();
}

QStringList SchemaToPrompt::getFieldConstraints(const QJsonObject &fieldSchema)
{
    static const QList<std::pair<QString, QString>> constraintKeys = {
        {"minLength", QString::fromUtf8("最小长度")},
        {"maxLength", QString::fromUtf8("最大长度")},
        {"minimum", QString::fromUtf8("最小值")},
        {"maximum", QString::fromUtf8("最大值")},
        {"minItems", QString::fromUtf8("最少元素")},
        {"maxItems", QString::fromUtf8("最多元素")}
    };
    
    QStringList constraints;
    for (const auto& pair : constraintKeys) {
        if (fieldSchema.contains(pair.first)) {
            constraints.append(QString("%1: %2").arg(pair.second).arg(fieldSchema[pair.first].toInt()));
        }
    }
    
    return constraints;
}
