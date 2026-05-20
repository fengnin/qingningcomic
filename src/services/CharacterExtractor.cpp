#include "services/CharacterExtractor.h"
#include "utils/BibleCache.h"
#include "data/DatabaseManager.h"
#include "data/FileStorage.h"
#include "utils/Logger.h"
#include "utils/JsonUtils.h"
#include "utils/BibleUtils.h"
#include "utils/EncodingUtils.h"
#include <QDateTime>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QSet>
#include <QUuid>

namespace {
QString normalizeExtractedName(const QString& name);
QString kinshipSignatureForCharacter(const QString& name, const QString& role);
int parseAgeFromJson(const QJsonValue& ageValue);
QString extractCharacterContext(const QString& sourceText, const QString& name, int radius = 80);
QString matchFirstCapture(const QString& text, const QList<QRegularExpression>& patterns);
QStringList collectKeywords(const QString& text, const QStringList& keywords);
bool isPronounLikeCharacter(const QString& name);
bool isGenericHairColor(const QString& hairColor);
bool isVariantCharacterName(const QString& name);
bool isGenericCharacterName(const QString& name);
int purgeGenericCharacterRows(const QString& novelId);
bool isCharacterSubject(const QString& sentence, const QString& name);

const QStringList& getCharacterNameSuffixes()
{
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
    return suffixes;
}

QString normalizedCharacterKey(const QString& name)
{
    return normalizeExtractedName(name).trimmed();
}

QString composeCharacterFeatureText(const QString& expression, const QString& pose)
{
    QStringList parts;
    if (!expression.isEmpty()) {
        parts << expression;
    }
    if (!pose.isEmpty()) {
        parts << pose;
    }
    return parts.join(" ").trimmed();
}

void normalizeAndFilterExtractedName(ExtractedCharacter& extracted)
{
    extracted.name = normalizeExtractedName(extracted.name);
    if (isVariantCharacterName(extracted.name) || isGenericCharacterName(extracted.name)) {
        extracted.name.clear();
    }
}

void appendUnique(QStringList& list, const QString& value)
{
    const QString trimmed = value.trimmed();
    if (!trimmed.isEmpty() && !list.contains(trimmed)) {
        list << trimmed;
    }
}

template<typename T>
bool resolveFieldUpdate(const T& target, const T& incoming,
                        const QJsonObject& existingSources, const QJsonObject& incomingSources,
                        const QString& key, QJsonObject& fieldSources, QString& outSource)
{
    Q_UNUSED(incoming);
    Q_UNUSED(fieldSources);
    
    const QString existingSource = existingSources.value(key).toString();
    const QString incomingSource = incomingSources.value(key).toString();
    if (!BibleUtils::BibleUpdateStrategy::canAdoptFieldValue(target, existingSource, incomingSource)) {
        return false;
    }
    outSource = BibleUtils::BibleUpdateStrategy::normalizedSource(incomingSource, QStringLiteral("inferred"));
    return true;
}

void updateFieldIfAllowed(QString& target, const QString& incoming,
                          const QJsonObject& existingSources, const QJsonObject& incomingSources,
                          const QString& key, QJsonObject& fieldSources)
{
    if (incoming.isEmpty() || incoming == target) {
        return;
    }
    QString src;
    if (resolveFieldUpdate(target, incoming, existingSources, incomingSources, key, fieldSources, src)) {
        target = incoming;
        fieldSources[key] = src;
    }
}

void updateIntFieldIfAllowed(int& target, int incoming,
                             const QJsonObject& existingSources, const QJsonObject& incomingSources,
                             const QString& key, QJsonObject& fieldSources)
{
    if (incoming <= 0 || incoming == target) {
        return;
    }
    QString src;
    if (resolveFieldUpdate(target, incoming, existingSources, incomingSources, key, fieldSources, src)) {
        target = incoming;
        fieldSources[key] = src;
    }
}

void updateListFieldIfAllowed(QStringList& target,
                              const QStringList& incoming,
                              const QJsonObject& existingSources,
                              const QJsonObject& incomingSources,
                              const QString& key,
                              QJsonObject& fieldSources)
{
    if (incoming.isEmpty()) {
        return;
    }

    QString src;
    if (!resolveFieldUpdate(target, incoming, existingSources, incomingSources, key, fieldSources, src)) {
        return;
    }

    const QString incomingSource = incomingSources.value(key).toString();
    target = (target.isEmpty() || BibleUtils::BibleUpdateStrategy::isExplicitSource(incomingSource) ||
              BibleUtils::BibleUpdateStrategy::isManualSource(incomingSource))
        ? incoming
        : BibleUtils::mergeStringLists(target, incoming);
    fieldSources[key] = src;
}

void mergeCharacterAppearance(CharacterAppearance& app, const ExtractedCharacter& incoming)
{
    const QJsonObject existingSources = app.fieldSources;
    const QJsonObject incomingSources = incoming.fieldSources;

    updateFieldIfAllowed(app.gender, incoming.gender, existingSources, incomingSources, "gender", app.fieldSources);
    updateIntFieldIfAllowed(app.age, incoming.age, existingSources, incomingSources, "age", app.fieldSources);
    updateFieldIfAllowed(app.hairColor, incoming.hairColor, existingSources, incomingSources, "hairColor", app.fieldSources);
    updateFieldIfAllowed(app.hairStyle, incoming.hairStyle, existingSources, incomingSources, "hairStyle", app.fieldSources);
    updateFieldIfAllowed(app.eyeColor, incoming.eyeColor, existingSources, incomingSources, "eyeColor", app.fieldSources);
    updateFieldIfAllowed(app.build, incoming.bodyType, existingSources, incomingSources, "build", app.fieldSources);
    updateFieldIfAllowed(app.height, incoming.height, existingSources, incomingSources, "height", app.fieldSources);

    updateListFieldIfAllowed(app.clothing, incoming.clothing, existingSources, incomingSources, "clothing", app.fieldSources);
    updateListFieldIfAllowed(app.accessories, incoming.accessories, existingSources, incomingSources, "accessories", app.fieldSources);
    updateListFieldIfAllowed(app.distinctiveFeatures, incoming.distinctiveFeatures, existingSources, incomingSources, "distinctiveFeatures", app.fieldSources);
}

QStringList mergeCharacterAliases(const Character& existing, const ExtractedCharacter& incoming, CharacterAppearance& app)
{
    QStringList aliases = existing.aliases();
    const QJsonObject existingSources = app.fieldSources;
    const QJsonObject incomingSources = incoming.fieldSources;

    updateListFieldIfAllowed(aliases, incoming.aliases, existingSources, incomingSources, "aliases", app.fieldSources);
    if (!BibleUtils::BibleUpdateStrategy::isLockedSource(existingSources.value("aliases").toString())) {
        appendUnique(aliases, incoming.name);
    }

    return aliases;
}

void mergeCharacterRole(Character& merged, const ExtractedCharacter& incoming)
{
    if (!merged.role().isEmpty()) {
        return;
    }

    if (!incoming.role.isEmpty()) {
        merged.setRole(incoming.role);
        return;
    }

    if (incoming.tags.isEmpty()) {
        return;
    }

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
            return;
        }
    }

    merged.setRole(incoming.tags.first());
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

