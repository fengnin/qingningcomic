#ifndef BIBLEIMAGESERVICE_H
#define BIBLEIMAGESERVICE_H

#include "services/BatchImageProcessor.h"
#include "models/Character.h"
#include "models/Scene.h"
#include "api/QwenImageClient.h"
#include "api/VolcEngineImageClient.h"
#include "utils/PromptBuilder.h"
#include <mutex>

namespace BibleImageConstants {
    const QString TYPE_CHARACTER = QStringLiteral("character");
    const QString TYPE_SCENE = QStringLiteral("scene");
}

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
    void onQwenImageGenerated(const QwenImageClient::GenerateResult& result);
    void onVolcEngineImageGenerated(const VolcEngineImageClient::GenerateResult& result);
    void processNextItem();

private:
    BibleImageService(QObject* parent = nullptr);
    ~BibleImageService();
    BibleImageService(const BibleImageService&) = delete;
    BibleImageService& operator=(const BibleImageService&) = delete;
    
    PromptBuilder::PromptResult buildCharacterPortraitPrompt(const Character& character);
    PromptBuilder::PromptResult buildSceneReferencePrompt(const Scene& scene);
    
    void handleImageGenerated(const QByteArray& imageData, bool success, const QString& errorMessage);
    bool validateAndHandleResult(const QString& requestId, const QByteArray& imageData, bool success, const QString& errorMessage);
    void startImageGeneration(const QString& requestId, const QString& prompt, const QString& negativePrompt = QString());
    void saveAndEmitResult(const QByteArray& imageData);
    void retryCurrentItem();
    void clearCurrentState();
    
    QString saveCharacterImage(const QByteArray& imageData);
    QString saveSceneImage(const QByteArray& imageData);
    
    bool processNextCharacter();
    bool processNextScene();
    
    QJsonObject buildAppearanceJson(const CharacterAppearance& app);
    QJsonObject buildSceneJson(const Scene& scene);

    static BibleImageService* m_instance;
    static std::once_flag m_instanceOnceFlag;
    
    QList<Character> m_pendingCharacters;
    QList<Scene> m_pendingScenes;
    int m_currentCharIndex;
    int m_currentSceneIndex;
    
    QString m_currentType;
    Character m_currentCharacter;
    Scene m_currentScene;
    bool m_combinedMode = false;
    
    int m_currentRetryCount = 0;
    QString m_currentPrompt;
    QString m_currentNegativePrompt;
    QString m_currentRequestId;
    static constexpr int MAX_RETRY_COUNT = 3;
};

#endif // BIBLEIMAGESERVICE_H
