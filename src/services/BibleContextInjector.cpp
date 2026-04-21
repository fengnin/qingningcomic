#include "services/BibleContextInjector.h"
#include "utils/JsonUtils.h"
#include "utils/BibleUtils.h"
#include "utils/Logger.h"
#include <QRegularExpression>
#include <QSet>

using namespace BibleUtils::BibleContextConstants;

namespace {
constexpr int kMaxContextItemsPerType = 8;
constexpr int kMaxContextPromptChars = 9000;

QStringList buildCharacterMentionVariants(const QString& candidate)
{
    QStringList variants;
    const QString trimmed = candidate.trimmed();
    if (trimmed.isEmpty()) {
        return variants;
    }

    variants << trimmed;

    struct KinshipRule {
        QStringList tokens;
        QStringList variants;
    };

    static const QList<KinshipRule> rules = {
        {{QString::fromUtf8("父亲"), QString::fromUtf8("爸爸"), QString::fromUtf8("老爸"), QString::fromUtf8("爹")},
         {QString::fromUtf8("父亲"), QString::fromUtf8("父"), QString::fromUtf8("爸爸"), QString::fromUtf8("爹")}},
        {{QString::fromUtf8("母亲"), QString::fromUtf8("妈妈"), QString::fromUtf8("老妈"), QString::fromUtf8("娘")},
         {QString::fromUtf8("母亲"), QString::fromUtf8("母"), QString::fromUtf8("妈妈"), QString::fromUtf8("娘")}},
        {{QString::fromUtf8("哥哥"), QString::fromUtf8("兄长"), QString::fromUtf8("大哥"), QString::fromUtf8("哥")},
         {QString::fromUtf8("哥哥"), QString::fromUtf8("哥")}},
        {{QString::fromUtf8("姐姐"), QString::fromUtf8("姊姊"), QString::fromUtf8("大姐"), QString::fromUtf8("姐")},
         {QString::fromUtf8("姐姐"), QString::fromUtf8("姐")}},
        {{QString::fromUtf8("弟弟"), QString::fromUtf8("小弟"), QString::fromUtf8("弟")},
         {QString::fromUtf8("弟弟"), QString::fromUtf8("弟")}},
        {{QString::fromUtf8("妹妹"), QString::fromUtf8("小妹"), QString::fromUtf8("妹")},
         {QString::fromUtf8("妹妹"), QString::fromUtf8("妹")}},
        {{QString::fromUtf8("爷爷"), QString::fromUtf8("爷"), QString::fromUtf8("祖父")},
         {QString::fromUtf8("爷爷"), QString::fromUtf8("爷")}},
        {{QString::fromUtf8("奶奶"), QString::fromUtf8("奶"), QString::fromUtf8("祖母")},
         {QString::fromUtf8("奶奶"), QString::fromUtf8("奶")}}
    };

    for (const KinshipRule& rule : rules) {
        for (const QString& token : rule.tokens) {
            if (trimmed.contains(token)) {
                variants << rule.variants;
            }
        }
    }

    variants.removeDuplicates();
    return variants;
}

QString truncateWithEllipsis(const QString& text, int maxChars)
{
    if (maxChars <= 0 || text.size() <= maxChars) {
        return text;
    }
    if (maxChars <= 3) {
        return text.left(maxChars);
    }
    int cut = maxChars - 3;
    const int newlineCut = text.lastIndexOf(QRegularExpression(QStringLiteral("[\\n\\r]")), cut);
    if (newlineCut > maxChars / 2) {
        cut = newlineCut;
    } else {
        const QStringList boundaries = {QStringLiteral("。"), QStringLiteral("！"), QStringLiteral("？"), QStringLiteral("\n")};
        for (const QString& boundary : boundaries) {
            int pos = text.lastIndexOf(boundary, cut);
            if (pos > cut / 2) {
                cut = pos + boundary.size();
                break;
            }
        }
    }
    return text.left(cut).trimmed() + QStringLiteral("...");
}
}

BibleContextInjector* BibleContextInjector::m_instance = nullptr;
std::once_flag BibleContextInjector::m_instanceOnceFlag;

BibleContextInjector* BibleContextInjector::instance()
{
    std::call_once(m_instanceOnceFlag, []() {
        m_instance = new BibleContextInjector();
    });
    return m_instance;
}

