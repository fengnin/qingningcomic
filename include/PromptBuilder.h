#ifndef PROMPTBUILDER_H
#define PROMPTBUILDER_H

#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QMap>

/**
 * @brief Prompt 构建工具类
 * 
 * 将面板、角色、场景数据转换为图像生成提示词。
 * 包含视角、姿态、风格、表情等映射表。
 */
class PromptBuilder
{
public:
    struct PromptResult {
        QString text;
        QString negativePrompt;
        QStringList referenceImages;
    };

    static PromptResult buildCharacterPrompt(const QJsonObject &character, 
                                              const QJsonObject &options = QJsonObject());
    
    static PromptResult buildPanelPrompt(const QJsonObject &panel,
                                          const QMap<QString, QJsonObject> &characterRefs = {},
                                          const QMap<QString, QJsonObject> &sceneRefs = {},
                                          const QJsonObject &options = QJsonObject());
    
    static PromptResult buildScenePrompt(const QJsonObject &scene);

private:
    static QString formatCharacterDescriptor(const QString &name, 
                                              const QString &pose, 
                                              const QString &expression);
    static QString buildHairDescription(const QJsonObject &appearance);
    static QStringList normaliseList(const QJsonValue &value);
};

#endif // PROMPTBUILDER_H
