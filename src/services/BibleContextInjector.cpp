#include "services/BibleContextInjector.h"
#include "utils/JsonUtils.h"
#include "utils/BibleUtils.h"
#include "utils/Logger.h"
#include <QJsonDocument>
#include <QRegularExpression>
#include <QSet>

using namespace BibleUtils::BibleContextConstants;

namespace {
constexpr int kMaxContextItemsPerType = 8;
constexpr int kMaxContextPromptChars = 9000;

struct ContextMatchResult
{
    bool matched = false;
    QString matchedBy;
};

static const QRegularExpression& getSplitLookupRegex()
{
    static const QRegularExpression regex(QString::fromUtf8("[,，;；、/\\\\\\|\\n\\r。！？!?]+"));
    return regex;
}

static const QRegularExpression& getNormalizeLookupRegex()
{
    static const QRegularExpression regex(QString::fromUtf8("[\\s\\t\\r\\n\\x{200B}\\x{200C}\\x{200D}\\x{FEFF}·•，。！？!?,；;：:\"""''（）()【】\\[\\]《》<>、/\\\\-]"));
    return regex;
}

static const QRegularExpression& getShortHanPatternRegex()
{
    static const QRegularExpression regex(QStringLiteral(R"([\p{Han}]{0,2})"));
    return regex;
}

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

    static const QStringList honorificSuffixes = {
        QString::fromUtf8("阿姨"),
        QString::fromUtf8("叔叔"),
        QString::fromUtf8("伯伯"),
        QString::fromUtf8("姑姑"),
        QString::fromUtf8("舅舅"),
        QString::fromUtf8("婶婶")
    };