QString BibleContextInjector::normalizeLookupText(const QString& text) const
{
    QString result = text.trimmed();
    result.remove(QRegularExpression(QString::fromUtf8("[\\s\\t\\r\\n·•，。！？!?,；;：:\"""''（）()【】\\[\\]《》<>、/\\\\-]")));
    return result;
}

QStringList BibleContextInjector::deduplicateKeys(const QStringList& keys) const
{
    QStringList result;
    QSet<QString> seen;
    for (QString key : keys) {
        key = key.trimmed();
        if (key.isEmpty() || seen.contains(key)) {
            continue;
        }
        seen.insert(key);
        result << key;
    }
    return result;
}

bool BibleContextInjector::textContainsMention(const QString& normalizedText, const QString& candidate) const
{
    QStringList candidates = buildCharacterMentionVariants(candidate);
    if (candidates.isEmpty()) {
        return false;
    }

    for (const QString& normalizedCandidate : candidates) {
        if (normalizedCandidate.size() < 1) {
            continue;
        }
        if (normalizedText.contains(normalizedCandidate)) {
            return true;
        }

        if (normalizedCandidate.size() <= 2) {
            const QRegularExpression regex(QStringLiteral(R"([\p{Han}]{0,2}%1)").arg(QRegularExpression::escape(normalizedCandidate)));
            if (regex.isValid() && normalizedText.contains(regex)) {
                return true;
            }
        }
    }
    return false;
}

QStringList BibleContextInjector::splitLookupPhrases(const QString& text) const
{
    return deduplicateKeys(text.split(
        QRegularExpression(QString::fromUtf8("[,，;；、/\\\\\\|\\n\\r。！？!?]+")),
        Qt::SkipEmptyParts));
}

bool BibleContextInjector::isLikelySceneLookupKey(const QString& text) const
{
    const QString candidate = text.trimmed();
    if (candidate.size() < 2) {
        return false;
    }

    const QStringList& dynamicTokens = getDynamicTokens();
    for (const QString& token : dynamicTokens) {
        if (candidate.contains(token)) {
            return false;
        }
    }

    const QStringList& locationHints = getLocationHints();
    for (const QString& hint : locationHints) {
        if (candidate.contains(hint)) {
            return true;
        }
    }

    return candidate.size() >= 4;
}

QStringList BibleContextInjector::deriveSceneNameVariants(const QString& name) const
{
    QStringList variants;
    const QString trimmed = name.trimmed();
    if (trimmed.isEmpty()) {
        return variants;
    }

    variants << trimmed;

    QString normalized = trimmed;
    normalized.remove(QRegularExpression(QString::fromUtf8("[（(][^）)]*[）)]")));
    variants << normalized;

    QString stripped = normalized;
    stripped.remove(QRegularExpression(QString::fromUtf8("^[\\p{Han}]{1,6}(家|家中|家里)")));
    stripped.remove(QRegularExpression(QString::fromUtf8("^[\\p{Han}]{1,6}(的)")));
    if (stripped != normalized) {
        variants << stripped;
    }

    const QStringList& locationSuffixes = getLocationSuffixes();
    for (const QString& suffix : locationSuffixes) {
        if (normalized.endsWith(suffix) && normalized != suffix) {
            variants << suffix;
        }
    }

    return deduplicateKeys(variants);
}

QStringList BibleContextInjector::collectSceneLookupKeys(const QJsonObject& scene) const
{
    QStringList keys;
    const QString name = scene["name"].toString().trimmed();
    if (!name.isEmpty()) {
        keys << deriveSceneNameVariants(name);
    }

    QJsonObject details = scene["details"].toObject();
    if (!details.isEmpty()) {
        const QJsonArray aliases = details["aliases"].toArray();
        for (const QJsonValue& aliasValue : aliases) {
            keys << deriveSceneNameVariants(aliasValue.toString().trimmed());
        }

        const QStringList stableLists[] = {
            JsonUtils::jsonArrayToStringList(details["anchorPoints"].toArray()),
            JsonUtils::jsonArrayToStringList(details["signatureObjects"].toArray())
        };
        for (const QStringList& values : stableLists) {
            for (const QString& value : values) {
                for (const QString& phrase : splitLookupPhrases(value)) {
                    if (isLikelySceneLookupKey(phrase)) {
                        keys << phrase;
                    }
                }
            }
        }

        const QStringList stableFields = {
            details["landmark"].toString(),
            details["building"].toString(),
            details["layout"].toString()
        };
        for (const QString& field : stableFields) {
            for (const QString& phrase : splitLookupPhrases(field)) {
                if (isLikelySceneLookupKey(phrase)) {
                    keys << phrase;
                }
            }
        }
    }

    return deduplicateKeys(keys);
}

