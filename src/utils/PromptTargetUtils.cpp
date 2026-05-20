#include "utils/PromptTargetUtils.h"

namespace PromptTargetUtils {


bool containsAny(const QString& text, const QStringList& keywords)
{
    const QString lowerText = text.trimmed().toLower();
    for (const QString& keyword : keywords) {
        if (lowerText.contains(keyword.toLower())) {
            return true;
        }
    }
    return false;
}

bool isSceneFocusedPrompt(const QString& text)
{
    static const QStringList keywords = {
        "scene", "background", "environment", "setting", "location",
        "room", "street", "building", "architecture", "landscape",
        QString::fromUtf8("场景"), QString::fromUtf8("背景"), QString::fromUtf8("环境"),
        QString::fromUtf8("街道"), QString::fromUtf8("建筑"), QString::fromUtf8("风景"),
        QString::fromUtf8("室内"), QString::fromUtf8("室外"), QString::fromUtf8("城市"),
        QString::fromUtf8("自然")
    };
    return containsAny(text, keywords);
}

bool isCharacterFocusedPrompt(const QString& text)
{
    static const QStringList keywords = {
        "character", "person", "face", "expression", "outfit",
        "clothing", "hair", "pose", "body", "identity",
        QString::fromUtf8("角色"), QString::fromUtf8("人物"), QString::fromUtf8("表情"),
        QString::fromUtf8("服装"), QString::fromUtf8("发型"), QString::fromUtf8("姿势"),
        QString::fromUtf8("外貌"), QString::fromUtf8("特征"), QString::fromUtf8("身份")
    };
    return containsAny(text, keywords);
}

bool isRemovalIntentText(const QString& text)
{
    const QString lower = text.trimmed().toLower();
    if (lower.isEmpty()) {
        return false;
    }

    static const QStringList keywords = {
        QString::fromUtf8("去掉"),
        QString::fromUtf8("删除"),
        QString::fromUtf8("移除"),
        QString::fromUtf8("去除"),
        QString::fromUtf8("擦掉"),
        QString::fromUtf8("抹掉"),
        QString::fromUtf8("拿掉"),
        QStringLiteral("remove"),
        QStringLiteral("delete"),
        QStringLiteral("erase"),
        QStringLiteral("eliminate"),
        QStringLiteral("clear out")
    };

    return containsAny(lower, keywords);
}

bool isRotationIntent(const QString& editIntent)
{
    return editIntent.trimmed().toLower() == QStringLiteral("rotate_subject");
}

bool isLocalObjectMovementIntent(const QString& editIntent)
{
    const QString normalizedIntent = editIntent.trimmed().toLower();
    return normalizedIntent == QStringLiteral("move_subject")
        || normalizedIntent == QStringLiteral("insert_subject");
}

bool isSceneEditIntent(const QString& editIntent)
{
    const QString normalizedIntent = editIntent.trimmed().toLower();
    return normalizedIntent == QStringLiteral("replace_background")
        || normalizedIntent == QStringLiteral("replace_lighting")
        || normalizedIntent == QStringLiteral("remove_subject")
        || isLocalObjectMovementIntent(normalizedIntent)
        || isRotationIntent(normalizedIntent);
}

bool isCharacterEditIntent(const QString& editIntent)
{
    const QString normalizedIntent = editIntent.trimmed().toLower();
    return normalizedIntent == QStringLiteral("replace_attribute")
        || normalizedIntent == QStringLiteral("set_expression")
        || normalizedIntent == QStringLiteral("setexpression");
}

bool shouldPreferSceneReferenceForEditIntent(const QString& editIntent, const QString& editPrompt)
{
    const QString normalizedIntent = editIntent.trimmed().toLower();
    const bool localObjectEdit = normalizedIntent == QStringLiteral("replace_subject")
        || isLocalObjectMovementIntent(normalizedIntent);
    return localObjectEdit && !isCharacterFocusedPrompt(editPrompt);
}

QString resolveTargetFromEditMetadata(const QJsonObject& panelJson)
{
    const QString editIntent = panelJson.value("editIntent").toString().trimmed().toLower();
    const QString editTarget = panelJson.value("editTarget").toString().trimmed().toLower();

    if (isCharacterEditIntent(editIntent)
        || editTarget == QStringLiteral("character")) {
        return QStringLiteral("character");
    }

    if (isSceneEditIntent(editIntent)
        || editTarget == QStringLiteral("background")
        || editTarget == QStringLiteral("scene")) {
        return QStringLiteral("scene");
    }

    return QString();
}

QString resolveEditPromptTarget(const QJsonObject& panelJson,
                                      const QString& explicitTarget,
                                      const QString& editPrompt,
                                      const QString& panelSceneText)
{
    const QString normalizedExplicitTarget = explicitTarget.trimmed().toLower();
    if (!normalizedExplicitTarget.isEmpty()) {
        return normalizedExplicitTarget;
    }

    const QString metadataTarget = resolveTargetFromEditMetadata(panelJson);
    if (!metadataTarget.isEmpty()) {
        return metadataTarget;
    }

    if (isRemovalIntentText(editPrompt)) {
        return QStringLiteral("scene");
    }

    const bool sceneFocused = isSceneFocusedPrompt(editPrompt) || isSceneFocusedPrompt(panelSceneText);
    const bool characterFocused = isCharacterFocusedPrompt(editPrompt);
    if (sceneFocused && !characterFocused) {
        return QStringLiteral("scene");
    }
    if (characterFocused && !sceneFocused) {
        return QStringLiteral("character");
    }
    return QString();
}

QString classifyPanelPromptTarget(const QJsonObject& panelJson)
{
    const QString visualPrompt = panelJson.value("visualPrompt").toString().trimmed();
    const QString visualPromptEn = panelJson.value("visualPromptEn").toString().trimmed();
    const QString combined = (visualPrompt + " " + visualPromptEn).trimmed().toLower();
    const QString sceneText = panelJson.value("scene").toString().trimmed().toLower();
    const QString shotType = panelJson.value("shotType").toString().trimmed().toLower();

    const QString metadataTarget = resolveTargetFromEditMetadata(panelJson);
    if (!metadataTarget.isEmpty()) {
        return metadataTarget;
    }

    const QString promptTarget = resolveEditPromptTarget(panelJson, QString(), combined, sceneText);
    if (!promptTarget.isEmpty()) {
        return promptTarget;
    }

    static const QStringList sceneShotTypes = {
        "wide", "extreme-wide", "establishing", "full", "long", "master"
    };
    static const QStringList characterShotTypes = {
        "close-up", "extreme-close-up", "medium-close-up", "portrait", "single"
    };

    if (containsAny(shotType, sceneShotTypes)) {
        return QStringLiteral("scene");
    }
    if (containsAny(shotType, characterShotTypes)) {
        return QStringLiteral("character");
    }

    const int charCount = panelJson.value("characters").toArray().size();
    if (charCount <= 1) {
        return QStringLiteral("character");
    }
    if (charCount >= 3) {
        return QStringLiteral("scene");
    }

    return QStringLiteral("balanced");
}

} // namespace PromptTargetUtils
