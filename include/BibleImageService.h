#ifndef BIBLEIMAGESERVICE_H
#define BIBLEIMAGESERVICE_H

#include "BatchImageProcessor.h"
#include "Character.h"
#include "Scene.h"
#include "QwenImageClient.h"
#include "PromptBuilder.h"

class BibleImageService : public BatchImageProcessor
{
    Q_OBJECT

public:
    static BibleImageService* instance();
    
    void generateCharacterPortraitAsync(const Character& character);
    void generateSceneReferenceAsync(const Scene& scene);
    
    void generateCharacterPortraits(const QList<Character>& characters);
    void generateSceneReferences(const QList<Scene>& scenes);
    
    void generateAllBibleImages(const QList<Character>& characters, const QList<Scene>& scenes);

signals:
    void portraitGenerated(const QString& characterId, const QString& imagePath);
    void sceneReferenceGenerated(const QString& sceneId, const QString& imagePath);
    void bibleBatchCompleted(const QString& type, int successCount, int failedCount);
    void allBibleImagesCompleted(int successCount, int failedCount);

protected:
    void finishBatch() override;

private slots:
    void onImageGenerated(const QwenImageClient::GenerateResult& result);
    void processNextItem();

private:
    BibleImageService(QObject* parent = nullptr);
    ~BibleImageService() = default;
    BibleImageService(const BibleImageService&) = delete;
    BibleImageService& operator=(const BibleImageService&) = delete;
    
    PromptBuilder::PromptResult buildCharacterPortraitPrompt(const Character& character);
    PromptBuilder::PromptResult buildSceneReferencePrompt(const Scene& scene);

    static BibleImageService* m_instance;
    
    QList<Character> m_pendingCharacters;
    QList<Scene> m_pendingScenes;
    int m_currentCharIndex;
    int m_currentSceneIndex;
    
    QString m_currentType;
    Character m_currentCharacter;
    Scene m_currentScene;
    bool m_combinedMode = false;
};

#endif // BIBLEIMAGESERVICE_H