QString BibleContextInjector::joinArrayPreview(const QJsonArray& array, int limit) const
{
    QStringList items;
    for (int i = 0; i < array.size() && i < limit; ++i) {
        const QString value = array.at(i).toString().trimmed();
        if (!value.isEmpty()) {
            items << value;
        }
    }
    return items.join(QString::fromUtf8("、"));
}

QString BibleContextInjector::summarizeCharacter(const QJsonObject& character) const
{
    QStringList parts;
    const QString name = character["name"].toString().trimmed();
    QJsonArray aliases = character["aliases"].toArray();
    if (aliases.isEmpty()) {
        aliases = character["appearance"].toObject()["aliases"].toArray();
    }
    const QString role = character["role"].toString().trimmed();
    if (!name.isEmpty()) {
        parts << QString::fromUtf8("角色=%1").arg(name);
    }
    if (!aliases.isEmpty()) {
        parts << QString::fromUtf8("别名=%1").arg(joinArrayPreview(aliases, 3));
    }
    if (!role.isEmpty()) {
        parts << QString::fromUtf8("定位=%1").arg(role);
    }

    const QJsonObject appearance = character["appearance"].toObject();
    if (!appearance.isEmpty()) {
        QStringList appearanceParts;
        const QString gender = appearance["gender"].toString().trimmed();
        const QString build = appearance["build"].toString().trimmed();
        const QString hair = appearance["hairStyle"].toString().trimmed();
        const QString clothing = joinArrayPreview(appearance["clothing"].toArray(), 3);
        if (!gender.isEmpty()) appearanceParts << gender;
        if (!build.isEmpty()) appearanceParts << build;
        if (!hair.isEmpty()) appearanceParts << hair;
        if (!clothing.isEmpty()) appearanceParts << QString::fromUtf8("服装:%1").arg(clothing);
        if (!appearanceParts.isEmpty()) {
            parts << QString::fromUtf8("外观=%1").arg(appearanceParts.join(QString::fromUtf8("，")));
        }
    }

    const QString personality = joinArrayPreview(character["personality"].toArray(), 3);
    if (!personality.isEmpty()) {
        parts << QString::fromUtf8("性格=%1").arg(personality);
    }

    return parts.join(QString::fromUtf8("；"));
}

QString BibleContextInjector::summarizeScene(const QJsonObject& scene) const
{
    QStringList parts;
    const QString name = scene["name"].toString().trimmed();
    const QString type = scene["type"].toString().trimmed();
    const QString description = scene["description"].toString().trimmed();
    if (!name.isEmpty()) {
        parts << QString::fromUtf8("场景=%1").arg(name);
    }
    if (!type.isEmpty()) {
        parts << QString::fromUtf8("类型=%1").arg(type);
    }

    QStringList staticParts;
    if (!description.isEmpty()) staticParts << description.left(80);

    const QJsonObject visual = scene["visualCharacteristics"].toObject();
    const QString architecture = visual["architecture"].toString().trimmed();
    const QString landmarks = joinArrayPreview(visual["keyLandmarks"].toArray(), 3);
    if (!architecture.isEmpty()) staticParts << architecture;
    if (!landmarks.isEmpty()) staticParts << QString::fromUtf8("地标:%1").arg(landmarks);

    const QJsonObject spatial = scene["spatialLayout"].toObject();
    const QString layout = spatial["layout"].toString().trimmed();
    if (!layout.isEmpty()) staticParts << QString::fromUtf8("布局:%1").arg(layout.left(50));

    const QString anchors = joinArrayPreview(scene["anchorPoints"].toArray(), 3);
    if (!anchors.isEmpty()) staticParts << QString::fromUtf8("锚点:%1").arg(anchors);

    if (!staticParts.isEmpty()) {
        parts << QString::fromUtf8("静态设定=%1").arg(staticParts.join(QString::fromUtf8("；")));
    }

    return parts.join(QString::fromUtf8("；"));
}

