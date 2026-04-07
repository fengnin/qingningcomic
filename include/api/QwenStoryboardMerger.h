#ifndef QWEN_STORYBOARD_MERGER_H
#define QWEN_STORYBOARD_MERGER_H

#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QList>
#include <QMap>

class QwenStoryboardMerger
{
public:
    static QJsonObject merge(const QList<QJsonObject>& storyboards);
    
private:
    static QJsonObject normalizePanel(const QJsonObject& panel, int index);
    static QJsonObject mergeCharacter(const QJsonObject& existing, const QJsonObject& newChar);
    
    static constexpr int PANELS_PER_PAGE = 4;
    
    QwenStoryboardMerger() = delete;
};

#endif // QWEN_STORYBOARD_MERGER_H