bool appendExtractedCharacter(QList<ExtractedCharacter>& result, QSet<QString>& seenNames, const ExtractedCharacter& extracted)
{
    if (extracted.name.isEmpty() || seenNames.contains(extracted.name)) {
        return false;
    }

    seenNames.insert(extracted.name);
    result.append(extracted);
    return true;
}

bool containsChineseCharacters(const QString& text)
{
    return text.contains(QRegularExpression(QStringLiteral("\\p{Han}")));
}

QString extractCharacterContext(const QString& sourceText, const QString& name, int radius)
{
    Q_UNUSED(radius);

    if (sourceText.isEmpty() || name.isEmpty()) {
        return QString();
    }

    const QStringList paragraphs = sourceText.split(
        QRegularExpression(QStringLiteral("(?:\\r?\\n\\s*){2,}")),
        Qt::SkipEmptyParts);

    QStringList matchedParagraphs;
    for (const QString& paragraph : paragraphs) {
        if (!paragraph.contains(name)) continue;
        const QString trimmed = paragraph.trimmed();
        if (!trimmed.isEmpty() && !matchedParagraphs.contains(trimmed)) {
            matchedParagraphs << trimmed;
        }
    }

    if (!matchedParagraphs.isEmpty()) {
        return matchedParagraphs.join(QStringLiteral("\n\n"));
    }

    return QString();
}

struct CharacterSubjectPatterns {
    QRegularExpression nonSubj1, nonSubj2, nonSubj3;
    QRegularExpression appSubj1, appSubj2, appSubj3, appSubj4, appSubj5, appSubj6;

    explicit CharacterSubjectPatterns(const QString& escaped)
        : nonSubj1(QString::fromUtf8("(?:看向|望着|对着|听见|看见|迎接|欢迎|目送|注视|观察|打量)[^，。！？]{0,10}%1").arg(escaped))
        , nonSubj2(QString::fromUtf8("(?:对|向|给|为|替|帮|同|和|与)[^，。！？]{0,10}%1(?:说|讲|问|笑|点头)").arg(escaped))
        , nonSubj3(QString::fromUtf8("[^，。！？]{0,15}的%1(?:攥着|提着|拿着|抱着|背着|戴着|穿着|系着|围着|套着)").arg(escaped))
        , appSubj1(QString::fromUtf8("%1[^，。！？]{0,10}(?:穿着|戴着|系着|围着|披着|裹着|扎着|梳着|留着|染了|长了|有着|长着|带着|是一头|留了一头|顶着一头|一头)").arg(escaped))
        , appSubj2(QString::fromUtf8("^%1(?:是|为|叫|名|姓|今年|是个|身为|作为)").arg(escaped))
        , appSubj3(QString::fromUtf8("^(?:这|那|该)(?:个?位?)%1[^，。]{0,25}(?:是|有|带着|长着|留着|梳着|染了)").arg(escaped))
        , appSubj4(QString::fromUtf8("%1[^，。！？]{0,20}(?:满头|一头|白发|银发|花白|黑发|短发|长发)").arg(escaped))
        , appSubj5(QString::fromUtf8("%1的[^，。！？]{0,6}(?:发色|头发|发丝|鬓发|眼睛|眼眸|眸子|双眸|瞳仁|瞳孔|眸|眼|身形|体型|身材|发型|发质)").arg(escaped))
        , appSubj6(QString::fromUtf8("%1[^，。！？]{0,20}(?:他|她|其)的(?:发色|头发|发丝|鬓发|眼睛|眼眸|眸子|双眸|瞳仁|瞳孔|眸|眼|身形|体型|身材|发型|发质)").arg(escaped))
    {}
};

// 按角色名缓存编译好的正则组，避免同一次提取中重复编译
static QHash<QString, CharacterSubjectPatterns*> s_subjectPatternsCache;

bool isCharacterSubject(const QString& sentence, const QString& name)
{
    CharacterSubjectPatterns* p = s_subjectPatternsCache.value(name, nullptr);
    if (!p) {
        p = new CharacterSubjectPatterns(QRegularExpression::escape(name));
        s_subjectPatternsCache.insert(name, p);
    }

    if (p->nonSubj1.match(sentence).hasMatch()) return false;
    if (p->nonSubj2.match(sentence).hasMatch()) return false;
    if (p->nonSubj3.match(sentence).hasMatch()) return false;

    if (p->appSubj1.match(sentence).hasMatch()) return true;
    if (p->appSubj2.match(sentence).hasMatch()) return true;
    if (p->appSubj3.match(sentence).hasMatch()) return true;
    if (p->appSubj4.match(sentence).hasMatch()) return true;
    if (p->appSubj5.match(sentence).hasMatch()) return true;
    if (p->appSubj6.match(sentence).hasMatch()) return true;

    return false;
}

QString matchFirstCapture(const QString& text, const QList<QRegularExpression>& patterns)
{
    QString bestMatch;
    for (const QRegularExpression& pattern : patterns) {
        QRegularExpressionMatchIterator it = pattern.globalMatch(text);
        while (it.hasNext()) {
            const QRegularExpressionMatch match = it.next();
            const QString captured = match.captured(1).trimmed();
            if (captured.isEmpty()) {
                continue;
            }
            if (captured.length() > bestMatch.length()) {
                bestMatch = captured;
            }
        }
    }
    return bestMatch;
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

            if (DatabaseManager::instance()->remove("characters", "id = ?", QVariantList() << id)) {
                removedCount++;
            }
        }
        return true;
    });

    return cleaned ? removedCount : 0;
}

QString normalizeFieldText(const QString& value)
{
    return EncodingUtils::fixGarbledString(value).trimmed();
}

// LLM 有时把字段名写进值里，如 hairStyle="发型是利落的短发"、build="体型偏瘦"
// 去掉已知字段名前缀，避免拼接时出现"白色发型是利落的短发"
static const QMap<QString, QStringList>& fieldLabelPrefixes()
{
    static const QMap<QString, QStringList> map = {
        { QStringLiteral("hairStyle"), { QStringLiteral("发型是"), QStringLiteral("发型为"), QStringLiteral("发型："), QStringLiteral("发型:") } },
        { QStringLiteral("hairColor"), { QStringLiteral("发色是"), QStringLiteral("发色为"), QStringLiteral("发色："), QStringLiteral("发色:") } },
        { QStringLiteral("eyeColor"),  { QStringLiteral("眼色是"), QStringLiteral("眼色为"), QStringLiteral("眼色："), QStringLiteral("眼色:") } },
        { QStringLiteral("build"),     { QStringLiteral("体型是"), QStringLiteral("体型为"), QStringLiteral("体型："), QStringLiteral("体型:") } },
        { QStringLiteral("height"),    { QStringLiteral("身高是"), QStringLiteral("身高为"), QStringLiteral("身高："), QStringLiteral("身高:") } },
    };
    return map;
}

QString stripFieldLabelPrefix(const QString& key, const QString& value)
{
    const auto& map = fieldLabelPrefixes();
    if (!map.contains(key)) return value;
    for (const QString& prefix : map.value(key)) {
        if (value.startsWith(prefix)) {
            return value.mid(prefix.length()).trimmed();
        }
    }
    return value;
}

void setFieldSourceIfPresent(ExtractedCharacter& extracted, const QString& key, const QString& source)
{
    if (!source.isEmpty()) {
        extracted.fieldSources[key] = source;
    }
}

