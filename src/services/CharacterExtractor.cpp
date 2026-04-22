#include "services/CharacterExtractor.h"
#include "utils/BibleCache.h"
#include "data/DatabaseManager.h"
#include "utils/Logger.h"
#include "utils/JsonUtils.h"
#include "utils/BibleUtils.h"
#include <QDateTime>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QSet>

namespace {
QString normalizeExtractedName(const QString& name);
QString kinshipSignatureForCharacter(const QString& name, const QString& role);
bool isGenericHairColor(const QString& hairColor);

QString normalizedCharacterKey(const QString& name)
{
    return normalizeExtractedName(name).trimmed();
}

void appendUnique(QStringList& list, const QString& value)
{
    const QString trimmed = value.trimmed();
    if (!trimmed.isEmpty() && !list.contains(trimmed)) {
        list << trimmed;
    }
}

template <typename T>
void fillIfEmpty(T& target, const T& incoming)
{
    if (target.isEmpty() && !incoming.isEmpty()) {
        target = incoming;
    }
}

void fillIfEmpty(int& target, int incoming)
{
    if (target <= 0 && incoming > 0) {
        target = incoming;
    }
}

void mergeStringListIfNotEmpty(QStringList& target, const QStringList& incoming)
{
    if (!incoming.isEmpty()) {
        target = BibleUtils::mergeStringLists(target, incoming);
    }
}

QStringList collectIdentityKeys(const QString& name, const QString& role, const QStringList& aliases)
{
    QStringList keys;
    appendUnique(keys, normalizeExtractedName(name));

    const QString signature = kinshipSignatureForCharacter(name, role);
    if (!signature.isEmpty()) {
        appendUnique(keys, signature);
    }

    for (const QString& alias : aliases) {
        appendUnique(keys, normalizeExtractedName(alias));
    }

    return keys;
}

bool hasSharedIdentityKey(const QStringList& lhs, const QStringList& rhs)
{
    for (const QString& key : lhs) {
        if (rhs.contains(key, Qt::CaseInsensitive)) {
            return true;
        }
    }
    return false;
}

bool shouldAdoptHairColor(const QString& existing, const QString& incoming)
{
    const QString existingTrimmed = existing.trimmed();
    const QString incomingTrimmed = incoming.trimmed();
    if (incomingTrimmed.isEmpty()) {
        return false;
    }
    if (existingTrimmed.isEmpty()) {
        return true;
    }
    return isGenericHairColor(existingTrimmed) && !isGenericHairColor(incomingTrimmed);
}

bool containsChineseCharacters(const QString& text)
{
    return text.contains(QRegularExpression(QStringLiteral("\\p{Han}")));
}

bool isVariantCharacterName(const QString& name)
{
    const QString normalized = normalizedCharacterKey(name);
    if (normalized.isEmpty()) {
        return false;
    }

    static const QStringList variantTokens = {
        QString::fromUtf8("虚影"),
        QString::fromUtf8("影子"),
        QString::fromUtf8("倒影"),
        QString::fromUtf8("分身"),
        QString::fromUtf8("残影"),
        QString::fromUtf8("幻影"),
        QString::fromUtf8("投影"),
        QString::fromUtf8("镜像")
    };

    if (normalized.contains('_') || normalized.contains('-')) {
        return true;
    }

    for (const QString& token : variantTokens) {
        if (normalized.contains(token)) {
            return true;
        }
    }

    return false;
}

QString normalizeExtractedName(const QString& name)
{
    QString normalized = name.trimmed();

    static const QStringList suffixes = {
        QString::fromUtf8("(true form)"),
        QString::fromUtf8("(clone)"),
        QString::fromUtf8("(past)"),
        QString::fromUtf8("(future)"),
        QString::fromUtf8("(young)"),
        QString::fromUtf8("(old)"),
        QString::fromUtf8("（真身）"),
        QString::fromUtf8("（分身）"),
        QString::fromUtf8("（少年）"),
        QString::fromUtf8("（老年）"),
        QString::fromUtf8("（POV）"),
        QString::fromUtf8("（视角）"),
        QString::fromUtf8("(POV)"),
        QString::fromUtf8("(视角)"),
        QString::fromUtf8("（本体）"),
        QString::fromUtf8("(本体)")
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

QString kinshipRelationKey(const QString& name)
{
    const QString normalized = normalizedCharacterKey(name);
    if (normalized.isEmpty()) {
        return QString();
    }

    struct KinshipRelationRule {
        QStringList tokens;
        QString relationKey;
    };

    static const QList<KinshipRelationRule> rules = {
        {{QString::fromUtf8("父亲"), QString::fromUtf8("爸爸"), QString::fromUtf8("老爸"), QString::fromUtf8("爹"), QString::fromUtf8("父"), QString::fromUtf8("养父"), QString::fromUtf8("继父"), QString::fromUtf8("义父")},
         QStringLiteral("kinship:father")},
        {{QString::fromUtf8("母亲"), QString::fromUtf8("妈妈"), QString::fromUtf8("老妈"), QString::fromUtf8("娘"), QString::fromUtf8("母"), QString::fromUtf8("养母"), QString::fromUtf8("继母"), QString::fromUtf8("义母")},
         QStringLiteral("kinship:mother")},
        {{QString::fromUtf8("哥哥"), QString::fromUtf8("兄长"), QString::fromUtf8("大哥"), QString::fromUtf8("哥")},
         QStringLiteral("kinship:brother:older")},
        {{QString::fromUtf8("弟弟"), QString::fromUtf8("小弟"), QString::fromUtf8("弟")},
         QStringLiteral("kinship:brother:younger")},
        {{QString::fromUtf8("姐姐"), QString::fromUtf8("姊姊"), QString::fromUtf8("大姐"), QString::fromUtf8("姐")},
         QStringLiteral("kinship:sister:older")},
        {{QString::fromUtf8("妹妹"), QString::fromUtf8("小妹"), QString::fromUtf8("妹")},
         QStringLiteral("kinship:sister:younger")},
        {{QString::fromUtf8("爷爷"), QString::fromUtf8("祖父"), QString::fromUtf8("爷"), QString::fromUtf8("外公"), QString::fromUtf8("外祖父")},
         QStringLiteral("kinship:grandfather")},
        {{QString::fromUtf8("奶奶"), QString::fromUtf8("祖母"), QString::fromUtf8("奶"), QString::fromUtf8("外婆"), QString::fromUtf8("外祖母")},
         QStringLiteral("kinship:grandmother")},
        {{QString::fromUtf8("叔叔"), QString::fromUtf8("叔")},
         QStringLiteral("kinship:uncle")},
        {{QString::fromUtf8("阿姨"), QString::fromUtf8("姨")},
         QStringLiteral("kinship:aunt")},
        {{QString::fromUtf8("伯伯"), QString::fromUtf8("伯")},
         QStringLiteral("kinship:uncle:elder")},
        {{QString::fromUtf8("姑姑"), QString::fromUtf8("姑")},
         QStringLiteral("kinship:aunt:paternal")},
        {{QString::fromUtf8("舅舅"), QString::fromUtf8("舅")},
         QStringLiteral("kinship:uncle:maternal")},
        {{QString::fromUtf8("婶婶"), QString::fromUtf8("婶")},
         QStringLiteral("kinship:inlaw")}
    };

    for (const KinshipRelationRule& rule : rules) {
        for (const QString& token : rule.tokens) {
            if (normalized.contains(token)) {
                return rule.relationKey;
            }
        }
    }

    return QString();
}

QString extractKinshipTargetKey(const QString& text)
{
    const QString normalized = normalizedCharacterKey(text);
    if (normalized.isEmpty()) {
        return QString();
    }

    static const QStringList kinshipTokens = {
        QString::fromUtf8("父亲"),
        QString::fromUtf8("母亲"),
        QString::fromUtf8("哥哥"),
        QString::fromUtf8("姐姐"),
        QString::fromUtf8("弟弟"),
        QString::fromUtf8("妹妹"),
        QString::fromUtf8("爷爷"),
        QString::fromUtf8("奶奶"),
        QString::fromUtf8("叔叔"),
        QString::fromUtf8("阿姨"),
        QString::fromUtf8("伯伯"),
        QString::fromUtf8("姑姑"),
        QString::fromUtf8("舅舅"),
        QString::fromUtf8("婶婶"),
        QString::fromUtf8("爸爸"),
        QString::fromUtf8("妈妈"),
        QString::fromUtf8("老爸"),
        QString::fromUtf8("老妈"),
        QString::fromUtf8("爹"),
        QString::fromUtf8("娘"),
        QString::fromUtf8("父"),
        QString::fromUtf8("母"),
        QString::fromUtf8("哥"),
        QString::fromUtf8("姐"),
        QString::fromUtf8("弟"),
        QString::fromUtf8("妹"),
        QString::fromUtf8("爷"),
        QString::fromUtf8("奶"),
        QString::fromUtf8("叔"),
        QString::fromUtf8("姨"),
        QString::fromUtf8("伯"),
        QString::fromUtf8("姑"),
        QString::fromUtf8("舅"),
        QString::fromUtf8("婶")
    };

    int bestPos = -1;
    int bestTokenLength = 0;
    for (const QString& token : kinshipTokens) {
        const int pos = normalized.indexOf(token);
        if (pos <= 0) {
            continue;
        }

        if (bestPos < 0 || pos < bestPos || (pos == bestPos && token.length() > bestTokenLength)) {
            bestPos = pos;
            bestTokenLength = token.length();
        }
    }

    if (bestPos <= 0) {
        return QString();
    }

    QString target = normalized.left(bestPos).trimmed();
    static const QStringList targetSuffixes = {
        QString::fromUtf8("之"),
        QString::fromUtf8("的"),
        QString::fromUtf8("其"),
        QString::fromUtf8("这位"),
        QString::fromUtf8("那位")
    };

    bool changed = true;
    while (changed && !target.isEmpty()) {
        changed = false;
        for (const QString& suffix : targetSuffixes) {
            if (target.endsWith(suffix)) {
                target.chop(suffix.length());
                target = target.trimmed();
                changed = true;
                break;
            }
        }
    }

    return target;
}

QString kinshipSignatureFromText(const QString& text)
{
    const QString relationKey = kinshipRelationKey(text);
    const QString targetKey = extractKinshipTargetKey(text);
    if (relationKey.isEmpty() || targetKey.isEmpty()) {
        return QString();
    }

    return QStringLiteral("%1|%2").arg(relationKey, targetKey);
}

QString kinshipSignatureForCharacter(const QString& name, const QString& role)
{
    const QString roleSignature = kinshipSignatureFromText(role);
    if (!roleSignature.isEmpty()) {
        return roleSignature;
    }

    const QString nameSignature = kinshipSignatureFromText(name);
    if (!nameSignature.isEmpty()) {
        return nameSignature;
    }

    return QString();
}

QString extractCharacterContext(const QString& sourceText, const QString& name, int radius = 120)
{
    if (sourceText.isEmpty() || name.isEmpty()) {
        return QString();
    }

    const int index = sourceText.indexOf(name);
    if (index < 0) {
        return QString();
    }

    const int start = qMax(0, index - radius);
    const int length = qMin(sourceText.size() - start, name.size() + radius * 2);
    return sourceText.mid(start, length);
}

QString matchFirstCapture(const QString& text, const QList<QRegularExpression>& patterns)
{
    for (const QRegularExpression& pattern : patterns) {
        QRegularExpressionMatch match = pattern.match(text);
        if (match.hasMatch()) {
            return match.captured(1).trimmed();
        }
    }
    return QString();
}

bool isGenericHairColor(const QString& hairColor)
{
    const QString normalized = hairColor.trimmed();
    if (normalized.isEmpty()) {
        return true;
    }

    static const QSet<QString> genericHairColors = {
        QString::fromUtf8("黑"),
        QString::fromUtf8("黑色"),
        QString::fromUtf8("乌黑"),
        QString::fromUtf8("乌黑色"),
        QString::fromUtf8("棕"),
        QString::fromUtf8("棕色"),
        QString::fromUtf8("深棕"),
        QString::fromUtf8("深棕色"),
        QString::fromUtf8("浅棕"),
        QString::fromUtf8("浅棕色"),
        QString::fromUtf8("黑褐"),
        QString::fromUtf8("黑褐色"),
        QString::fromUtf8("棕褐"),
        QString::fromUtf8("棕褐色"),
        QString::fromUtf8("银白"),
        QString::fromUtf8("银白色"),
        QString::fromUtf8("灰白"),
        QString::fromUtf8("灰白色"),
        QString::fromUtf8("花白"),
        QString::fromUtf8("花白色"),
        QString::fromUtf8("金"),
        QString::fromUtf8("金色"),
        QString::fromUtf8("栗"),
        QString::fromUtf8("栗色")
    };

    return genericHairColors.contains(normalized);
}

QStringList collectKeywords(const QString& text, const QStringList& keywords)
{
    QStringList result;
    for (const QString& keyword : keywords) {
        if (text.contains(keyword) && !result.contains(keyword)) {
            result << keyword;
        }
    }
    return result;
}

QString readStringField(const QJsonObject& primary, const QJsonObject& fallback, const QString& key)
{
    QString value = primary.value(key).toString();
    if (value.isEmpty()) {
        value = fallback.value(key).toString();
    }
    return value;
}

QStringList readStringListField(const QJsonObject& primary, const QJsonObject& fallback, const QString& key)
{
    QStringList value = JsonUtils::jsonArrayToStringList(primary.value(key).toArray());
    if (value.isEmpty()) {
        value = JsonUtils::jsonArrayToStringList(fallback.value(key).toArray());
    }
    return value;
}

void mergeAliasFields(CharacterAppearance& app, QStringList& aliases, const QStringList& incomingAliases)
{
    aliases = BibleUtils::mergeStringLists(aliases, incomingAliases);
    app.aliases = BibleUtils::mergeStringLists(app.aliases, incomingAliases);
}

QString extractPanelCharacterName(const QJsonValue& charVal)
{
    if (!charVal.isString()) {
        return QString();
    }

    const QString charStr = charVal.toString().trimmed();
    if (charStr.isEmpty()) {
        return QString();
    }

    const int parenPos = charStr.indexOf('(');
    if (parenPos <= 0) {
        return charStr;
    }

    return charStr.left(parenPos).trimmed();
}

QString extractPanelCharacterFeature(const QJsonValue& charVal)
{
    if (!charVal.isString()) {
        return QString();
    }

    const QString charStr = charVal.toString().trimmed();
    const int parenPos = charStr.indexOf('(');
    if (parenPos <= 0) {
        return QString();
    }

    const int endParen = charStr.indexOf(')', parenPos);
    if (endParen <= parenPos) {
        return QString();
    }

    return charStr.mid(parenPos + 1, endParen - parenPos - 1).trimmed();
}

QString composeFeatureText(const QString& first, const QString& second)
{
    QString featureText = first.trimmed();
    const QString extra = second.trimmed();
    if (featureText.isEmpty()) {
        return extra;
    }
    if (!extra.isEmpty()) {
        featureText += ' ';
        featureText += extra;
    }
    return featureText;
}

bool isPronounLikeCharacter(const QString& name)
{
    static const QStringList pronouns = {
        QString::fromUtf8("我"),
        QString::fromUtf8("我们"),
        QString::fromUtf8("你"),
        QString::fromUtf8("你们"),
        QString::fromUtf8("他"),
        QString::fromUtf8("他们"),
        QString::fromUtf8("她"),
        QString::fromUtf8("她们"),
        QString::fromUtf8("它"),
        QString::fromUtf8("它们"),
        QString::fromUtf8("对方")
    };

    return pronouns.contains(normalizedCharacterKey(name));
}

bool isGenericCharacterName(const QString& name)
{
    const QString normalized = normalizedCharacterKey(name);
    if (normalized.isEmpty()) {
        return true;
    }

    if (isPronounLikeCharacter(normalized)) {
        return true;
    }

    static const QSet<QString> genericLabels = {
        QString::fromUtf8("男人"),
        QString::fromUtf8("女人"),
        QString::fromUtf8("老人"),
        QString::fromUtf8("小孩"),
        QString::fromUtf8("孩子"),
        QString::fromUtf8("少年"),
        QString::fromUtf8("少女"),
        QString::fromUtf8("青年"),
        QString::fromUtf8("中年人"),
        QString::fromUtf8("老年人"),
        QString::fromUtf8("路人"),
        QString::fromUtf8("行人"),
        QString::fromUtf8("旁人"),
        QString::fromUtf8("众人"),
        QString::fromUtf8("人群"),
        QString::fromUtf8("陌生人"),
        QString::fromUtf8("父亲"),
        QString::fromUtf8("母亲"),
        QString::fromUtf8("爸爸"),
        QString::fromUtf8("妈妈"),
        QString::fromUtf8("老爸"),
        QString::fromUtf8("老妈"),
        QString::fromUtf8("爹"),
        QString::fromUtf8("娘"),
        QString::fromUtf8("哥哥"),
        QString::fromUtf8("姐姐"),
        QString::fromUtf8("弟弟"),
        QString::fromUtf8("妹妹"),
        QString::fromUtf8("爷爷"),
        QString::fromUtf8("奶奶"),
        QString::fromUtf8("叔叔"),
        QString::fromUtf8("阿姨"),
        QString::fromUtf8("伯伯"),
        QString::fromUtf8("姑姑"),
        QString::fromUtf8("舅舅"),
        QString::fromUtf8("婶婶"),
        QString::fromUtf8("身影"),
        QString::fromUtf8("影子"),
        QString::fromUtf8("黑影"),
        QString::fromUtf8("背影"),
        QString::fromUtf8("声音")
    };

    if (genericLabels.contains(normalized)) {
        return true;
    }

    return false;
}

int purgeGenericCharacterRows(const QString& novelId)
{
    int removedCount = 0;
    QList<QVariantMap> rows = DatabaseManager::instance()->selectAll(
        "characters", "novel_id = ?", QVariantList() << novelId);

    if (rows.isEmpty()) {
        return 0;
    }

    const bool cleaned = DatabaseManager::instance()->transaction([&]() -> bool {
        for (const QVariantMap& row : rows) {
            const QString name = row.value("name").toString();
            if (!isGenericCharacterName(name)) {
                continue;
            }

            const QString id = row.value("id").toString();
            if (id.isEmpty()) {
                continue;
            }

            if (!DatabaseManager::instance()->remove("characters", "id = ?", QVariantList() << id)) {
                return false;
            }

            ++removedCount;
            LOG_INFO("CharacterExtractor", QString("Purged generic character: %1").arg(name));
        }
        return true;
    });

    return cleaned ? removedCount : 0;
}

int parseAgeFromString(const QString& ageStr)
{
    bool ok = false;
    int age = ageStr.toInt(&ok);
    if (ok && age > 0) {
        return age;
    }
    QString normalized = ageStr.trimmed();

    static const QList<QPair<QRegularExpression, int>> agePatterns = {
        {QRegularExpression(QString::fromUtf8("二十[一二三四五六七八九]?岁?")), 22},
        {QRegularExpression(QString::fromUtf8("三十[一二三四五六七八九]?岁?")), 32},
        {QRegularExpression(QString::fromUtf8("四十[一二三四五六七八九]?岁?")), 42},
        {QRegularExpression(QString::fromUtf8("五十[一二三四五六七八九]?岁?")), 52},
        {QRegularExpression(QString::fromUtf8("六十[一二三四五六七八九]?岁?")), 62},
        {QRegularExpression(QString::fromUtf8("七十[一二三四五六七八九]?岁?")), 72},
        {QRegularExpression(QString::fromUtf8("二十[一二三四五六七八九]?岁?[上下左右]")), 22},
        {QRegularExpression(QString::fromUtf8("三十[一二三四五六七八九]?岁?[上下左右]")), 32},
        {QRegularExpression(QString::fromUtf8("四十[一二三四五六七八九]?岁?[上下左右]")), 42},
        {QRegularExpression(QString::fromUtf8("五十[一二三四五六七八九]?岁?[上下左右]")), 52},
        {QRegularExpression(QString::fromUtf8("(\\d+)\\s*岁")), 0},
        {QRegularExpression(QString::fromUtf8("约?(\\d+)\\s*岁")), 0}
    };

    for (const auto& pattern : agePatterns) {
        QRegularExpressionMatch match = pattern.first.match(normalized);
        if (match.hasMatch()) {
            if (pattern.second > 0) {
                return pattern.second;
            }
            bool capturedOk = false;
            const int extractedAge = match.captured(1).toInt(&capturedOk);
            if (capturedOk && extractedAge > 0) {
                return extractedAge;
            }
        }
    }

    static const QMap<QString, int> ageRangeMapping = {
        {QString::fromUtf8("二十出头"), 22},
        {QString::fromUtf8("二十来岁"), 23},
        {QString::fromUtf8("三十出头"), 32},
        {QString::fromUtf8("三十来岁"), 33},
        {QString::fromUtf8("四十出头"), 42},
        {QString::fromUtf8("四十来岁"), 43},
        {QString::fromUtf8("五十出头"), 52},
        {QString::fromUtf8("五十来岁"), 53}
    };

    for (auto it = ageRangeMapping.constBegin(); it != ageRangeMapping.constEnd(); ++it) {
        if (normalized.contains(it.key())) {
            return it.value();
        }
    }

    return 0;
}
}

CharacterExtractor* CharacterExtractor::m_instance = nullptr;
std::once_flag CharacterExtractor::m_instanceOnceFlag;

QJsonObject ExtractedCharacter::toJson() const
{
    QJsonObject json;
    json["name"] = name;
    json["role"] = role;
    json["gender"] = gender;
    json["age"] = age;
    json["hairColor"] = hairColor;
    json["hairStyle"] = hairStyle;
    json["eyeColor"] = eyeColor;
    json["bodyType"] = bodyType;
    json["height"] = height;
    json["clothing"] = JsonUtils::stringListToJsonArray(clothing);
    json["accessories"] = JsonUtils::stringListToJsonArray(accessories);
    json["distinctiveFeatures"] = JsonUtils::stringListToJsonArray(distinctiveFeatures);
    json["aliases"] = JsonUtils::stringListToJsonArray(aliases);
    json["personality"] = JsonUtils::stringListToJsonArray(personality);
    json["tags"] = JsonUtils::stringListToJsonArray(tags);
    return json;
}

ExtractedCharacter ExtractedCharacter::fromJson(const QJsonObject& json)
{
    ExtractedCharacter c;
    c.name = json["name"].toString();
    c.role = json["role"].toString();
    c.gender = json["gender"].toString();
    c.age = json["age"].toInt();
    c.hairColor = json["hairColor"].toString();
    c.hairStyle = json["hairStyle"].toString();
    c.eyeColor = json["eyeColor"].toString();
    c.bodyType = json["bodyType"].toString();
    c.height = json["height"].toString();
    c.clothing = JsonUtils::jsonArrayToStringList(json["clothing"].toArray());
    c.accessories = JsonUtils::jsonArrayToStringList(json["accessories"].toArray());
    c.distinctiveFeatures = JsonUtils::jsonArrayToStringList(json["distinctiveFeatures"].toArray());
    c.aliases = JsonUtils::jsonArrayToStringList(json["aliases"].toArray());
    c.personality = JsonUtils::jsonArrayToStringList(json["personality"].toArray());
    c.tags = JsonUtils::jsonArrayToStringList(json["tags"].toArray());
    return c;
}

CharacterExtractor::CharacterExtractor(QObject* parent)
    : QObject(parent)
{
}

CharacterExtractor::~CharacterExtractor()
{
    LOG_INFO("CharacterExtractor", "CharacterExtractor destroyed");
}

CharacterExtractor* CharacterExtractor::instance()
{
    std::call_once(m_instanceOnceFlag, []() {
        m_instance = new CharacterExtractor();
    });
    return m_instance;
}

QList<ExtractedCharacter> CharacterExtractor::extractFromPanels(const QJsonArray& panels)
{
    QList<ExtractedCharacter> characters;
    QSet<QString> seenNames;

    for (const QJsonValue& panelVal : panels) {
        QJsonObject panel = panelVal.toObject();
        QJsonArray charArray = panel["characters"].toArray();
        
        for (const QJsonValue& charVal : charArray) {
            ExtractedCharacter extracted = parsePanelCharacter(charVal);
            
            if (!extracted.name.isEmpty() && !seenNames.contains(extracted.name)) {
                seenNames.insert(extracted.name);
                characters.append(extracted);
                emit characterExtracted(extracted);
            }
        }
    }

    emit extractionCompleted(characters.size());
    LOG_INFO("CharacterExtractor", QString("Extracted %1 unique characters from %2 panels")
        .arg(characters.size()).arg(panels.size()));

    return characters;
}

ExtractedCharacter CharacterExtractor::parsePanelCharacter(const QJsonValue& charVal)
{
    ExtractedCharacter extracted;

    if (charVal.isString()) {
        extracted.name = extractPanelCharacterName(charVal);
        const QString featureText = extractPanelCharacterFeature(charVal);
        if (!featureText.isEmpty()) {
            extracted.distinctiveFeatures << featureText;
        }
    } else if (charVal.isObject()) {
        QJsonObject charObj = charVal.toObject();
        extracted.name = normalizeExtractedName(charObj.value("name").toString());
        const QString featureText = composeFeatureText(
            charObj.value("expression").toString(),
            charObj.value("pose").toString());
        if (!featureText.isEmpty()) {
            extracted.distinctiveFeatures << featureText;
        }
        extracted.aliases = JsonUtils::jsonArrayToStringList(charObj.value("aliases").toArray());
    }

    extracted.name = normalizedCharacterKey(extracted.name);

    if (isVariantCharacterName(extracted.name) || isGenericCharacterName(extracted.name)) {
        extracted.name.clear();
        return extracted;
    }

    return extracted;
}

QList<ExtractedCharacter> CharacterExtractor::extractFromCharacters(const QJsonArray& characters)
{
    return extractFromCharacters(characters, QString());
}

QList<ExtractedCharacter> CharacterExtractor::extractFromCharacters(const QJsonArray& characters, const QString& sourceText)
{
    QList<ExtractedCharacter> result;

    for (const QJsonValue& charVal : characters) {
        QJsonObject charObj = charVal.toObject();
        ExtractedCharacter extracted = parseAICharacter(charObj);
        enrichCharacterFromText(extracted, sourceText);
        
        if (!extracted.name.isEmpty() && containsChineseCharacters(extracted.name)) {
            result.append(extracted);
            emit characterExtracted(extracted);
        } else if (!extracted.name.isEmpty()) {
            LOG_DEBUG("CharacterExtractor", QString("Skipping non-Chinese character: %1").arg(extracted.name));
        }
    }
    
    emit extractionCompleted(result.size());
    LOG_INFO("CharacterExtractor", QString("Extracted %1 characters from AI response")
        .arg(result.size()));
    
    return result;
}

ExtractedCharacter CharacterExtractor::parseAICharacter(const QJsonObject& charObj)
{
    ExtractedCharacter extracted;
    extracted.name = normalizedCharacterKey(charObj.value("name").toString());
    extracted.role = charObj.value("role").toString();
    
    if (extracted.name.isEmpty()) {
        return extracted;
    }

    if (isVariantCharacterName(extracted.name) || isGenericCharacterName(extracted.name)) {
        LOG_DEBUG("CharacterExtractor", QString("Skipping non-specific character: %1").arg(extracted.name));
        extracted.name.clear();
        return extracted;
    }
    
    QJsonObject appObj = charObj["appearance"].toObject();
    if (appObj.isEmpty() && charObj["appearance"].isString()) {
        QJsonParseError parseError;
        QJsonDocument appearanceDoc = QJsonDocument::fromJson(charObj["appearance"].toString().toUtf8(), &parseError);
        if (parseError.error == QJsonParseError::NoError && appearanceDoc.isObject()) {
            appObj = appearanceDoc.object();
        }
    }
    if (appObj.isEmpty()) {
        appObj = charObj;
    }
    LOG_DEBUG("CharacterExtractor", QString("Parsing character '%1', role=%2, appearance: %3")
        .arg(extracted.name)
        .arg(extracted.role)
        .arg(QString::fromUtf8(QJsonDocument(appObj).toJson(QJsonDocument::Compact))));

    extracted.gender = readStringField(appObj, charObj, "gender");
    QJsonValue ageVal = appObj.value("age");
    if (ageVal.isUndefined() || ageVal.isNull()) {
        ageVal = charObj.value("age");
    }
    if (ageVal.isDouble()) {
        extracted.age = ageVal.toInt();
    } else {
        extracted.age = parseAgeFromString(ageVal.toString());
    }
    extracted.hairColor = readStringField(appObj, charObj, "hairColor");
    extracted.hairStyle = readStringField(appObj, charObj, "hairStyle");
    extracted.eyeColor = readStringField(appObj, charObj, "eyeColor");
    extracted.bodyType = readStringField(appObj, charObj, "build");
    extracted.height = readStringField(appObj, charObj, "height");
    extracted.clothing = readStringListField(appObj, charObj, "clothing");
    extracted.accessories = readStringListField(appObj, charObj, "accessories");
    extracted.distinctiveFeatures = readStringListField(appObj, charObj, "distinctiveFeatures");
    extracted.aliases = readStringListField(appObj, charObj, "aliases");
    extracted.personality = JsonUtils::jsonArrayToStringList(charObj.value("personality").toArray());
    if (extracted.personality.isEmpty()) {
        extracted.personality = JsonUtils::jsonArrayToStringList(charObj.value("personalities").toArray());
    }
    extracted.tags = JsonUtils::jsonArrayToStringList(charObj.value("tags").toArray());

    LOG_DEBUG("CharacterExtractor", QString("Parsed character '%1': role=%2, gender=%3, age=%4, hairColor=%5, eyeColor=%6, build=%7")
        .arg(extracted.name)
        .arg(extracted.role)
        .arg(extracted.gender)
        .arg(extracted.age)
        .arg(extracted.hairColor)
        .arg(extracted.eyeColor)
        .arg(extracted.bodyType));

    return extracted;
}

void CharacterExtractor::enrichCharacterFromText(ExtractedCharacter& extracted, const QString& sourceText)
{
    if (isPronounLikeCharacter(extracted.name)) {
        LOG_DEBUG("CharacterExtractor", QString("Skip text enrichment for pronoun-like character '%1'").arg(extracted.name));
        return;
    }

    const QString context = extractCharacterContext(sourceText, extracted.name);
    if (context.isEmpty()) {
        return;
    }

    static const QList<QRegularExpression> hairColorPatterns = {
        QRegularExpression(QString::fromUtf8("((?:浅亚麻|亚麻|浅棕|深棕|棕褐|黑褐|黑|乌黑|灰白|花白|金|银白|银灰|栗)[色]?)[^，。；\\n]{0,8}(?:低马尾|高马尾|马尾|短发|长发|卷发|碎发|头发)")),
        QRegularExpression(QString::fromUtf8("(浅亚麻色|亚麻色|浅棕色|深棕色|棕褐色|黑褐色|黑色|乌黑|灰白色|花白|银白|银灰色|金色|栗色)"))
    };

    static const QList<QRegularExpression> hairStylePatterns = {
        QRegularExpression(QString::fromUtf8("(低马尾|高马尾|马尾|短发|长发|卷发|齐肩短发|齐耳短发|碎发|丸子头)")),
        QRegularExpression(QString::fromUtf8("([一-龥]{0,6}(?:马尾|短发|长发))"))
    };

    static const QList<QRegularExpression> eyeColorPatterns = {
        QRegularExpression(QString::fromUtf8("((?:黑|棕|浅褐|灰褐|琥珀)[色]?)[^，。；\\n]{0,4}(?:眼|眼底|眸|瞳)"))
    };

    static const QList<QRegularExpression> bodyTypePatterns = {
        QRegularExpression(QString::fromUtf8("(清瘦|瘦削|瘦高|高挑|苗条|纤细|单薄|健壮|结实|修长)[^，。；\\n]{0,6}(?:身形|体型|身材)?"))
    };

    static const QStringList clothingKeywords = {
        QString::fromUtf8("白色T恤"),
        QString::fromUtf8("白T恤"),
        QString::fromUtf8("白色T"),
        QString::fromUtf8("纯白T恤"),
        QString::fromUtf8("浅蓝色牛仔裤"),
        QString::fromUtf8("浅蓝牛仔裤"),
        QString::fromUtf8("牛仔裤"),
        QString::fromUtf8("白色帆布鞋"),
        QString::fromUtf8("帆布鞋"),
        QString::fromUtf8("围裙"),
        QString::fromUtf8("衬衫"),
        QString::fromUtf8("长裙")
    };

    static const QStringList distinctiveFeaturesKeywords = {
        QString::fromUtf8("虎牙"),
        QString::fromUtf8("酒窝"),
        QString::fromUtf8("泪痣"),
        QString::fromUtf8("伤痕"),
        QString::fromUtf8("疤痕"),
        QString::fromUtf8("眼镜")
    };

    static const QStringList accessoriesKeywords = {
        QString::fromUtf8("凉茶"),
        QString::fromUtf8("旧诗集")
    };

    const QString textHairColor = matchFirstCapture(context, hairColorPatterns);
    if (!textHairColor.isEmpty() && (extracted.hairColor.isEmpty() || isGenericHairColor(extracted.hairColor))) {
        extracted.hairColor = textHairColor;
    }

    if (extracted.hairStyle.isEmpty()) {
        extracted.hairStyle = matchFirstCapture(context, hairStylePatterns);
    }

    if (extracted.eyeColor.isEmpty()) {
        extracted.eyeColor = matchFirstCapture(context, eyeColorPatterns);
    }

    if (extracted.bodyType.isEmpty()) {
        extracted.bodyType = matchFirstCapture(context, bodyTypePatterns);
    }

    if (extracted.clothing.isEmpty()) {
        extracted.clothing = collectKeywords(context, clothingKeywords);
    }

    if (extracted.distinctiveFeatures.isEmpty()) {
        extracted.distinctiveFeatures = collectKeywords(context, distinctiveFeaturesKeywords);
    }

    if (extracted.accessories.isEmpty()) {
        extracted.accessories = collectKeywords(context, accessoriesKeywords);
    }

    LOG_DEBUG("CharacterExtractor", QString("Enriched from text for '%1': hairColor=%2, hairStyle=%3, eyeColor=%4, build=%5, clothing=%6, features=%7, accessories=%8")
        .arg(extracted.name)
        .arg(extracted.hairColor)
        .arg(extracted.hairStyle)
        .arg(extracted.eyeColor)
        .arg(extracted.bodyType)
        .arg(extracted.clothing.join(", "))
        .arg(extracted.distinctiveFeatures.join(", "))
        .arg(extracted.accessories.join(", ")));
}

Character CharacterExtractor::toCharacter(const ExtractedCharacter& extracted, const QString& novelId)
{
    Character character;
    character.setId(BibleUtils::Utils::generateId());
    character.setNovelId(novelId);
    character.setName(extracted.name);
    character.setRole(extracted.role);
    
    CharacterAppearance app;
    app.gender = extracted.gender;
    app.age = extracted.age;
    app.hairColor = extracted.hairColor;
    app.hairStyle = extracted.hairStyle;
    app.eyeColor = extracted.eyeColor;
    app.build = extracted.bodyType;
    app.height = extracted.height;
    app.clothing = extracted.clothing;
    app.accessories = extracted.accessories;
    app.distinctiveFeatures = extracted.distinctiveFeatures;
    app.aliases = extracted.aliases;
    
    character.setAppearance(app);
    character.setAliases(extracted.aliases);
    character.setPersonality(extracted.personality);
    
    return character;
}

QStringList CharacterExtractor::buildIdentityKeys(const Character& character)
{
    return collectIdentityKeys(character.name(), character.role(), character.aliases());
}

QStringList CharacterExtractor::buildIdentityKeys(const ExtractedCharacter& character)
{
    return collectIdentityKeys(character.name, character.role, character.aliases);
}

bool CharacterExtractor::isLikelySameCharacter(const Character& existing, const ExtractedCharacter& incoming)
{
    return hasSharedIdentityKey(buildIdentityKeys(existing), buildIdentityKeys(incoming));
}

QString CharacterExtractor::normalizeCharacterName(const QString& name)
{
    if (name.isEmpty()) return name;
    
    QString normalized = name.trimmed();
    
    // 清理 AI 常见的角色后缀，避免同一角色被拆成多个名字
    static const QStringList suffixes = {
        "(true form)",
        "(clone)",
        "(past)",
        "(future)",
        "(young)",
        "(old)",
        "（真身）",
        "（分身）",
        "（少年）",
        "（老年）"
    };
    
    for (const QString& suffix : suffixes) {
        if (normalized.endsWith(suffix, Qt::CaseInsensitive)) {
            normalized.chop(suffix.length());
            normalized = normalized.trimmed();
            
            LOG_DEBUG("CharacterExtractor", QString("Normalized character name: '%1' -> '%2'")
                .arg(name).arg(normalized));
            break;
        }
    }
    
    return normalized;
}

namespace {
bool matchesCharacterName(const Character& candidate, const QString& name)
{
    const QString normalizedName = normalizeExtractedName(name).trimmed();
    if (normalizedName.isEmpty()) {
        return false;
    }

    const QString candidateName = normalizeExtractedName(candidate.name()).trimmed();
    if (candidateName.compare(normalizedName, Qt::CaseInsensitive) == 0) {
        return true;
    }

    const QStringList incomingAliases = collectIdentityKeys(name, QString(), QStringList());
    for (const QString& alias : incomingAliases) {
        if (candidateName.compare(alias, Qt::CaseInsensitive) == 0) {
            return true;
        }
    }

    return false;
}

Character findMatchingCharacter(const QList<Character>& candidates, const ExtractedCharacter& extracted)
{
    for (const Character& candidate : candidates) {
        if (CharacterExtractor::isLikelySameCharacter(candidate, extracted) ||
            matchesCharacterName(candidate, extracted.name)) {
            return candidate;
        }
    }

    return Character();
}
}

bool CharacterExtractor::saveCharacter(const QString& novelId, const ExtractedCharacter& extracted)
{
    if (isGenericCharacterName(extracted.name)) {
        LOG_DEBUG("CharacterExtractor", QString("Skipping generic character: %1").arg(extracted.name));
        return true;
    }

    Character character = toCharacter(extracted, novelId);
    const QString normalizedName = normalizeCharacterName(character.name());
    if (!normalizedName.isEmpty() && normalizedName != character.name()) {
        character.setName(normalizedName);
    }

    QList<Character> candidates = getCharactersByNovel(novelId);
    Character existing = findMatchingCharacter(candidates, extracted);
    if (!existing.id().isEmpty()) {
        LOG_DEBUG("CharacterExtractor", QString("Character '%1' matched existing '%2', merging...")
            .arg(character.name(), existing.name()));
        Character merged = mergeCharacters(existing, extracted);
        return updateCharacter(merged);
    }

    QVariantMap data = characterToData(character);
    qint64 result = DatabaseManager::instance()->insert("characters", data);

    if (result > 0) {
        LOG_INFO("CharacterExtractor", QString("Saved new character: %1").arg(character.name()));
        BibleCache::instance()->invalidateCharacters(novelId);
        emit characterSaved(character.id());
    } else {
        LOG_ERROR("CharacterExtractor", QString("Failed to save character: %1").arg(character.name()));
    }

    return result > 0;
}

// Merge extracted character data with an existing character record.
Character CharacterExtractor::mergeCharacters(const Character& existing, const ExtractedCharacter& incoming) const
{
    Character merged = existing;
    CharacterAppearance app = merged.appearance();

    // 优先使用直接提取的role字段
    if (merged.role().isEmpty() && !incoming.role.isEmpty()) {
        merged.setRole(incoming.role);
    }

    // 如果role仍然为空，尝试从tags中提取
    if (merged.role().isEmpty() && !incoming.tags.isEmpty()) {
        static const QStringList roleOrder = {
            "protagonist",
            "antagonist",
            "supporting",
            "background",
            "mentor",
            "love-interest"
        };

        for (const QString& role : roleOrder) {
            if (incoming.tags.contains(role, Qt::CaseInsensitive)) {
                merged.setRole(role);
                break;
            }
        }

        if (merged.role().isEmpty()) {
            merged.setRole(incoming.tags.first());
        }
    }

    fillIfEmpty(app.gender, incoming.gender);
    fillIfEmpty(app.age, incoming.age);
    if (shouldAdoptHairColor(app.hairColor, incoming.hairColor)) {
        app.hairColor = incoming.hairColor;
    }
    fillIfEmpty(app.hairStyle, incoming.hairStyle);
    fillIfEmpty(app.eyeColor, incoming.eyeColor);
    fillIfEmpty(app.build, incoming.bodyType);
    fillIfEmpty(app.height, incoming.height);

    mergeStringListIfNotEmpty(app.clothing, incoming.clothing);
    mergeStringListIfNotEmpty(app.accessories, incoming.accessories);
    mergeStringListIfNotEmpty(app.distinctiveFeatures, incoming.distinctiveFeatures);

    QStringList aliases = merged.aliases();
    mergeAliasFields(app, aliases, incoming.aliases);
    appendUnique(aliases, incoming.name);

    merged.setAppearance(app);
    merged.setAliases(aliases);

    if (!incoming.personality.isEmpty()) {
        merged.setPersonality(BibleUtils::mergeStringLists(merged.personality(), incoming.personality));
    }

    return merged;
}

QVariantMap CharacterExtractor::characterToData(const Character& character) const
{
    QVariantMap data;
    data["id"] = character.id();
    data["novel_id"] = character.novelId();
    data["name"] = character.name();
    data["role"] = character.role();

    CharacterAppearance app = character.appearance();
    data["gender"] = app.gender;
    data["age"] = app.age;

    QJsonObject appearanceJson;
    appearanceJson["gender"] = app.gender;
    appearanceJson["age"] = app.age;
    appearanceJson["hairColor"] = app.hairColor;
    appearanceJson["hairStyle"] = app.hairStyle;
    appearanceJson["eyeColor"] = app.eyeColor;
    appearanceJson["height"] = app.height;
    appearanceJson["build"] = app.build;
    appearanceJson["clothing"] = JsonUtils::stringListToJsonArray(app.clothing);
    appearanceJson["accessories"] = JsonUtils::stringListToJsonArray(app.accessories);
    appearanceJson["distinctiveFeatures"] = JsonUtils::stringListToJsonArray(app.distinctiveFeatures);
    appearanceJson["aliases"] = JsonUtils::stringListToJsonArray(app.aliases);
    data["appearance"] = JsonUtils::jsonToString(appearanceJson);
    data["personalities"] = JsonUtils::jsonToString(JsonUtils::stringListToJsonArray(character.personality()));
    data["portrait_path"] = character.portraitPath();

    return data;
}

int CharacterExtractor::saveCharacters(const QString& novelId, const QList<ExtractedCharacter>& characters)
{
    if (characters.isEmpty()) {
        return 0;
    }

    int savedCount = 0;

    auto operation = [&]() -> bool {
        for (const ExtractedCharacter& extracted : characters) {
            if (isGenericCharacterName(extracted.name)) {
                LOG_DEBUG("CharacterExtractor", QString("Skipping generic character in batch: %1").arg(extracted.name));
                continue;
            }
            if (!saveCharacter(novelId, extracted)) {
                return false;
            }
            savedCount++;
        }
        return true;
    };

    if (!DatabaseManager::instance()->transaction(operation)) {
        LOG_ERROR("CharacterExtractor", QString("Transaction failed, characters may be partially saved for novel %1").arg(novelId));
        savedCount = 0;
    } else {
        const int purgedCount = purgeGenericCharacterRows(novelId);
        if (purgedCount > 0) {
            LOG_INFO("CharacterExtractor", QString("Purged %1 generic characters for novel %2")
                .arg(purgedCount).arg(novelId));
        }
    }

    LOG_INFO("CharacterExtractor", QString("Saved %1/%2 characters for novel %3")
        .arg(savedCount).arg(characters.size()).arg(novelId));

    return savedCount;
}

Character CharacterExtractor::characterFromRow(const QVariantMap& row)
{
    Character character = Character::fromVariantMap(row);
    const CharacterAppearance app = character.appearance();
    LOG_DEBUG("CharacterExtractor", QString("Loaded character row: id=%1, name=%2, role=%3, gender=%4, age=%5, hairColor=%6, hairStyle=%7, eyeColor=%8, build=%9, clothing=%10, features=%11, personality=%12, portraitPath=%13")
        .arg(character.id())
        .arg(character.name())
        .arg(character.role())
        .arg(app.gender)
        .arg(app.age)
        .arg(app.hairColor)
        .arg(app.hairStyle)
        .arg(app.eyeColor)
        .arg(app.build)
        .arg(app.clothing.join(", "))
        .arg(app.distinctiveFeatures.join(", "))
        .arg(character.personality().join(", "))
        .arg(character.portraitPath()));
    return character;
}

QList<Character> CharacterExtractor::getCharactersByNovel(const QString& novelId)
{
    if (BibleCache::instance()->hasCharacters(novelId)) {
        QList<Character> cached = BibleCache::instance()->getCharacters(novelId);
        QList<Character> filtered;
        for (const Character& character : cached) {
            if (!isGenericCharacterName(character.name())) {
                filtered.append(character);
            }
        }
        return filtered;
    }

    QList<Character> characters;
    QList<QVariantMap> results = DatabaseManager::instance()->selectAll(
        "characters", "novel_id = ?", QVariantList() << novelId);
    
    for (const QVariantMap& row : results) {
        Character character = characterFromRow(row);
        if (!isGenericCharacterName(character.name())) {
            characters.append(character);
        }
    }
    BibleCache::instance()->setCharacters(novelId, characters);
    return characters;
}

Character CharacterExtractor::getCharacterById(const QString& characterId)
{
    QVariantMap row = DatabaseManager::instance()->selectOne(
        "characters", "id = ?", QVariantList() << characterId);
    
    return row.isEmpty() ? Character() : characterFromRow(row);
}

Character CharacterExtractor::getCharacterByName(const QString& novelId, const QString& name)
{
    if (isGenericCharacterName(name)) {
        return Character();
    }

    QList<Character> characters = getCharactersByNovel(novelId);
    
    for (const Character& character : characters) {
        if (matchesCharacterName(character, name)) {
            return character;
        }
    }
    
    return Character();
}

bool CharacterExtractor::updateCharacter(const Character& character)
{
    QVariantMap data = characterToData(character);

    bool success = DatabaseManager::instance()->update(
        "characters", data, "id = ?", QVariantList() << character.id());
    
    if (success) {
        const CharacterAppearance app = character.appearance();
        LOG_INFO("CharacterExtractor", QString("Updated character: %1, gender=%2, age=%3, hairColor=%4, hairStyle=%5, eyeColor=%6, build=%7, portraitPath=%8")
            .arg(character.name())
            .arg(app.gender)
            .arg(app.age)
            .arg(app.hairColor)
            .arg(app.hairStyle)
            .arg(app.eyeColor)
            .arg(app.build)
            .arg(character.portraitPath()));
        BibleCache::instance()->invalidateCharacters(character.novelId());
        emit characterUpdated(character.id(), character.portraitPath());
    }
    
    return success;
}

bool CharacterExtractor::updateCharacterSilent(const Character& character)
{
    QVariantMap data = characterToData(character);

    bool success = DatabaseManager::instance()->update(
        "characters", data, "id = ?", QVariantList() << character.id());
    
    if (success) {
        LOG_INFO("CharacterExtractor", QString("Updated character (silent): %1").arg(character.name()));
        BibleCache::instance()->invalidateCharacters(character.novelId());
    }
    
    return success;
}

bool CharacterExtractor::deleteCharacter(const QString& characterId)
{
    QString novelId;
    bool success = DatabaseManager::instance()->transaction([&]() -> bool {
        QVariantMap row = DatabaseManager::instance()->selectOne(
            "characters", "id = ?", QVariantList() << characterId);
        novelId = row["novel_id"].toString();

        return DatabaseManager::instance()->remove(
            "characters", "id = ?", QVariantList() << characterId);
    });
    
    if (success) {
        LOG_INFO("CharacterExtractor", QString("Deleted character: %1").arg(characterId));
        if (!novelId.isEmpty()) {
            BibleCache::instance()->invalidateCharacters(novelId);
        }
    }
    
    return success;
}
