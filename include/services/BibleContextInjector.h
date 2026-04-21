#ifndef BIBLECONTEXTINJECTOR_H
#define BIBLECONTEXTINJECTOR_H

#include <QString>
#include <QJsonArray>
#include <QJsonObject>
#include <mutex>

class BibleContextInjector
{
public:
    static BibleContextInjector* instance();
    
    struct FilteredContext
    {
        QJsonArray characters;
        QJsonArray scenes;
    };
    
    FilteredContext filterRelevantContext(const QString& text,
                                          const QJsonArray& existingCharacters,
                                          const QJsonArray& existingScenes);
    
    QString buildContextPrompt(const QString& text,
                               const QJsonArray& characters,
                               const QJsonArray& scenes,
                               int chapterNumber);
    
    QJsonArray filterCharacters(const QString& text, const QJsonArray& characters);
    QJsonArray filterScenes(const QString& text, const QJsonArray& scenes);

private:
    BibleContextInjector() = default;
    ~BibleContextInjector() = default;
    BibleContextInjector(const BibleContextInjector&) = delete;
    BibleContextInjector& operator=(const BibleContextInjector&) = delete;
    
    static BibleContextInjector* m_instance;
    static std::once_flag m_instanceOnceFlag;
    
    QString normalizeLookupText(const QString& text) const;
    QStringList deduplicateKeys(const QStringList& keys) const;
    bool textContainsMention(const QString& normalizedText, const QString& candidate) const;
    QStringList splitLookupPhrases(const QString& text) const;
    bool isLikelySceneLookupKey(const QString& text) const;
    QStringList deriveSceneNameVariants(const QString& name) const;
    QStringList collectSceneLookupKeys(const QJsonObject& scene) const;
    
    QString summarizeCharacter(const QJsonObject& character) const;
    QString summarizeScene(const QJsonObject& scene) const;
    QString joinArrayPreview(const QJsonArray& array, int limit = 3) const;
};

#endif