QStringList normalizeTextList(const QStringList& values)
{
    QStringList result;
    for (const QString& value : values) {
        const QString normalized = normalizeFieldText(value);
        if (!normalized.isEmpty() && !result.contains(normalized)) {
            result << normalized;
        }
    }
    return result;
}

void assignParsedStringField(ExtractedCharacter& extracted, const QString& key, QString& field, const QString& value,
                             const QString& source = QStringLiteral("inferred"))
{
    const QString normalized = stripFieldLabelPrefix(key, normalizeFieldText(value));
    if (normalized.isEmpty()) {
        return;
    }

    field = normalized;
    setFieldSourceIfPresent(extracted, key, source);
}

void assignParsedListField(ExtractedCharacter& extracted, const QString& key, QStringList& field, const QStringList& values,
                           const QString& source = QStringLiteral("inferred"))
{
    const QStringList normalized = normalizeTextList(values);
    if (normalized.isEmpty()) {
        return;
    }

    field = normalized;
    setFieldSourceIfPresent(extracted, key, source);
}

void assignTextMatchedStringField(ExtractedCharacter& extracted, const QString& key, QString& field,
                                  const QString& candidate, bool replaceIfGeneric = false,
                                  const QString& source = QStringLiteral("inferred"))
{
    const QString normalized = normalizeFieldText(candidate);
    if (normalized.isEmpty()) {
        return;
    }

    const QString existingSource = extracted.fieldSources.value(key).toString();
    if (!BibleUtils::BibleUpdateStrategy::shouldUpdateFromSource(existingSource, source) &&
        !(source.compare(QStringLiteral("explicit"), Qt::CaseInsensitive) == 0 &&
          !field.isEmpty() &&
          replaceIfGeneric &&
          isGenericHairColor(field))) {
        return;
    }

    if (field == normalized) {
        setFieldSourceIfPresent(extracted, key, source);
        return;
    }

    field = normalized;
    setFieldSourceIfPresent(extracted, key, source);
}

QString detectGenderFromText(const QString& text)
{
    if (text.isEmpty()) {
        return QString();
    }

    static const QStringList femaleMarkers = {
        QString::fromUtf8("女性"), QString::fromUtf8("女生"), QString::fromUtf8("少女"),
        QString::fromUtf8("姑娘"), QString::fromUtf8("女士"), QString::fromUtf8("女孩子"),
        QString::fromUtf8("女孩"), QString::fromUtf8("太太")
    };

    static const QStringList maleMarkers = {
        QString::fromUtf8("男性"), QString::fromUtf8("男生"), QString::fromUtf8("少年"),
        QString::fromUtf8("先生"), QString::fromUtf8("男人"), QString::fromUtf8("男孩")
    };

    for (const QString& marker : femaleMarkers) {
        if (text.contains(marker)) return QString::fromUtf8("女");
    }
    for (const QString& marker : maleMarkers) {
        if (text.contains(marker)) return QString::fromUtf8("男");
    }

    return QString();
}

QString parseChineseAgeText(const QString& ageText)
{
    static const QHash<QChar, int> digitMap = {
        {QChar(0x96F6), 0},
        {QChar(0x3007), 0},
        {QChar(0x4E00), 1},
        {QChar(0x4E8C), 2},
        {QChar(0x4E09), 3},
        {QChar(0x56DB), 4},
        {QChar(0x4E94), 5},
        {QChar(0x516D), 6},
        {QChar(0x4E03), 7},
        {QChar(0x516B), 8},
        {QChar(0x4E5D), 9}
    };

    const QString trimmed = ageText.trimmed();
    if (trimmed.isEmpty()) {
        return QString();
    }

    if (trimmed == QString::fromUtf8("十")) {
        return QStringLiteral("10");
    }

    const int tenIndex = trimmed.indexOf(QString::fromUtf8("十"));
    if (tenIndex < 0) {
        if (trimmed.size() == 1 && digitMap.contains(trimmed.front())) {
            return QString::number(digitMap.value(trimmed.front()));
        }
        return QString();
    }

    int tens = 1;
    if (tenIndex > 0 && digitMap.contains(trimmed.at(0))) {
        tens = digitMap.value(trimmed.at(0));
    }

    int ones = 0;
    if (tenIndex + 1 < trimmed.size() && digitMap.contains(trimmed.at(tenIndex + 1))) {
        ones = digitMap.value(trimmed.at(tenIndex + 1));
    }

    return QString::number(tens * 10 + ones);
}

