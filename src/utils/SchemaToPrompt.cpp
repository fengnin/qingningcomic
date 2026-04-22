#include "utils/SchemaToPrompt.h"
#include <QJsonDocument>
#include <QRegularExpression>

QString SchemaToPrompt::sanitizeDescription(const QString &description)
{
    if (description.isEmpty()) {
        return description;
    }

    QString result = description;

    static const QStringList markers = {
        QString::fromUtf8("例如："),
        QString::fromUtf8("例如:"),
        QString::fromUtf8("例如"),
        QString::fromUtf8("示例："),
        QString::fromUtf8("示例:"),
        QString::fromUtf8("示例"),
        QString::fromUtf8("比如："),
        QString::fromUtf8("比如:"),
        QString::fromUtf8("比如"),
        QStringLiteral("Examples:"),
        QStringLiteral("Examples："),
        QStringLiteral("Examples"),
        QStringLiteral("Example:"),
        QStringLiteral("Example："),
        QStringLiteral("Example"),
        QStringLiteral("e.g."),
        QStringLiteral("e.g")
    };

    int cutPos = result.length();
    for (const QString& marker : markers) {
        const int pos = result.indexOf(marker, 0, Qt::CaseInsensitive);
        if (pos >= 0 && pos < cutPos) {
            cutPos = pos;
        }
    }

    if (cutPos < result.length()) {
        result = result.left(cutPos);
    }

    result = result.trimmed();
    result.replace(QRegularExpression(QStringLiteral("\\s{2,}")), QStringLiteral(" "));
    return result;
}

QString SchemaToPrompt::getRoleDescription()
{
    return QString::fromUtf8(
        "你是一个专业的漫画分镜师，擅长将小说文本转换为详细的视觉分镜脚本。\n\n"
    );
}

QString SchemaToPrompt::getContinuityRules()
{
    return QString::fromUtf8(
        "📖 **跨章节连续性规则**（如果提供了现有圣经）：\n\n"
        "- **复用已有角色**：如果用户提供了 existingCharacters，必须在 characters 数组中包含所有现有角色（保持原有属性不变），并添加新出现的角色\n"
        "- **复用已有场景**：如果用户提供了 existingScenes，必须在 scenes 数组中包含所有现有场景（保持原有属性不变），并添加新出现的场景\n"
        "- **引用场景ID**：在 panel.background.sceneId 中优先使用现有场景的 id，确保重复出现的地点使用相同的场景定义\n"
        "- **补全新内容**：遇到新角色或新场景时，按照正常规则创建新条目，但保持与现有风格一致\n"
        "- **禁止修改**：不要修改现有角色的 appearance 或现有场景的 visualCharacteristics，保持视觉连续性\n\n"
    );
}

QString SchemaToPrompt::getCoreTaskRules()
{
    return QString::fromUtf8(
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
        "10. **分页规范**：每页 6 个面板，index 从 0-5\n"
        "11. **场景圣经**：为重复出现的场景创建统一视觉定义，确保跨章节一致性\n\n"
    );
}

QString SchemaToPrompt::getLanguageRequirements()
{
    return QString::fromUtf8(
        "🌐 **语言要求**：\n\n"
        "- 所有文字类字段（包括 panels[].scene、panels[].background / atmosphere、panels[].characters[].pose & expression、dialogue、narrativeFunction、visualPrompt，以及 scenes / characters 梳理出来的描述）必须使用**简体中文**。\n"
        "- Imagen Prompt（visualPrompt）也需要使用简体中文描述画面，不要混入英文提示词。\n"
        "- 除变量名、接口字段名或必要的专有名词外，请避免英文单词或拼音。\n\n"
    );
}

QString SchemaToPrompt::getSchemaIntro()
{
    return QString::fromUtf8(
        "📋 **JSON Schema 字段说明**：\n\n"
        "以下是完整的输出结构定义。所有字段说明都直接来自 JSON Schema，是唯一的事实来源。\n\n"
    );
}

