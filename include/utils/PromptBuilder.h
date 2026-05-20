#ifndef PROMPTBUILDER_H
#define PROMPTBUILDER_H

#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QMap>

class PromptBuilder
{
public:
    struct PromptResult {
        QString text;
        QString negativePrompt;
        QStringList referenceImages;
        QMap<QString, QString> referenceImageTypes;
    };

    static PromptResult buildCharacterPrompt(const QJsonObject &character, 
                                              const QJsonObject &options = QJsonObject());
    
    static PromptResult buildPanelPrompt(const QJsonObject &panel,
                                          const QMap<QString, QJsonObject> &characterRefs = {},
                                          const QMap<QString, QJsonObject> &sceneRefs = {},
                                          const QJsonObject &options = QJsonObject());
    
    static PromptResult buildScenePrompt(const QJsonObject &scene,
                                          const QJsonObject &options = QJsonObject());

    static QString filterHumanKeywords(const QString &text);

    static QStringList normalizeList(const QJsonValue &value);
    static QString formatHair(const QJsonObject &appearance);
    static QString formatCharacterDescriptor(const QString &name, 
                                              const QString &pose, 
                                              const QString &expression);
    static QString buildCharacterAppearance(const QJsonObject &appearance);
    static QString firstNonEmptyPortrait(const QStringList &portraitPaths);

    struct PanelCharacterPromptData {
        QStringList panelCharNames;
        QStringList characterDescriptions;
        QString primaryCharacterRef;
        QStringList allCharacterRefs;
    };

private:
    static QString matchSceneDetails(const QMap<QString, QJsonObject> &sceneRefs,
                                      const QString &sceneId,
                                      const QString &sceneName);
    static QMap<QString, QString> extractDialogueSpeakers(const QJsonArray &dialogue);
    static QString resolveSceneRefPath(const QMap<QString, QJsonObject> &sceneRefs,
                                        const QString &sceneId,
                                        const QString &sceneName);
    static void appendVariationDescs(QStringList &parts,
                                      const QJsonArray &variations,
                                      const QMap<QString, QString> &mapping,
                                      const QString &keyField,
                                      const QString &label);
    static void appendScenePromptDetails(QStringList &parts, const QJsonObject &scene);
    static void appendSpatialLayout(QStringList &parts, const QJsonObject &spatialLayout);
    static void appendVisualCharacteristics(QStringList &parts, const QJsonObject &visual);
};

#endif // PROMPTBUILDER_H
