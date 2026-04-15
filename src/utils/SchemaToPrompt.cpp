#include "SchemaToPrompt.h"
#include <QJsonDocument>
#include <QStringList>

QString SchemaToPrompt::buildSystemPrompt(const QJsonObject &schema, const Options &options)
{
    QString prompt = QString::fromUtf8(
        "你是漫画分镜 JSON Schema 说明助手。\n\n"
        "请把下面的 Schema 整理成适合模型阅读的说明，重点突出字段用途、必填项、枚举值和约束。\n\n"
        "- 优先说明 `existingCharacters` 和 `existingScenes` 对应的已有角色/场景，避免重复创建。\n"
        "- `panel.background.sceneId` 应尽量映射到已有场景的 id。\n"
        "- `appearance` 描述角色外貌，不要写成镜头语言。\n"
        "- `visualCharacteristics` 用于固定场景的视觉特征。\n"
        "- `scene` 只描述场景本身，不包含人物或动作。\n"
        "- `visualPrompt` 必须是中文图像提示词。\n\n"
        "请按字段层级输出，保持字段名不变。\n\n"
    );
    
    prompt += generateFieldDocs(schema);
    
    prompt += "\n示例 JSON：\n\n";
    
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
        
        QString requiredMark = isRequired ? QString::fromUtf8("必填") : QString::fromUtf8("可选");
        QString fieldType = getFieldType(fieldSchema);

        docs += QString("%1- **%2**（%3）").arg(indent, fieldName, requiredMark);
        
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
            docs += QString("%1  枚举: %2\n").arg(indent, enumStrings.join(", "));
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
                docs += QString("%1  子项:\n").arg(indent);
                docs += generateFieldDocs(items, indentLevel + 2);
            } else if (items.contains("type")) {
                docs += QString("%1  子项类型: %2\n").arg(indent, items["type"].toString());
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
        {"minItems", QString::fromUtf8("最少项数")},
        {"maxItems", QString::fromUtf8("最多项数")}
    };
    
    QStringList constraints;
    for (const auto& pair : constraintKeys) {
        if (fieldSchema.contains(pair.first)) {
            constraints.append(QString("%1: %2").arg(pair.second).arg(fieldSchema[pair.first].toInt()));
        }
    }
    
    return constraints;
}
