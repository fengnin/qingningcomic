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

QString SchemaToPrompt::buildSystemPrompt(const QJsonObject &schema, const Options &options)
{
    QString prompt = QString::fromUtf8(
        "你是一个专业的漫画分镜师，擅长将小说文本转换为详细的视觉分镜脚本。\n\n"

        "📖 **跨章节连续性**：复用已有角色和场景，识别角色别名，将后续新称呼归并到同一实体；当新章节提供更明确的名字、别名或设定时，更新对应圣经条目，并保留历史别名。\n\n"

        "🎬 **核心规则**：\n"
        "- 每页6个面板，index从0-5\n"
        "- 所有文字使用简体中文\n"
        "- 角色/场景必须有完整属性，禁止空值\n\n"

        "👤 **角色外观**：必须提取gender、age、hairColor、hairStyle、eyeColor、build、clothing、distinctiveFeatures。文本未明确描述时可合理推断，但不得编造与上下文冲突的信息；后续章节若提供更准确设定，应回写到角色圣经。\n\n"

        "🏞️ **场景规则**：\n"
        "- **场景ID**：使用稳定、简洁、可复用的标识，避免同一场景在不同章节生成不同ID\n"
        "- **合并同类场景**：语义、位置和视觉锚点一致的描述必须归并为同一场景实体；后续更完整的场景信息应更新到同一场景条目\n"
        "- **不拆连通空间**：玄关与客厅相连则合并为一个场景，不要拆成两个\n"
        "- **建筑统称需拆分**：整体建筑描述应拆分为具体可画的功能区域\n"
        "- **命名绝对一致**：同一场景在所有面板中使用完全相同的名称\n"
        "- 必须提取：type、setting、timeOfDay、weather、narrativeRole、visualCharacteristics、anchorPoints（至少3个）、signatureObjects（至少3个）\n\n"

        "📋 **JSON Schema 字段说明**：\n\n"
    );
    
    prompt += generateFieldDocs(schema);
    
    QJsonObject example = options.customExample.isEmpty() ? generateExample(schema) : options.customExample;
    if (!example.isEmpty()) {
        prompt += QString::fromUtf8("\n📝 **完整示例**：\n\n");
        prompt += "```json\n";
        prompt += QString::fromUtf8(QJsonDocument(example).toJson(QJsonDocument::Indented));
        prompt += "\n```\n";
    }
    
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
        
        QString requiredMark = isRequired ? QString::fromUtf8("必填") : QString::fromUtf8("可选");
        QString fieldType = getFieldType(fieldSchema);

        docs += QString("%1- **%2**（%3）").arg(indent, fieldName, requiredMark);
        
        if (!fieldType.isEmpty()) {
            docs += QString(" [%1]").arg(fieldType);
        }
        docs += "\n";
        
        if (fieldSchema.contains("description")) {
            const QString description = sanitizeDescription(fieldSchema["description"].toString());
            if (!description.isEmpty()) {
                docs += QString("%1  %2\n").arg(indent, description);
            }
        }
        
        if (fieldSchema.contains("enum")) {
            QJsonArray enumValues = fieldSchema["enum"].toArray();
            QStringList enumStrings;
            for (const QJsonValue &v : enumValues) {
                enumStrings.append(v.toString());
            }
            docs += QString("%1  可选值: %2\n").arg(indent, enumStrings.join(", "));
        }
        
        QStringList constraints = getFieldConstraints(fieldSchema);
        if (!constraints.isEmpty()) {
            docs += QString("%1  约束: %2\n").arg(indent, constraints.join(", "));
        }
        
        if (fieldSchema["type"].toString() == "object" && fieldSchema.contains("properties")) {
            docs += generateFieldDocs(fieldSchema, indentLevel + 1);
        }
        
        if (fieldSchema["type"].toString() == "array" && fieldSchema.contains("items")) {
            QJsonObject items = fieldSchema["items"].toObject();
            if (items["type"].toString() == "object" && items.contains("properties")) {
                docs += QString("%1  数组元素结构:\n").arg(indent);
                docs += generateFieldDocs(items, indentLevel + 2);
            } else if (items.contains("type")) {
                docs += QString("%1  数组元素类型: %2\n").arg(indent, items["type"].toString());
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
            return sanitizeDescription(fieldSchema["description"].toString());
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