QString SchemaToPrompt::getExampleSection(const QJsonObject &example)
{
    QString section = QString::fromUtf8("\n📝 **完整示例**：\n\n");
    section += QStringLiteral("```json\n");
    section += QString::fromUtf8(QJsonDocument(example).toJson(QJsonDocument::Indented));
    section += QStringLiteral("\n```\n");
    return section;
}

QString SchemaToPrompt::buildSystemPrompt(const QJsonObject &schema, const Options &options)
{
    QString prompt;
    prompt.reserve(4096);
    
    prompt += getRoleDescription();
    prompt += getContinuityRules();
    prompt += getCoreTaskRules();
    prompt += getLanguageRequirements();
    prompt += getSchemaIntro();
    prompt += generateFieldDocs(schema);
    
    QJsonObject example = options.customExample.isEmpty() 
        ? generateExample(schema) 
        : options.customExample;
    prompt += getExampleSection(example);
    
    if (!options.additionalInstructions.isEmpty()) {
        prompt += QStringLiteral("\n") + options.additionalInstructions + QStringLiteral("\n");
    }
    
    return prompt;
}

QString SchemaToPrompt::generateFieldDocs(const QJsonObject &schema, int indentLevel)
{
    QString indent = QStringLiteral("  ").repeated(indentLevel);
    QString docs;
    
    if (schema.isEmpty()) {
        return docs;
    }
    
    QJsonObject properties = schema[QStringLiteral("properties")].toObject();
    QJsonArray required = schema[QStringLiteral("required")].toArray();
    
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
        
        QString requiredMark = isRequired 
            ? QString::fromUtf8("必填") 
            : QString::fromUtf8("可选");
        QString fieldType = getFieldType(fieldSchema);

        docs += QStringLiteral("%1- **%2**（%3）").arg(indent, fieldName, requiredMark);
        
        if (!fieldType.isEmpty()) {
            docs += QStringLiteral(" [%1]").arg(fieldType);
        }
        docs += QStringLiteral("\n");
        
        if (fieldSchema.contains(QStringLiteral("description"))) {
            const QString description = sanitizeDescription(
                fieldSchema[QStringLiteral("description")].toString());
            if (!description.isEmpty()) {
                docs += QStringLiteral("%1  %2\n").arg(indent, description);
            }
        }
        
        if (fieldSchema.contains(QStringLiteral("enum"))) {
            QJsonArray enumValues = fieldSchema[QStringLiteral("enum")].toArray();
            QStringList enumStrings;
            for (const QJsonValue &v : enumValues) {
                enumStrings.append(v.toString());
            }
            docs += QStringLiteral("%1  可选值: %2\n").arg(indent, enumStrings.join(QStringLiteral(", ")));
        }
        
        QStringList constraints = getFieldConstraints(fieldSchema);
        if (!constraints.isEmpty()) {
            docs += QStringLiteral("%1  约束: %2\n").arg(indent, constraints.join(QStringLiteral(", ")));
        }
        
        QString type = fieldSchema[QStringLiteral("type")].toString();
        if (type == QStringLiteral("object") && fieldSchema.contains(QStringLiteral("properties"))) {
            docs += generateFieldDocs(fieldSchema, indentLevel + 1);
        }
        
        if (type == QStringLiteral("array") && fieldSchema.contains(QStringLiteral("items"))) {
            QJsonObject items = fieldSchema[QStringLiteral("items")].toObject();
            if (items[QStringLiteral("type")].toString() == QStringLiteral("object") 
                && items.contains(QStringLiteral("properties"))) {
                docs += QStringLiteral("%1  数组元素结构:\n").arg(indent);
                docs += generateFieldDocs(items, indentLevel + 2);
            } else if (items.contains(QStringLiteral("type"))) {
                docs += QStringLiteral("%1  数组元素类型: %2\n")
                    .arg(indent, items[QStringLiteral("type")].toString());
            }
        }
        
        docs += QStringLiteral("\n");
    }
    
    return docs;
}

