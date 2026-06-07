#ifndef BIBLEIMAGESERVICE_H
#define BIBLEIMAGESERVICE_H

#include "services/BatchImageProcessor.h"
#include "models/Character.h"
#include "models/Scene.h"
#include "api/QwenImageClient.h"
#include "api/VolcEngineImageClient.h"
#include "services/CharacterExtractor.h"
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
    // 编辑路径专用：非空表示这是 portrait edit 请求
    QByteArray sourceImage;
    QString baseVersionId;        // 编辑基底版本 ID（空时取当前 current）
    QList<CharacterFieldDiff> diff;
    int sourceChapter = 0;
};

class BibleImageService : public BatchImageProcessor
{
    Q_OBJECT

public:
    static BibleImageService* instance();
    
    void generateCharacterPortraitAsync(const Character& character);
    void generateSceneReferenceAsync(const Scene& scene);

    // 在已有版本基础上调用 qwen-image-edit 生成新版本（半自动应用编辑草稿）
    // baseVersionId 为空时使用 character.currentPortraitVersionId() 或回退到 portrait_path
    void editCharacterPortraitAsync(const Character& character,
                                    const QString& editPrompt,
                                    const QString& baseVersionId,
                                    const QList<CharacterFieldDiff>& diff,
                                    int sourceChapter);

    void generateCharacterPortraits(const QList<Character>& characters);
    void generateSceneReferences(const QList<Scene>& scenes);

    void generateAllBibleImages(const QList<Character>& characters, const QList<Scene>& scenes);

signals:
    void portraitGenerated(const QString& characterId, const QString& imagePath);
    void sceneReferenceGenerated(const QString& sceneId, const QString& imagePath);
    void portraitVersionGenerated(const QString& characterId, const QString& versionId, const QString& imagePath);
    void portraitEditFailed(const QString& characterId, const QString& errorMessage);
    void bibleBatchCompleted(const QString& type, int successCount, int failedCount);
    void allBibleImagesCompleted(int successCount, int failedCount);

protected:
    void finishBatch() override;

private slots:
    void onQwenImageGenerated(const QwenImageClient::GenerateResult& result);
    void onQwenImageEdited(const QwenImageClient::GenerateResult& result);
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

    // 编辑路径完成回调：写新版本 + 更新 current 指针
    void saveAndEmitEditResult(const QString& requestId, const QByteArray& imageData);
    // 首版生成路径：把 portrait 同步落到版本表 v1
    bool ensureFirstPortraitVersionRecorded(const Character& character, const QString& imagePath);
    // 构造版本行 + 写 DB + 更新 current 指针（两条路径共用），返回新版本 id，失败返回空串
    QString insertPortraitVersionAndSetCurrent(const Character& character,
                                            const QString& portraitPath,
                                            int versionNo,
                                            const QString& baseVersionId,
                                            const QString& editPrompt,
                                            const QJsonObject& fieldDiff,
                                            const QJsonObject& appearanceSnapshot,
                                            int sourceChapter);

    struct CharacterImageProcessResult {
        QByteArray imageData;
        bool qualityAccepted = true;
    };

    // 角色图片后处理：白化背景 + 质检 + 安全裁边
    CharacterImageProcessResult processCharacterImageData(const QByteArray& raw) const;
    void handleRejectedCharacterImage(const QString& requestId, const PendingImageRequest& request);
    // editCharacterPortraitAsync 子步骤
    bool resolveEditBaseVersion(const Character& character,
                                const QString& requestedBaseVersionId,
                                QString& outVersionId,
                                QString& outRelativePath);
    QByteArray readSourceImageBytes(const QString& relativePath) const;
    void failEdit(const QString& characterId, const QString& reason, const QString& requestId = QString());
    
    bool processNextCharacter();
    bool processNextScene();
    
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