    for (const QString& suffix : honorificSuffixes) {
        if (trimmed.endsWith(suffix) && trimmed.size() > suffix.size()) {
            variants << suffix;
            const QString abbreviated = trimmed.left(trimmed.size() - suffix.size()) + suffix.right(1);
            if (!abbreviated.isEmpty()) {
                variants << abbreviated;
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

void appendSummaryPart(QStringList& parts, const QString& label, const QString& value)
{
    const QString trimmed = value.trimmed();
    if (!trimmed.isEmpty()) {
        parts << QString::fromUtf8("%1=%2").arg(label, trimmed);
    }
}

void appendSummaryValue(QStringList& parts, const QString& value)
{
    const QString trimmed = value.trimmed();
    if (!trimmed.isEmpty()) {
        parts << trimmed;
    }
}

QString previewArrayValues(const QJsonArray& array, int limit)
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

QStringList buildCharacterSummaryParts(const QJsonObject& character)
{
    QStringList parts;
    const QString name = character["name"].toString().trimmed();
    QJsonArray aliases = character["aliases"].toArray();
    if (aliases.isEmpty()) {
        aliases = character["appearance"].toObject()["aliases"].toArray();
    }
    const QString role = character["role"].toString().trimmed();
    appendSummaryPart(parts, QString::fromUtf8("角色"), name);
    appendSummaryPart(parts, QString::fromUtf8("别名"), previewArrayValues(aliases, 3));
    appendSummaryPart(parts, QString::fromUtf8("定位"), role);

    const QJsonObject appearance = character["appearance"].toObject();
    if (!appearance.isEmpty()) {
        QStringList appearanceParts;
        appendSummaryValue(appearanceParts, appearance["gender"].toString());
        appendSummaryValue(appearanceParts, appearance["build"].toString());
        appendSummaryValue(appearanceParts, appearance["hairStyle"].toString());
        const QString clothing = previewArrayValues(appearance["clothing"].toArray(), 3);
        if (!clothing.isEmpty()) appearanceParts << QString::fromUtf8("服装:%1").arg(clothing);
        if (!appearanceParts.isEmpty()) {
            parts << QString::fromUtf8("外观=%1").arg(appearanceParts.join(QString::fromUtf8("，")));
        }
    }

    appendSummaryPart(parts, QString::fromUtf8("性格"), previewArrayValues(character["personality"].toArray(), 3));
    return parts;
}

QStringList buildSceneSummaryParts(const QJsonObject& scene)
{
    QStringList parts;
    appendSummaryPart(parts, QString::fromUtf8("场景"), scene["name"].toString());
    appendSummaryPart(parts, QString::fromUtf8("类型"), scene["type"].toString());

    QStringList staticParts;
    const QString description = scene["description"].toString().trimmed();
    if (!description.isEmpty()) staticParts << description.left(80);

    const QJsonObject visual = scene["visualCharacteristics"].toObject();
    appendSummaryValue(staticParts, visual["architecture"].toString());
    const QString landmarks = previewArrayValues(visual["keyLandmarks"].toArray(), 3);
    if (!landmarks.isEmpty()) staticParts << QString::fromUtf8("地标:%1").arg(landmarks);

    const QJsonObject spatial = scene["spatialLayout"].toObject();
    const QString layout = spatial["layout"].toString().trimmed();
    if (!layout.isEmpty()) staticParts << QString::fromUtf8("布局:%1").arg(layout.left(50));

    const QString anchors = previewArrayValues(scene["anchorPoints"].toArray(), 3);
    if (!anchors.isEmpty()) staticParts << QString::fromUtf8("锚点:%1").arg(anchors);

    if (!staticParts.isEmpty()) {
        parts << QString::fromUtf8("静态设定=%1").arg(staticParts.join(QString::fromUtf8("；")));
    }

    return parts;
}

template <typename MatchFn>
QJsonArray filterContextItems(const QString& text,
                              const QJsonArray& items,
                              int limit,
                              const QString& itemLabel,
                              bool logSkipped,
                              MatchFn matchFn)
{
    const QString normalizedText = BibleContextInjector::instance()->normalizeLookupText(text);
    QJsonArray filtered;

    for (const QJsonValue& value : items) {
        if (filtered.size() >= limit) {
            break;
        }

        const QJsonObject item = value.toObject();
        const QString name = item["name"].toString().trimmed();
        const ContextMatchResult match = matchFn(normalizedText, item);

        if (match.matched) {
            LOG_DEBUG("BibleContextInjector", QString("Matched %1 context '%2' by %3")
                .arg(itemLabel, name.isEmpty() ? QString::fromUtf8("未命名") : name,
                     match.matchedBy.isEmpty() ? QStringLiteral("unknown") : match.matchedBy));
            filtered.append(item);
        } else if (logSkipped && !name.isEmpty()) {
            LOG_DEBUG("BibleContextInjector", QString("Skipped %1 context '%2' because it is not mentioned in current chapter text")
                .arg(itemLabel, name));
        }
    }

    return filtered;
}

template <typename SummarizeFn>
void appendContextSection(QString& userMessage,
                          const QString& title,
                          const QJsonArray& items,
                          int maxItems,
                          SummarizeFn summarizeFn)
{
    if (items.isEmpty()) {
        return;
    }

    userMessage += title + "\n";
    int appended = 0;
    for (const QJsonValue& value : items) {
        if (appended >= maxItems) {
            break;
        }

        const QString summary = summarizeFn(value.toObject());
        if (summary.isEmpty()) {
            continue;
        }

        userMessage += QString::fromUtf8("- ") + summary + "\n";
        ++appended;
    }
    userMessage += "\n\n";
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
    result.remove(getNormalizeLookupRegex());
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

    const QString normalizedCandidate = normalizeLookupText(candidate);
    if (!normalizedCandidate.isEmpty() && !candidates.contains(normalizedCandidate)) {
        candidates.prepend(normalizedCandidate);
    }

    for (const QString& normalizedCandidate : candidates) {
        if (normalizedCandidate.size() < 1) {
            continue;
        }
        if (normalizedText.contains(normalizedCandidate)) {
            return true;
        }

        if (normalizedCandidate.size() <= 2) {
            const QRegularExpression regex(getShortHanPatternRegex().pattern() +
                QRegularExpression::escape(normalizedCandidate));
            if (regex.isValid() && normalizedText.contains(regex)) {
                return true;
            }
        }
    }

    LOG_DEBUG("BibleContextInjector", QString("Mention check failed: candidate='%1', variants='%2', textHead='%3', textTail='%4'")
        .arg(candidate.trimmed(),
             candidates.join(QStringLiteral(" | ")),
             normalizedText.left(120),
             normalizedText.right(120)));
    return false;
}

QStringList BibleContextInjector::splitLookupPhrases(const QString& text) const
{
    return deduplicateKeys(text.split(getSplitLookupRegex(), Qt::SkipEmptyParts));
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

QString BibleContextInjector::summarizeCharacter(const QJsonObject& character) const
{
    return buildCharacterSummaryParts(character).join(QString::fromUtf8("；"));
}

QString BibleContextInjector::summarizeScene(const QJsonObject& scene) const
{
    return buildSceneSummaryParts(scene).join(QString::fromUtf8("；"));
}

BibleContextInjector::FilteredContext BibleContextInjector::filterRelevantContext(
    const QString& text,
    const QJsonArray& existingCharacters,
    const QJsonArray& existingScenes)
{
    FilteredContext result;
    LOG_INFO("BibleContextInjector", QString("Filtering bible context from text length %1").arg(text.length()));
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
        .arg(chapterNumber > 0 ? QString::number(chapterNumber) : QString::fromUtf8("?"));

    if (!characters.isEmpty()) {
        userMessage += QString::fromUtf8("【🔒 角色外观锁定（必须严格遵守）】\n");
        userMessage += QString::fromUtf8(
            "以下角色的外观特征是【不可更改的权威设定】。\n"
            "在生成每个面板的 visualPrompt 时，**必须**准确使用以下特征：\n\n"

            "⚠️ 绝对禁止的错误：\n"
            "- 不得修改或忽略 appearance 中的任何字段值（如发色、年龄、眼睛颜色等）！\n"
            "- 不得将角色年龄改大或改小（如老年人不能画成年轻人）！\n"
            "- 不得遗漏 features 字段中的标志性身体特征（如斑、纹、疤等）！\n\n"

            "✅ 正确做法：\n"
            "- visualPrompt 中的角色外观描述必须与 Bible 的 appearance 字段完全一致\n"
            "- 年龄特征必须准确体现（如高龄角色需强调老年特征）\n"
            "- features 字段中的关键特征必须在 visualPrompt 中体现\n\n"

            "📋 完整角色数据（appearance字段不可违反）：\n"
        );
        userMessage += QString::fromUtf8(QJsonDocument(characters).toJson(QJsonDocument::Indented));
        userMessage += QString::fromUtf8("\n\n");

        LOG_INFO("BibleContextInjector", QString(
            "准备生成Few-Shot示例 (角色数: %1)").arg(characters.size()));
        
        const QString fewShotExamples = buildFewShotExamples();
        
        userMessage += fewShotExamples;
        
        LOG_INFO("BibleContextInjector", QString(
            "✅ Few-Shot示例已添加到User Message (示例长度: %1字符)")
            .arg(fewShotExamples.length()));
        
        LOG_DEBUG("BibleContextInjector", QString(
            "Few-Shot内容预览: %1...")
            .arg(fewShotExamples.left(100)));
        
        userMessage += QString::fromUtf8("\n\n");
    }

    if (!scenes.isEmpty()) {
        userMessage += QString::fromUtf8("【现有场景圣经】请在生成的 scenes 数组中包含以下所有场景，并保持其 visualCharacteristics 不变。在 panel.background.sceneId 中优先使用这些场景ID：\n");
        userMessage += QString::fromUtf8(QJsonDocument(scenes).toJson(QJsonDocument::Indented));
        userMessage += QString::fromUtf8("\n\n");
    }

    userMessage += QString::fromUtf8("【新章节文本】\n") + text;
    return truncateWithEllipsis(userMessage, kMaxContextPromptChars);
}

QJsonArray BibleContextInjector::filterCharacters(const QString& text, const QJsonArray& characters)
{
    return filterContextItems(text, characters, kMaxContextItemsPerType, QStringLiteral("character"), true,
        [this](const QString& normalizedText, const QJsonObject& character) {
            QJsonArray aliases = character["aliases"].toArray();
            QJsonObject appearance = character["appearance"].toObject();
            if (aliases.isEmpty()) {
                aliases = appearance["aliases"].toArray();
            }

            const QString name = character["name"].toString().trimmed();
            if (textContainsMention(normalizedText, name)) {
                ContextMatchResult match;
                match.matched = true;
                match.matchedBy = QStringLiteral("name");
                return match;
            }

            for (const QJsonValue& aliasValue : aliases) {
                const QString alias = aliasValue.toString().trimmed();
                if (textContainsMention(normalizedText, alias)) {
                    ContextMatchResult match;
                    match.matched = true;
                    match.matchedBy = QStringLiteral("alias:%1").arg(alias);
                    return match;
                }
            }

            return ContextMatchResult{};
        });
}

QString BibleContextInjector::buildFewShotExamples()
{
    return QString::fromUtf8(
        "📌 **visualPrompt生成规范（必须严格遵循）**：\n\n"

        "**核心原则：100%基于Bible数据生成**\n"
        "- visualPrompt中的每个角色外观描述必须来自Bible的appearance字段\n"
        "- 禁止编造或修改Bible中未定义的特征\n"
        "- Bible是最高优先级，高于文学性、美观性或流畅性\n\n"

        "**✅ 正确的输出格式示例**\n\n"

        "**格式1 - 完整外观描述（推荐）**\n"
        "'{发色}{发型}的{角色名}{年龄称谓}，身穿{衣服1}配{衣服2}及{衣服3}，{眼睛颜色}眼睛'\n"
        "✓ 特征完整：包含Bible中的所有appearance字段\n"
        "✓ 顺序合理：先整体印象，再细节特征\n"
        "✓ 衣服具体：使用全称而非笼统描述\n\n"

        "**格式2 - 年龄特征强调（老年/幼年角色）**\n"
        "'{年龄}岁{性别标签}{角色名}，{标志性特征}，身穿{衣服}'\n"
        "✓ 年龄准确：必须体现年龄特征（皱纹/微驼/稚嫩等）\n"
        "✓ 标志性特征：保留Bible中的features字段\n"
        "✓ 衣服符合年龄：避免不符合年龄的着装\n\n"

        "**格式3 - 多角色场景**\n"
        "'{角色A描述}在{位置A}，{角色B描述}在{位置B}，两人{互动方式}'\n"
        "✓ 每个角色都保持完整的Bible特征\n"
        "✓ 角色关系清晰\n"
        "✓ 位置明确\n\n"

        "**格式4 - 动作场景**\n"
        "'{基础外观}正在{动作描述}，{衣服随动作的变化}'\n"
        "✓ 基础外观不变：动作不能改变Bible定义的特征\n"
        "✓ 衣服变化合理：只允许物理上合理的动态效果\n"
        "✓ 不夸张化：避免不合理的动作描写\n\n"

        "**❌ 常见错误及原因**\n\n"
        "- 错误1：修改了Bible中定义的发色/发型/眼睛颜色\n"
        "  ✗ 原因：未严格参考appearance字段\n"
        "  ✓ 解决：逐项对照Bible的appearance字段\n\n"
        "- 错误2：衣服描述过于笼统（如'连衣裙'代替具体的衣服组合）\n"
        "  ✗ 原因：遗漏了clothing数组中的具体项\n"
        "  ✓ 解决：列出clothing数组中的每一件衣服的全称\n\n"
        "- 错误3：年龄特征不准确（如老年人画成年轻人）\n"
        "  ✗ 原因：忽略了age字段的数值\n"
        "  ✓ 解决：根据age值添加相应的年龄特征描写\n\n"
        "- 错误4：丢失角色的标志性身体特征\n"
        "  ✗ 原因：未检查features字段\n"
        "  ✓ 解决：确保features中的关键特征体现在visualPrompt中\n\n"

        "**⚠️ 强制检查清单（生成每个panel时必须验证）：**\n"
        "□ 角色姓名是否与Bible一致？\n"
        "□ 发色是否与hairColor完全匹配？\n"
        "□ 发型是否与hairStyle一致？\n"
        "□ 眼睛颜色是否与eyeColor正确？\n"
        "□ 衣服是否完整列出clothing数组的所有项？（使用全称）\n"
        "□ 年龄是否与age字段匹配？是否体现相应年龄特征？\n"
        "□ features中的关键特征是否体现？\n"
        "□ 如果任何一项不通过，必须修改visualPrompt直到完全符合Bible！\n\n"

        "**💡 记住：Bible是绝对权威，visualPrompt只是Bible的可视化表达！**\n"
    );
}

QJsonArray BibleContextInjector::filterScenes(const QString& text, const QJsonArray& scenes)
{
    return filterContextItems(text, scenes, kMaxContextItemsPerType, QStringLiteral("scene"), false,
        [this](const QString& normalizedText, const QJsonObject& scene) {
            const QStringList keys = collectSceneLookupKeys(scene);
            for (const QString& key : keys) {
                if (textContainsMention(normalizedText, key)) {
                    ContextMatchResult match;
                    match.matched = true;
                    match.matchedBy = key;
                    return match;
                }
            }
            return ContextMatchResult{};
        });
}