BibleContextInjector::FilteredContext BibleContextInjector::filterRelevantContext(
    const QString& text,
    const QJsonArray& existingCharacters,
    const QJsonArray& existingScenes)
{
    FilteredContext result;
    result.characters = filterCharacters(text, existingCharacters);
    result.scenes = filterScenes(text, existingScenes);
    
    LOG_INFO("BibleContextInjector", QString("Filtered bible context: characters %1/%2, scenes %3/%4")
        .arg(result.characters.size())
        .arg(existingCharacters.size())
        .arg(result.scenes.size())
        .arg(existingScenes.size()));
    
    return result;
}

QString BibleContextInjector::buildContextPrompt(
    const QString& text,
    const QJsonArray& characters,
    const QJsonArray& scenes,
    int chapterNumber)
{
    if (characters.isEmpty() && scenes.isEmpty()) {
        return text;
    }
    
    QString userMessage = QString::fromUtf8("以下是第%1章的设定资料：\n\n")
        .arg(chapterNumber > 0 ? QString::number(chapterNumber) : "?");
    
    if (!characters.isEmpty()) {
        userMessage += QString::fromUtf8("【角色设定】\n");
        int appended = 0;
        for (const QJsonValue& value : characters) {
            if (appended >= kMaxContextItemsPerType) {
                break;
            }
            const QString summary = summarizeCharacter(value.toObject());
            if (!summary.isEmpty()) {
                userMessage += QString::fromUtf8("- ") + summary + "\n";
                ++appended;
            }
        }
        userMessage += "\n\n";
    }
    
    if (!scenes.isEmpty()) {
        userMessage += QString::fromUtf8("【场景设定】\n");
        int appended = 0;
        for (const QJsonValue& value : scenes) {
            if (appended >= kMaxContextItemsPerType) {
                break;
            }
            const QString summary = summarizeScene(value.toObject());
            if (!summary.isEmpty()) {
                userMessage += QString::fromUtf8("- ") + summary + "\n";
                ++appended;
            }
        }
        userMessage += "\n\n";
    }
    
    userMessage += QString::fromUtf8("【正文内容】\n") + text;
    userMessage = truncateWithEllipsis(userMessage, kMaxContextPromptChars);
    
    return userMessage;
}

QJsonArray BibleContextInjector::filterCharacters(const QString& text, const QJsonArray& characters)
{
    const QString normalizedText = normalizeLookupText(text);
    QJsonArray filtered;
    for (const QJsonValue& value : characters) {
        if (filtered.size() >= kMaxContextItemsPerType) {
            break;
        }
        const QJsonObject character = value.toObject();
        const QString name = character["name"].toString().trimmed();
        QJsonArray aliases = character["aliases"].toArray();
        QJsonObject appearance = character["appearance"].toObject();
        if (aliases.isEmpty()) {
            aliases = appearance["aliases"].toArray();
        }

        bool matched = textContainsMention(normalizedText, name);
        if (!matched) {
            for (const QJsonValue& aliasValue : aliases) {
                if (textContainsMention(normalizedText, aliasValue.toString().trimmed())) {
                    matched = true;
                    break;
                }
            }
        }

        if (matched) {
            filtered.append(character);
        }
    }
    return filtered;
}

QJsonArray BibleContextInjector::filterScenes(const QString& text, const QJsonArray& scenes)
{
    const QString normalizedText = normalizeLookupText(text);
    QJsonArray filtered;
    for (const QJsonValue& value : scenes) {
        if (filtered.size() >= kMaxContextItemsPerType) {
            break;
        }
        const QJsonObject scene = value.toObject();
        const QStringList keys = collectSceneLookupKeys(scene);
        bool matched = false;
        QString matchedKey;
        for (const QString& key : keys) {
            if (textContainsMention(normalizedText, key)) {
                matched = true;
                matchedKey = key;
                break;
            }
        }
        if (matched) {
            LOG_DEBUG("BibleContextInjector", QString("Matched scene context '%1' by key '%2'")
                .arg(scene["name"].toString(), matchedKey));
            filtered.append(scene);
        }
    }
    return filtered;
}