int parseAgeText(const QString& ageText)
{
    const QString normalized = normalizeFieldText(ageText);
    if (normalized.isEmpty()) {
        return 0;
    }

    bool ok = false;
    const int directAge = normalized.toInt(&ok);
    if (ok && directAge > 0) {
        return directAge;
    }

    static const QList<QRegularExpression> patterns = {
        QRegularExpression(QStringLiteral(R"((\d{1,2})(?:多|余)?\s*岁(?:左右|上下|出头)?)")),
        QRegularExpression(QStringLiteral(R"(([一二三四五六七八九十〇零]{1,3})(?:多|余)?\s*岁(?:左右|上下|出头)?)")),
        QRegularExpression(QStringLiteral(R"(约?\s*(\d{1,2})\s*岁)"))
    };

    for (const QRegularExpression& pattern : patterns) {
        const QRegularExpressionMatch match = pattern.match(normalized);
        if (!match.hasMatch()) {
            continue;
        }

        const QString captured = match.captured(1);
        const int numericAge = captured.toInt(&ok);
        if (ok && numericAge > 0) {
            return numericAge;
        }

        const QString chineseAge = parseChineseAgeText(captured);
        if (!chineseAge.isEmpty()) {
            const int parsed = chineseAge.toInt(&ok);
            if (ok && parsed > 0) {
                return parsed;
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
        {QString::fromUtf8("五十来岁"), 53},
        {QString::fromUtf8("六十来岁"), 63},
        {QString::fromUtf8("七十来岁"), 73}
    };

    for (auto it = ageRangeMapping.constBegin(); it != ageRangeMapping.constEnd(); ++it) {
        if (normalized.contains(it.key())) {
            return it.value();
        }
    }

    // 描述性年龄映射（将文字描述转换为代表性数值）
    static const QMap<QString, int> descriptiveAgeMapping = {
        // 婴幼儿阶段
        {QString::fromUtf8("婴儿"), 1},
        {QString::fromUtf8("幼年"), 5},
        {QString::fromUtf8("幼童"), 6},
        {QString::fromUtf8("孩童"), 8},
        
        // 少年/儿童阶段
        {QString::fromUtf8("少年"), 12},
        {QString::fromUtf8("儿童"), 10},
        {QString::fromUtf8("小孩"), 8},
        {QString::fromUtf8("小姑娘"), 10},
        {QString::fromUtf8("小男孩"), 10},
        
        // 青年阶段
        {QString::fromUtf8("青年"), 22},
        {QString::fromUtf8("年轻"), 25},
        {QString::fromUtf8("年轻人"), 25},
        {QString::fromUtf8("少女"), 16},
        {QString::fromUtf8("少男"), 16},
        {QString::fromUtf8("姑娘"), 18},
        {QString::fromUtf8("小伙子"), 20},
        {QString::fromUtf8("青春"), 20},
        
        // 中年阶段
        {QString::fromUtf8("中年"), 45},
        {QString::fromUtf8("中年人"), 45},
        {QString::fromUtf8("壮年"), 40},
        {QString::fromUtf8("而立之年"), 30},
        {QString::fromUtf8("不惑之年"), 40},
        {QString::fromUtf8("知天命"), 50},
        
        // 老年阶段
        {QString::fromUtf8("老年"), 70},
        {QString::fromUtf8("老年人"), 70},
        {QString::fromUtf8("老人"), 72},
        {QString::fromUtf8("老者"), 75},
        {QString::fromUtf8("长者"), 70},
        {QString::fromUtf8("高龄"), 80},
        {QString::fromUtf8("古稀"), 70},
        {QString::fromUtf8("耄耋"), 85},
        {QString::fromUtf8("花甲之年"), 60},
        {QString::fromUtf8("耳顺之年"), 60}
    };

    int containsMatch = 0;
    for (auto it = descriptiveAgeMapping.constBegin(); it != descriptiveAgeMapping.constEnd(); ++it) {
        if (normalized == it.key()) {
            return it.value();
        }
        if (!containsMatch && normalized.contains(it.key())) {
            containsMatch = it.value();
        }
    }
    if (containsMatch) {
        return containsMatch;
    }

    return 0;
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
        QString::fromUtf8("中年"),
        QString::fromUtf8("大人"),
        QString::fromUtf8("路人"),
        QString::fromUtf8("男子"),
        QString::fromUtf8("女子"),
        QString::fromUtf8("男孩"),
        QString::fromUtf8("女孩"),
        QString::fromUtf8("男生"),
        QString::fromUtf8("女生")
    };

    if (genericLabels.contains(normalized)) {
        return true;
    }

    static const QStringList genericTokens = {
        QString::fromUtf8("某人"),
        QString::fromUtf8("这个人"),
        QString::fromUtf8("那个人"),
        QString::fromUtf8("一个人"),
        QString::fromUtf8("一名"),
        QString::fromUtf8("一位"),
        QString::fromUtf8("人影"),
        QString::fromUtf8("身影")
    };

    for (const QString& token : genericTokens) {
        if (normalized.contains(token)) {
            return true;
        }
    }

    return false;
}

QString normalizeExtractedName(const QString& name)
{
    QString normalized = name.trimmed();

    for (const QString& suffix : getCharacterNameSuffixes()) {
        if (normalized.endsWith(suffix, Qt::CaseInsensitive)) {
            normalized.chop(suffix.length());
            normalized = normalized.trimmed();
            break;
        }
    }

    return normalized;
}

struct KinshipRelationRule {
    QStringList tokens;
    QString relationKey;
};

const QList<KinshipRelationRule>& kinshipRules()
{
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
    return rules;
}

QString kinshipRelationKey(const QString& name)
{
    const QString normalized = normalizedCharacterKey(name);
    if (normalized.isEmpty()) {
        return QString();
    }

    for (const KinshipRelationRule& rule : kinshipRules()) {
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

    int bestPos = -1;
    int bestTokenLength = 0;
    for (const KinshipRelationRule& rule : kinshipRules()) {
        for (const QString& token : rule.tokens) {
            const int pos = normalized.indexOf(token);
            if (pos <= 0) {
                continue;
            }
            if (bestPos < 0 || pos < bestPos || (pos == bestPos && token.length() > bestTokenLength)) {
                bestPos = pos;
                bestTokenLength = token.length();
            }
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

int parseAgeFromJson(const QJsonValue& ageValue)
{
    if (ageValue.isDouble()) {
        return ageValue.toInt();
    }

    if (ageValue.isString()) {
        const QString ageStr = normalizeFieldText(ageValue.toString());
        const int parsedAge = parseAgeText(ageStr);
        if (parsedAge > 0) {
            return parsedAge;
        }
    }

    return 0;
}

QString readStringField(const QJsonObject& primary, const QJsonObject& fallback, const QString& key)
{
    QString value = primary.value(key).toString();
    if (value.isEmpty()) {
        value = fallback.value(key).toString();
    }
    return EncodingUtils::fixGarbledString(value.trimmed());
}

QStringList readStringListField(const QJsonObject& primary, const QJsonObject& fallback, const QString& key)
{
    QStringList value = JsonUtils::jsonArrayToStringList(primary.value(key).toArray());
    if (value.isEmpty()) {
        value = JsonUtils::jsonArrayToStringList(fallback.value(key).toArray());
    }
    for (QString& item : value) {
        item = EncodingUtils::fixGarbledString(item.trimmed());
    }
    return value;
}

QJsonObject extractAppearanceObject(const QJsonObject& charObj)
{
    QJsonObject appObj = charObj.value("appearance").toObject();
    if (!appObj.isEmpty()) {
        return appObj;
    }

    if (charObj.value("appearance").isString()) {
        QJsonParseError parseError;
        const QJsonDocument appearanceDoc =
            QJsonDocument::fromJson(charObj.value("appearance").toString().toUtf8(), &parseError);
        if (parseError.error == QJsonParseError::NoError && appearanceDoc.isObject()) {
            return appearanceDoc.object();
        }
    }

    return charObj;
}

void applyCharacterFields(ExtractedCharacter& extracted,
                          const QJsonObject& appObj,
                          const QJsonObject& charObj)
{
    assignParsedStringField(extracted, QStringLiteral("gender"), extracted.gender, readStringField(appObj, charObj, "gender"));
    const QJsonValue ageValue = appObj.contains("age") && !appObj.value("age").isNull()
        ? appObj.value("age")
        : charObj.value("age");
    const int parsedAge = parseAgeFromJson(ageValue);
    if (parsedAge > 0) {
        extracted.age = parsedAge;
        setFieldSourceIfPresent(extracted, QStringLiteral("age"), QStringLiteral("inferred"));
    }
    assignParsedStringField(extracted, QStringLiteral("hairColor"), extracted.hairColor, readStringField(appObj, charObj, "hairColor"));
    assignParsedStringField(extracted, QStringLiteral("hairStyle"), extracted.hairStyle, readStringField(appObj, charObj, "hairStyle"));
    assignParsedStringField(extracted, QStringLiteral("eyeColor"), extracted.eyeColor, readStringField(appObj, charObj, "eyeColor"));
    assignParsedStringField(extracted, QStringLiteral("build"), extracted.bodyType, readStringField(appObj, charObj, "build"));
    assignParsedStringField(extracted, QStringLiteral("height"), extracted.height, readStringField(appObj, charObj, "height"));
    assignParsedListField(extracted, QStringLiteral("clothing"), extracted.clothing, readStringListField(appObj, charObj, "clothing"));
    assignParsedListField(extracted, QStringLiteral("accessories"), extracted.accessories, readStringListField(appObj, charObj, "accessories"));
    assignParsedListField(extracted, QStringLiteral("distinctiveFeatures"), extracted.distinctiveFeatures, readStringListField(appObj, charObj, "distinctiveFeatures"));
    assignParsedListField(extracted, QStringLiteral("aliases"), extracted.aliases, readStringListField(appObj, charObj, "aliases"));

    QStringList personalityValues = JsonUtils::jsonArrayToStringList(charObj.value("personality").toArray());
    if (personalityValues.isEmpty()) {
        personalityValues = JsonUtils::jsonArrayToStringList(charObj.value("personalities").toArray());
    }
    personalityValues = normalizeTextList(personalityValues);
    if (!personalityValues.isEmpty()) {
        extracted.personality = personalityValues;
        setFieldSourceIfPresent(extracted, QStringLiteral("personality"), QStringLiteral("inferred"));
    }

    const QStringList tagValues = normalizeTextList(JsonUtils::jsonArrayToStringList(charObj.value("tags").toArray()));
    if (!tagValues.isEmpty()) {
        extracted.tags = tagValues;
        setFieldSourceIfPresent(extracted, QStringLiteral("tags"), QStringLiteral("inferred"));
    }
}

void enrichCharacterAppearanceFromText(ExtractedCharacter& extracted, const QString& context)
{
    static const QList<QRegularExpression> hairColorPatterns = {
        QRegularExpression(QString::fromUtf8("((?:白发|银发|灰发|花白发|黑发|金发|栗发|白色头发|黑色头发)|(?:浅亚麻|亚麻|浅棕|深棕|棕褐|黑褐|乌黑|黑|灰白|花白|银白|银灰|金|栗|纯白|雪白|白)[色]?)[^，。；\\n]{0,8}(?:低马尾|高马尾|马尾|短发|长发|卷发|碎发|头发|发丝|鬓发)")),
        QRegularExpression(QString::fromUtf8("(?:发色|头发|发丝|鬓发)[^，。；\\n]{0,6}(?:是|为|呈|染成)?((?:浅亚麻|亚麻|浅棕|深棕|棕褐|黑褐|乌黑|黑|灰白|花白|银白|银灰|金|栗|纯白|雪白|白)[色]?|白发|银发|灰发|花白发)"))
    };

    static const QList<QRegularExpression> hairStylePatterns = {
        QRegularExpression(QString::fromUtf8("(低马尾|高马尾|马尾|短发|长发|卷发|齐肩短发|齐耳短发|碎发|丸子头)")),
        QRegularExpression(QString::fromUtf8("((?:及肩|及腰|齐肩|齐耳|超短|超长|蓬松|凌乱|整齐|利落|柔顺|自然|微卷|直|卷)[^，。；\\n]{0,4}(?:短发|长发|马尾|发))"))
    };

    static const QList<QRegularExpression> eyeColorPatterns = {
        QRegularExpression(QString::fromUtf8("((?:浅|深)?(?:琥珀|褐|棕|黑|灰|蓝|绿|金|银|茶|墨|琉璃)色?)[^，。；\\n]{0,6}(?:的)?(?:眼睛|眼眸|眸子|双眸|瞳仁|瞳孔|眸|眼)")),
        QRegularExpression(QString::fromUtf8("(?:眼睛|眼眸|眸子|双眸|瞳仁|瞳孔|眸|眼)[^，。；\\n]{0,6}(?:是|为|呈|带着|透着)?((?:浅|深)?(?:琥珀|褐|棕|黑|灰|蓝|绿|金|银|茶|墨|琉璃)色?)"))
    };

    static const QList<QRegularExpression> bodyTypePatterns = {
        QRegularExpression(QString::fromUtf8("((?:身形|体型|身材|身躯)[^，。；\\n]{0,6}(?:纤细|偏瘦|清瘦|瘦削|瘦高|高挑|苗条|单薄|健壮|结实|修长|偏高|偏矮))")),
        QRegularExpression(QString::fromUtf8("((?:纤细|偏瘦|清瘦|瘦削|瘦高|高挑|苗条|单薄|健壮|结实|修长)[^，。；\\n]{0,6}(?:身形|体型|身材|身躯)?)"))
    };

    static const QStringList clothingKeywords = {
        QString::fromUtf8("白色T恤"),
        QString::fromUtf8("白T恤"),
        QString::fromUtf8("白色T"),
        QString::fromUtf8("纯白T恤"),
        QString::fromUtf8("白色短袖衬衫"),
        QString::fromUtf8("浅青色小领结"),
        QString::fromUtf8("浅灰色百褶短裙"),
        QString::fromUtf8("浅蓝色牛仔裤"),
        QString::fromUtf8("浅蓝牛仔裤"),
        QString::fromUtf8("牛仔裤"),
        QString::fromUtf8("白色帆布鞋"),
        QString::fromUtf8("帆布鞋"),
        QString::fromUtf8("围裙"),
        QString::fromUtf8("衬衫"),
        QString::fromUtf8("长裙"),
        QString::fromUtf8("藏青色斜襟棉布衫"),
        QString::fromUtf8("深灰色宽松长裤"),
        QString::fromUtf8("黑色千层底布鞋")
    };

    static const QStringList distinctiveFeaturesKeywords = {
        QString::fromUtf8("虎牙"),
        QString::fromUtf8("酒窝"),
        QString::fromUtf8("泪痣"),
        QString::fromUtf8("小雀斑"),
        QString::fromUtf8("雀斑"),
        QString::fromUtf8("伤痕"),
        QString::fromUtf8("疤痕"),
        QString::fromUtf8("眼镜"),
        QString::fromUtf8("黑痣"),
        QString::fromUtf8("皱纹")
    };

    static const QStringList accessoriesKeywords = {
        QString::fromUtf8("凉茶"),
        QString::fromUtf8("旧诗集")
    };

    if (extracted.hairColor.isEmpty() || isGenericHairColor(extracted.hairColor)) {
        // 只从角色作为主语的句子里提取发色，避免把其他角色的发色归给目标角色
        QString subjectHairColor;
        const QStringList hairSentences = context.split(QRegularExpression(QStringLiteral("[。！？]")), Qt::SkipEmptyParts);
        for (const QString& sentence : hairSentences) {
            const QString trimmed = sentence.trimmed();
            if (trimmed.isEmpty() || !isCharacterSubject(trimmed, extracted.name)) continue;
            const QString matched = matchFirstCapture(trimmed, hairColorPatterns);
            if (!matched.isEmpty()) {
                subjectHairColor = matched;
                break;
            }
        }
        if (!subjectHairColor.isEmpty()) {
            assignTextMatchedStringField(extracted, QStringLiteral("hairColor"), extracted.hairColor,
                                         subjectHairColor, true, QStringLiteral("explicit"));
        }
    }
    {
        // 按句子过滤，只从角色作为主语的句子里提取发型和眼色，避免跨角色污染
        const QStringList sentences = context.split(QRegularExpression(QStringLiteral("[。！？]")), Qt::SkipEmptyParts);
        QString subjectHairStyle;
        QString subjectEyeColor;
        for (const QString& sentence : sentences) {
            const QString trimmed = sentence.trimmed();
            if (trimmed.isEmpty() || !isCharacterSubject(trimmed, extracted.name)) continue;
            if (subjectHairStyle.isEmpty()) {
                subjectHairStyle = matchFirstCapture(trimmed, hairStylePatterns);
            }
            if (subjectEyeColor.isEmpty()) {
                subjectEyeColor = matchFirstCapture(trimmed, eyeColorPatterns);
            }
            if (!subjectHairStyle.isEmpty() && !subjectEyeColor.isEmpty()) break;
        }
        assignTextMatchedStringField(extracted, QStringLiteral("hairStyle"), extracted.hairStyle,
                                     subjectHairStyle, false, QStringLiteral("explicit"));
        assignTextMatchedStringField(extracted, QStringLiteral("eyeColor"), extracted.eyeColor,
                                     subjectEyeColor, false, QStringLiteral("explicit"));
    }
    assignTextMatchedStringField(extracted, QStringLiteral("build"), extracted.bodyType,
                                 matchFirstCapture(context, bodyTypePatterns),
                                 false,
                                 QStringLiteral("explicit"));

    if (extracted.clothing.isEmpty()) {
        extracted.clothing = collectKeywords(context, clothingKeywords);
    }

    if (extracted.distinctiveFeatures.isEmpty()) {
        extracted.distinctiveFeatures = collectKeywords(context, distinctiveFeaturesKeywords);
    }
    if (extracted.accessories.isEmpty()) {
        extracted.accessories = collectKeywords(context, accessoriesKeywords);
    }
}

void enrichCharacterCoreIdentityFromText(ExtractedCharacter& extracted, const QString& context)
{
    // 按句子拆分，只在角色是主语的句子里检测性别/年龄，避免把其他角色的描述归给目标角色
    const QStringList sentences = context.split(
        QRegularExpression(QStringLiteral("[。！？]")), Qt::SkipEmptyParts);

    QString detectedGender;
    int detectedAge = 0;
    for (const QString& sentence : sentences) {
        const QString trimmed = sentence.trimmed();
        if (trimmed.isEmpty() || !isCharacterSubject(trimmed, extracted.name)) {
            continue;
        }
        if (detectedGender.isEmpty()) {
            detectedGender = detectGenderFromText(trimmed);
        }
        if (detectedAge <= 0) {
            detectedAge = parseAgeText(trimmed);
        }
        if (!detectedGender.isEmpty() && detectedAge > 0) {
            break;
        }
    }

    assignTextMatchedStringField(extracted, QStringLiteral("gender"), extracted.gender,
                                 detectedGender, false, QStringLiteral("explicit"));

    const QString ageSource = extracted.fieldSources.value(QStringLiteral("age")).toString();
    const int parsedAge = detectedAge;
    if (parsedAge > 0 &&
        BibleUtils::BibleUpdateStrategy::shouldUpdateFromSource(ageSource, QStringLiteral("explicit"))) {
        extracted.age = parsedAge;
        setFieldSourceIfPresent(extracted, QStringLiteral("age"), QStringLiteral("explicit"));
    } else if (extracted.age <= 0 && parsedAge > 0 &&
               BibleUtils::BibleUpdateStrategy::shouldUpdateFromSource(ageSource, QStringLiteral("inferred"))) {
        extracted.age = parsedAge;
        setFieldSourceIfPresent(extracted, QStringLiteral("age"), QStringLiteral("inferred"));
    }
}

void enrichCharacterFromContext(ExtractedCharacter& extracted, const QString& context)
{
    enrichCharacterAppearanceFromText(extracted, context);
    enrichCharacterCoreIdentityFromText(extracted, context);
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
    json["build"] = bodyType;
    json["bodyType"] = bodyType;
    json["height"] = height;
    json["clothing"] = JsonUtils::stringListToJsonArray(clothing);
    json["accessories"] = JsonUtils::stringListToJsonArray(accessories);
    json["distinctiveFeatures"] = JsonUtils::stringListToJsonArray(distinctiveFeatures);
    json["aliases"] = JsonUtils::stringListToJsonArray(aliases);
    json["personality"] = JsonUtils::stringListToJsonArray(personality);
    json["tags"] = JsonUtils::stringListToJsonArray(tags);
    if (!fieldSources.isEmpty()) {
        json["fieldSources"] = fieldSources;
    }
    return json;
}

ExtractedCharacter ExtractedCharacter::fromJson(const QJsonObject& json)
{
    ExtractedCharacter c;
    c.name = json["name"].toString();
    c.role = json["role"].toString();
    c.gender = json["gender"].toString();
    c.age = parseAgeFromJson(json["age"]);
    c.hairColor = json["hairColor"].toString();
    c.hairStyle = json["hairStyle"].toString();
    c.eyeColor = json["eyeColor"].toString();
    c.bodyType = normalizeFieldText(json["bodyType"].toString());
    if (c.bodyType.isEmpty()) {
        c.bodyType = normalizeFieldText(json["build"].toString());
    }
    c.height = normalizeFieldText(json["height"].toString());
    c.clothing = normalizeTextList(JsonUtils::jsonArrayToStringList(json["clothing"].toArray()));
    c.accessories = normalizeTextList(JsonUtils::jsonArrayToStringList(json["accessories"].toArray()));
    c.distinctiveFeatures = normalizeTextList(JsonUtils::jsonArrayToStringList(json["distinctiveFeatures"].toArray()));
    c.aliases = normalizeTextList(JsonUtils::jsonArrayToStringList(json["aliases"].toArray()));
    c.personality = normalizeTextList(JsonUtils::jsonArrayToStringList(json["personality"].toArray()));
    c.tags = normalizeTextList(JsonUtils::jsonArrayToStringList(json["tags"].toArray()));
    c.fieldSources = json["fieldSources"].toObject();
    return c;
}

CharacterExtractor::CharacterExtractor(QObject* parent)
    : QObject(parent)
{
    qRegisterMetaType<CharacterFieldDiff>("CharacterFieldDiff");
    qRegisterMetaType<QList<CharacterFieldDiff>>("QList<CharacterFieldDiff>");
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
            if (appendExtractedCharacter(characters, seenNames, extracted)) {
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
        QString charStr = charVal.toString();
        int parenPos = charStr.indexOf('(');
        if (parenPos > 0) {
            extracted.name = charStr.left(parenPos).trimmed();
            int endParen = charStr.indexOf(')', parenPos);
            if (endParen > parenPos) {
                QString featureStr = charStr.mid(parenPos + 1, endParen - parenPos - 1);
                if (!featureStr.isEmpty()) {
                    extracted.distinctiveFeatures << featureStr;
                }
            }
        } else {
            extracted.name = charStr.trimmed();
        }
    } else if (charVal.isObject()) {
        QJsonObject charObj = charVal.toObject();
        extracted.name = normalizeExtractedName(charObj.value("name").toString());
        const QString featureStr = composeCharacterFeatureText(charObj["expression"].toString(),
                                                              charObj["pose"].toString());
        if (!featureStr.isEmpty()) {
            extracted.distinctiveFeatures << featureStr;
        }
    }

    normalizeAndFilterExtractedName(extracted);
    if (extracted.name.isEmpty()) {
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
    QSet<QString> seenNames;

    for (const QJsonValue& charVal : characters) {
        ExtractedCharacter extracted = parseAICharacter(charVal.toObject());
        enrichCharacterFromText(extracted, sourceText);

        if (!extracted.name.isEmpty() && containsChineseCharacters(extracted.name)) {
            if (appendExtractedCharacter(result, seenNames, extracted)) {
                emit characterExtracted(extracted);
            }
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
    const QString originalName = EncodingUtils::fixGarbledString(charObj["name"].toString());
    extracted.name = normalizeExtractedName(originalName);
    extracted.role = charObj.value("role").toString();
    extracted.role = EncodingUtils::fixGarbledString(extracted.role);

    if (extracted.name.isEmpty()) {
        return extracted;
    }

    normalizeAndFilterExtractedName(extracted);
    if (extracted.name.isEmpty()) {
        LOG_DEBUG("CharacterExtractor", QString("Skipping non-specific character: %1").arg(originalName));
        return extracted;
    }

    const QJsonObject appObj = extractAppearanceObject(charObj);
    LOG_DEBUG("CharacterExtractor", QString("Parsing character '%1', role=%2, appearance: %3")
        .arg(extracted.name)
        .arg(extracted.role)
        .arg(QString::fromUtf8(QJsonDocument(appObj).toJson(QJsonDocument::Compact))));

    applyCharacterFields(extracted, appObj, charObj);

    if (extracted.age <= 0) {
        const int roleBasedAge = BibleUtils::Inference::inferAge(extracted.role, extracted.name);
        if (roleBasedAge > 0) {
            extracted.age = roleBasedAge;
            setFieldSourceIfPresent(extracted, QStringLiteral("age"), QStringLiteral("inferred"));
            LOG_DEBUG("CharacterExtractor", QString("Inferred age from role/name for '%1': role=%2, age=%3")
                .arg(extracted.name).arg(extracted.role).arg(extracted.age));
        }
    }

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

    const QString cleanSourceText = EncodingUtils::fixGarbledString(sourceText);
    const QString context = extractCharacterContext(cleanSourceText, extracted.name);
    if (context.isEmpty()) {
        return;
    }
    enrichCharacterFromContext(extracted, context);

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
    app.fieldSources = extracted.fieldSources;

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

    for (const QString& suffix : getCharacterNameSuffixes()) {
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

    if (normalizeExtractedName(candidate.name()).compare(normalizedName, Qt::CaseInsensitive) == 0) {
        return true;
    }

    const QStringList existingKeys = CharacterExtractor::buildIdentityKeys(candidate);
    const QStringList incomingKeys = collectIdentityKeys(name, QString(), QStringList());
    for (const QString& key : incomingKeys) {
        if (existingKeys.contains(key, Qt::CaseInsensitive)) {
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
    return saveCharacter(novelId, extracted, 0);
}

bool CharacterExtractor::saveCharacter(const QString& novelId, const ExtractedCharacter& extracted, int sourceChapter)
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
        QList<CharacterFieldDiff> diff;
        Character merged = mergeCharacters(existing, extracted, &diff);
        bool ok = updateCharacter(merged);
        if (ok && !diff.isEmpty()) {
            emit characterVisualFieldsChanged(merged.id(), diff, sourceChapter);
        }
        return ok;
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
    return mergeCharacters(existing, incoming, nullptr);
}

namespace {

// 视觉字段白名单：只有这些字段变化才触发圣经图增量编辑
const QStringList& visualFieldWhitelist()
{
    static const QStringList fields = {
        "hairColor", "hairStyle", "eyeColor", "height", "build",
        "clothing", "accessories", "distinctiveFeatures"
    };
    return fields;
}

QString joinList(const QStringList& list)
{
    return list.join(QString::fromUtf8("、"));
}

QString readVisualFieldAsString(const CharacterAppearance& app, const QString& field)
{
    if (field == "hairColor") return app.hairColor;
    if (field == "hairStyle") return app.hairStyle;
    if (field == "eyeColor") return app.eyeColor;
    if (field == "height") return app.height;
    if (field == "build") return app.build;
    if (field == "clothing") return joinList(app.clothing);
    if (field == "accessories") return joinList(app.accessories);
    if (field == "distinctiveFeatures") return joinList(app.distinctiveFeatures);
    return QString();
}

void collectVisualDiff(const CharacterAppearance& before,
                       const CharacterAppearance& after,
                       QList<CharacterFieldDiff>* outDiff)
{
    if (!outDiff) {
        return;
    }
    for (const QString& field : visualFieldWhitelist()) {
        const QString oldVal = readVisualFieldAsString(before, field);
        const QString newVal = readVisualFieldAsString(after, field);
        const QString oldSrc = before.fieldSources.value(field).toString();
        const QString newSrc = after.fieldSources.value(field).toString();
        const bool valueChanged = oldVal != newVal;
        if (!valueChanged) {
            continue;
        }
        CharacterFieldDiff diff;
        diff.field = field;
        diff.oldValue = oldVal;
        diff.newValue = newVal;
        diff.sourceFrom = oldSrc;
        diff.sourceTo = newSrc;
        outDiff->append(diff);
    }
}

} // namespace

Character CharacterExtractor::mergeCharacters(const Character& existing,
                                              const ExtractedCharacter& incoming,
                                              QList<CharacterFieldDiff>* outDiff) const
{
    Character merged = existing;
    CharacterAppearance app = merged.appearance();
    const CharacterAppearance before = app;

    mergeCharacterAppearance(app, incoming);
    merged.setAliases(mergeCharacterAliases(existing, incoming, app));
    mergeCharacterRole(merged, incoming);

    merged.setAppearance(app);

    if (!incoming.personality.isEmpty()) {
        merged.setPersonality(BibleUtils::mergeStringLists(merged.personality(), incoming.personality));
    }

    collectVisualDiff(before, app, outDiff);

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

    data["appearance"] = JsonUtils::jsonToString(app.toJson());
    data["personalities"] = JsonUtils::jsonToString(JsonUtils::stringListToJsonArray(character.personality()));
    data["portrait_path"] = character.portraitPath();
    if (character.currentPortraitVersionId().isEmpty()) {
        data["current_portrait_version_id"] = QVariant(QVariant::String);
    } else {
        data["current_portrait_version_id"] = character.currentPortraitVersionId();
    }

    return data;
}

int CharacterExtractor::saveCharacters(const QString& novelId, const QList<ExtractedCharacter>& characters)
{
    return saveCharacters(novelId, characters, 0);
}

int CharacterExtractor::saveCharacters(const QString& novelId, const QList<ExtractedCharacter>& characters, int sourceChapter)
{
    if (characters.isEmpty()) {
        return 0;
    }

    int savedCount = 0;

    auto operation = [&]() -> bool {
        for (const ExtractedCharacter& extracted : characters) {
            if (!saveCharacter(novelId, extracted, sourceChapter)) {
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

static QString characterSelectSql(const QString& whereClause)
{
    return QStringLiteral(
        "SELECT c.*, COALESCE(cpv.portrait_path, c.portrait_path) AS portrait_path "
        "FROM characters c "
        "LEFT JOIN character_portrait_versions cpv ON cpv.id = c.current_portrait_version_id "
        "WHERE ") + whereClause;
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
    QList<QVariantMap> results = DatabaseManager::instance()->executeQuery(
        characterSelectSql(QStringLiteral("c.novel_id = ?")), QVariantList() << novelId);
    if (results.isEmpty()) {
        results = DatabaseManager::instance()->selectAll(
            "characters", "novel_id = ?", QVariantList() << novelId);
    }

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
    QList<QVariantMap> rows = DatabaseManager::instance()->executeQuery(
        characterSelectSql(QStringLiteral("c.id = ?")), QVariantList() << characterId);
    if (rows.isEmpty()) {
        rows = DatabaseManager::instance()->selectAll(
            "characters", "id = ?", QVariantList() << characterId);
    }
    return rows.isEmpty() ? Character() : characterFromRow(rows.first());
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
    return persistCharacterRecord(character, true);
}

bool CharacterExtractor::updateCharacterSilent(const Character& character)
{
    return persistCharacterRecord(character, false);
}

bool CharacterExtractor::persistCharacterRecord(const Character& character, bool emitSignals)
{
    QVariantMap data = characterToData(character);
    bool success = DatabaseManager::instance()->update(
        "characters", data, "id = ?", QVariantList() << character.id());

    if (!success) {
        return false;
    }

    BibleCache::instance()->invalidateCharacters(character.novelId());

    if (emitSignals) {
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
        emit characterUpdated(character.id(), character.portraitPath());
    } else {
        LOG_INFO("CharacterExtractor", QString("Updated character (silent): %1").arg(character.name()));
    }

    return true;
}

bool CharacterExtractor::deleteCharacter(const QString& characterId)
{
    // 先删版本文件（事务外，文件操作不可回滚）
    QList<CharacterPortraitVersion> versions = loadPortraitVersions(characterId);
    for (const CharacterPortraitVersion& v : versions) {
        if (!v.portraitPath().isEmpty()) {
            FileStorage::instance()->deleteReferenceImage(v.portraitPath());
        }
    }

    QString novelId;
    bool success = DatabaseManager::instance()->transaction([&]() -> bool {
        QVariantMap row = DatabaseManager::instance()->selectOne(
            "characters", "id = ?", QVariantList() << characterId);
        novelId = row["novel_id"].toString();

        DatabaseManager::instance()->remove(
            "character_portrait_versions", "character_id = ?", QVariantList() << characterId);

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

// ==================== 角色肖像版本 CRUD ====================

QList<CharacterPortraitVersion> CharacterExtractor::loadPortraitVersions(const QString& characterId)
{
    QList<CharacterPortraitVersion> versions;
    if (characterId.isEmpty()) {
        return versions;
    }
    QList<QVariantMap> rows = DatabaseManager::instance()->selectAll(
        "character_portrait_versions",
        "character_id = ?",
        QVariantList() << characterId,
        "version_no ASC");
    for (const QVariantMap& row : rows) {
        versions.append(CharacterPortraitVersion::fromVariantMap(row));
    }
    return versions;
}

QMap<QString, QList<CharacterPortraitVersion>> CharacterExtractor::loadPortraitVersionsByNovel(const QString& novelId)
{
    QMap<QString, QList<CharacterPortraitVersion>> result;
    if (novelId.isEmpty()) {
        return result;
    }
    const QString sql =
        "SELECT cpv.* FROM character_portrait_versions cpv "
        "INNER JOIN characters c ON c.id = cpv.character_id "
        "WHERE c.novel_id = ? "
        "ORDER BY cpv.character_id, cpv.version_no ASC";
    QList<QVariantMap> rows = DatabaseManager::instance()->executeQuery(sql, QVariantList() << novelId);
    for (const QVariantMap& row : rows) {
        CharacterPortraitVersion v = CharacterPortraitVersion::fromVariantMap(row);
        result[v.characterId()].append(v);
    }
    return result;
}

CharacterPortraitVersion CharacterExtractor::getPortraitVersion(const QString& versionId)
{
    if (versionId.isEmpty()) {
        return CharacterPortraitVersion();
    }
    QVariantMap row = DatabaseManager::instance()->selectOne(
        "character_portrait_versions", "id = ?", QVariantList() << versionId);
    return row.isEmpty() ? CharacterPortraitVersion() : CharacterPortraitVersion::fromVariantMap(row);
}

int CharacterExtractor::nextVersionNo(const QString& characterId)
{
    if (characterId.isEmpty()) {
        return 1;
    }
    QList<QVariantMap> rows = DatabaseManager::instance()->executeQuery(
        "SELECT COALESCE(MAX(version_no), 0) AS max_no FROM character_portrait_versions WHERE character_id = ?",
        QVariantList() << characterId);
    if (rows.isEmpty()) {
        return 1;
    }
    return rows.first().value("max_no").toInt() + 1;
}

bool CharacterExtractor::addPortraitVersion(const CharacterPortraitVersion& version)
{
    if (version.id().isEmpty() || version.characterId().isEmpty() || version.portraitPath().isEmpty()) {
        LOG_ERROR("CharacterExtractor", "addPortraitVersion: missing required fields");
        return false;
    }
    QVariantMap data = version.toVariantMap();
    qint64 result = DatabaseManager::instance()->insert("character_portrait_versions", data);
    if (result <= 0) {
        LOG_ERROR("CharacterExtractor", QString("Failed to insert portrait version for character %1").arg(version.characterId()));
        return false;
    }
    LOG_INFO("CharacterExtractor", QString("Added portrait version v%1 for character %2")
        .arg(version.versionNo()).arg(version.characterId()));
    return true;
}

bool CharacterExtractor::setCurrentPortraitVersion(const QString& characterId, const QString& versionId)
{
    if (characterId.isEmpty() || versionId.isEmpty()) {
        return false;
    }
    CharacterPortraitVersion v = getPortraitVersion(versionId);
    if (v.id().isEmpty() || v.characterId() != characterId) {
        LOG_ERROR("CharacterExtractor", QString("setCurrentPortraitVersion: version %1 not found or mismatched character").arg(versionId));
        return false;
    }
    QVariantMap data;
    data["current_portrait_version_id"] = versionId;
    data["portrait_path"] = v.portraitPath();
    bool ok = DatabaseManager::instance()->update("characters", data, "id = ?", QVariantList() << characterId);
    if (ok) {
        Character ch = getCharacterById(characterId);
        if (!ch.id().isEmpty()) {
            BibleCache::instance()->invalidateCharacters(ch.novelId());
            emit characterUpdated(characterId, v.portraitPath());
        }
    }
    return ok;
}

bool CharacterExtractor::deletePortraitVersion(const QString& versionId)
{
    if (versionId.isEmpty()) {
        return false;
    }
    CharacterPortraitVersion v = getPortraitVersion(versionId);
    if (v.id().isEmpty()) {
        return false;
    }
    if (!v.portraitPath().isEmpty()) {
        FileStorage::instance()->deleteReferenceImage(v.portraitPath());
    }
    return DatabaseManager::instance()->remove(
        "character_portrait_versions", "id = ?", QVariantList() << versionId);
}

CharacterPortraitVersion CharacterExtractor::ensureFirstPortraitVersion(const Character& character)
{
    if (character.id().isEmpty() || character.portraitPath().isEmpty()) {
        return CharacterPortraitVersion();
    }
    if (!character.currentPortraitVersionId().isEmpty()) {
        CharacterPortraitVersion existing = getPortraitVersion(character.currentPortraitVersionId());
        if (!existing.id().isEmpty()) {
            return existing;
        }
    }
    QList<CharacterPortraitVersion> existing = loadPortraitVersions(character.id());
    if (!existing.isEmpty()) {
        return existing.first();
    }

    CharacterPortraitVersion v;
    v.setId(QUuid::createUuid().toString(QUuid::WithoutBraces));
    v.setCharacterId(character.id());
    v.setVersionNo(1);
    v.setPortraitPath(character.portraitPath());
    v.setCreatedAt(QDateTime::currentDateTime());
    if (!addPortraitVersion(v)) {
        return CharacterPortraitVersion();
    }
    QVariantMap upd;
    upd["current_portrait_version_id"] = v.id();
    DatabaseManager::instance()->update("characters", upd, "id = ?", QVariantList() << character.id());
    BibleCache::instance()->invalidateCharacters(character.novelId());
    return v;
}