QJsonObject SchemaToPrompt::generateExample(const QJsonObject &schema)
{
    if (schema.isEmpty()) {
        return QJsonObject();
    }
    
    if (schema[QStringLiteral("type")].toString() == QStringLiteral("object") 
        && schema.contains(QStringLiteral("properties"))) {
        QJsonObject example;
        QJsonObject properties = schema[QStringLiteral("properties")].toObject();
        
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
    if (fieldSchema.contains(QStringLiteral("example"))) {
        return fieldSchema[QStringLiteral("example")];
    }
    
    QString type = fieldSchema[QStringLiteral("type")].toString();
    
    if (type == QStringLiteral("string")) {
        if (fieldSchema.contains(QStringLiteral("enum"))) {
            QJsonArray enumValues = fieldSchema[QStringLiteral("enum")].toArray();
            if (!enumValues.isEmpty()) {
                return enumValues.first();
            }
        }
        if (fieldSchema.contains(QStringLiteral("description"))) {
            return sanitizeDescription(fieldSchema[QStringLiteral("description")].toString());
        }
        return QStringLiteral("string value");
    }
    
    if (type == QStringLiteral("number") || type == QStringLiteral("integer")) {
        if (fieldSchema.contains(QStringLiteral("minimum"))) {
            return fieldSchema[QStringLiteral("minimum")].toInt();
        }
        return 0;
    }
    
    if (type == QStringLiteral("boolean")) {
        return true;
    }
    
    if (type == QStringLiteral("array")) {
        if (fieldSchema.contains(QStringLiteral("items"))) {
            QJsonArray arr;
            arr.append(generateFieldExample(fieldSchema[QStringLiteral("items")].toObject()));
            return arr;
        }
        return QJsonArray();
    }
    
    if (type == QStringLiteral("object") && fieldSchema.contains(QStringLiteral("properties"))) {
        QJsonObject obj;
        QJsonObject properties = fieldSchema[QStringLiteral("properties")].toObject();
        for (auto it = properties.begin(); it != properties.end(); ++it) {
            obj[it.key()] = generateFieldExample(it.value().toObject());
        }
        return obj;
    }
    
    return QJsonValue();
}

QString SchemaToPrompt::getFieldType(const QJsonObject &fieldSchema)
{
    if (fieldSchema.contains(QStringLiteral("type"))) {
        return fieldSchema[QStringLiteral("type")].toString();
    }
    
    if (fieldSchema.contains(QStringLiteral("oneOf"))) {
        QJsonArray oneOf = fieldSchema[QStringLiteral("oneOf")].toArray();
        QStringList types;
        for (const QJsonValue &v : oneOf) {
            if (v.isObject() && v.toObject().contains(QStringLiteral("type"))) {
                types.append(v.toObject()[QStringLiteral("type")].toString());
            }
        }
        return types.join(QStringLiteral(" | "));
    }
    
    return QString();
}

QStringList SchemaToPrompt::getFieldConstraints(const QJsonObject &fieldSchema)
{
    static const QList<std::pair<QString, QString>> constraintKeys = {
        {QStringLiteral("minLength"), QString::fromUtf8("最小长度")},
        {QStringLiteral("maxLength"), QString::fromUtf8("最大长度")},
        {QStringLiteral("minimum"), QString::fromUtf8("最小值")},
        {QStringLiteral("maximum"), QString::fromUtf8("最大值")},
        {QStringLiteral("minItems"), QString::fromUtf8("最少元素")},
        {QStringLiteral("maxItems"), QString::fromUtf8("最多元素")}
    };
    
    QStringList constraints;
    for (const auto& pair : constraintKeys) {
        if (fieldSchema.contains(pair.first)) {
            constraints.append(QStringLiteral("%1: %2")
                .arg(pair.second)
                .arg(fieldSchema[pair.first].toInt()));
        }
    }
    
    return constraints;
}
