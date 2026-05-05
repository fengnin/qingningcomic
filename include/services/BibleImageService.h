#ifndef BIBLEIMAGESERVICE_H
#define BIBLEIMAGESERVICE_H

#include "services/BatchImageProcessor.h"
#include "models/Character.h"
#include "models/Scene.h"
#include "api/QwenImageClient.h"
#include "api/VolcEngineImageClient.h"
#include "utils/PromptBuilder.h"
#include <mutex>
#include <QMap>

namespace BibleImageConstants {
    const QString TYPE_CHARACTER = QStringLiteral("character");
    const QString TYPE_SCENE = QStringLiteral("scene");
}

struct PendingImageRequest {
    QString type;
    Character character;
    Scene scene;
    QString prompt;
    QString negativePrompt;
    int retryCount = 0;
};

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
    
    void handleImageGenerated(const QString& requestId, const QByteArray& imageData, bool success, const QString& errorMessage);
    void startImageGeneration(const QString& requestId, const QString& prompt, const QString& negativePrompt, const QString& type, const Character& character, const Scene& scene);
    void saveAndEmitResult(const QString& requestId, const QByteArray& imageData);
    void retryRequest(const QString& requestId, const QString& errorMessage);
    void clearPendingRequest(const QString& requestId);
    bool maybeFinishBatch();
    void scheduleNextItem();
    void handleRetryExhausted(const QString& requestId, const QString& errorMessage);
    
    QString saveCharacterImage(const PendingImageRequest& request, const QByteArray& imageData);
    QString saveSceneImage(const PendingImageRequest& request, const QByteArray& imageData);
    bool updateCharacterPortraitRecord(Character character, const QString& imagePath);
    bool updateSceneReferenceRecord(Scene scene, const QString& imagePath);
    
    bool processNextCharacter();
    bool processNextScene();
    
    QJsonObject buildAppearanceJson(const CharacterAppearance& app);
    QJsonObject buildSceneJson(const Scene& scene);

    static BibleImageService* m_instance;
    static std::once_flag m_instanceOnceFlag;
    
    mutable std::mutex m_stateMutex;
    
    QList<Character> m_pendingCharacters;
    QList<Scene> m_pendingScenes;
    int m_currentCharIndex;
    int m_currentSceneIndex;
    
    QString m_currentType;
    bool m_combinedMode = false;
    
    QMap<QString, PendingImageRequest> m_pendingRequests;
    
    static constexpr int MAX_RETRY_COUNT = 3;
};

#endif // BIBLEIMAGESERVICE_H
