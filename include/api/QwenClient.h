#ifndef QWENCLIENT_H
#define QWENCLIENT_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QJsonObject>
#include <QJsonArray>
#include <QStringList>
#include <QSet>
#include <QFuture>
#include <QtConcurrent>
#include <QMutex>
#include <QByteArray>
#include "utils/SingletonUtils.h"

class QNetworkReply;
class QTimer;

class QwenClient : public QObject
{
    Q_OBJECT
    DECLARE_SINGLETON_MEMBERS(QwenClient)

public:
    struct Config {
        QString apiKey;
        QString endpoint;
        int maxRetries = 3;
        QString model = "qwen-plus";
        int maxChunkLength = 8000;
        int maxTokens = 32768;
        int requestTimeout = 120000;
    };

    struct StoryboardResult {
        QJsonArray panels;
        QJsonArray characters;
        QJsonArray scenes;
        int totalPages = 0;
        bool success = false;
        QString errorMessage;
    };
    
    bool init(const Config& config);
    
    StoryboardResult generateStoryboard(
        const QString& text,
        const QJsonObject& jsonSchema,
        bool strictMode = true,
        const QJsonArray& existingCharacters = QJsonArray(),
        const QJsonArray& existingScenes = QJsonArray(),
        int chapterNumber = 0
    );

    QJsonObject parseChangeRequest(
        const QString& naturalLanguage,
        const QJsonObject& jsonSchema,
        const QJsonObject& context = QJsonObject()
    );

    QString rewriteDialogue(const QString& originalDialogue, const QString& instruction);
    
    QJsonObject correctJson(const QJsonObject& invalidJson, const QStringList& errors);

    QString lastError() const;
    bool isInitialized() const;
    int maxChunkLength() const { return m_config.maxChunkLength; }
    
    void generateStoryboardStream(
        const QString& text,
        const QJsonObject& jsonSchema,
        bool strictMode = true,
        const QJsonArray& existingCharacters = QJsonArray(),
        const QJsonArray& existingScenes = QJsonArray(),
        int chapterNumber = 0
    );

signals:
    void progressChanged(int current, int total, const QString& message);
    void errorOccurred(const QString& operation, const QString& message);
    void streamReceived(const QString& content);
    void streamCompleted(const QJsonObject& result);
    void streamProgress(const QString& partialContent);

public:
    QwenClient();
    ~QwenClient();

private:
    static constexpr int PANELS_PER_PAGE = 6;

    struct StoryboardPromptContext {
        QString systemPrompt;
        QString userMessage;
    };

    bool checkInitialized(const QString& operation);
    
    QJsonArray buildMessages(const QString& systemPrompt, const QString& userPrompt);
    StoryboardPromptContext buildStoryboardPromptContext(
        const QString& text,
        const QJsonArray& existingCharacters,
        const QJsonArray& existingScenes,
        int chapterNumber) const;

    QStringList splitTextIntelligently(const QString& text, int maxLength);
    QStringList extractSentences(const QString& text);
    QJsonObject mergeStoryboards(const QList<QJsonObject>& storyboards);
    void mergeCharacters(QMap<QString, QJsonObject>& charMap, const QJsonArray& characters);
    void mergeScenes(QMap<QString, QJsonObject>& sceneMap, const QJsonArray& scenes);

    QJsonObject callStoryboardApi(
        const QString& text,
        const QJsonObject& schema,
        bool strictMode,
        const QJsonArray& existingCharacters,
        const QJsonArray& existingScenes,
        int chapterNumber
    );

    QJsonObject sendApiRequest(const QJsonObject& payload);
    void sendApiRequestStream(const QJsonObject& payload);
    
    QNetworkRequest createApiRequest() const;
    QString parseBadRequestError(const QByteArray& errorData);
    void handleStreamFinished(QNetworkReply* reply, QTimer* timer);
    void handleStreamTimeout(QNetworkReply* reply);
    QJsonObject buildJsonSchemaPayload(
        const QString& systemPrompt,
        const QString& userMessage,
        const QJsonObject& schema,
        bool strictMode,
        const QString& schemaName
    );
    QJsonObject buildBasePayload(
        const QString& systemPrompt,
        const QString& userMessage,
        double temperature,
        int maxTokens
    );
    QJsonObject buildSimplePayload(
        const QString& systemPrompt,
        const QString& userMessage,
        double temperature,
        int maxTokens
    );
    QJsonObject sendPromptRequest(const QJsonObject& payload);
    QJsonObject extractApiResponse(const QJsonObject& response);

    QJsonObject ensureStoryboardShape(const QJsonObject& storyboard);
    
    QString getStringField(const QJsonObject& obj, const QString& key, const QString& defaultValue = QString());
    QString getStringFieldAny(const QJsonObject& obj, const QStringList& keys, const QString& defaultValue = QString());
    QJsonObject getObjectField(const QJsonObject& obj, const QString& key, const QJsonObject& defaultValue = QJsonObject());
    QJsonArray getArrayField(const QJsonObject& obj, const QString& key, const QJsonArray& defaultValue = QJsonArray());
    int getIntField(const QJsonObject& obj, const QString& key, int defaultValue = 0);
    
    QJsonObject normalizePanel(const QJsonObject& panel, int index, QMap<QString, QJsonObject>& extractedChars, QMap<QString, QJsonObject>& extractedScenes, int& sceneCounter, const QMap<QString, QString>& existingSceneLookup);
    QJsonObject buildBackground(const QJsonObject& panel);
    QJsonObject buildAtmosphere(const QJsonObject& panel);
    QJsonArray normalizeCharacters(const QJsonArray& chars, QMap<QString, QJsonObject>& extractedChars);
    QJsonObject convertSceneToPanel(const QJsonObject& sceneObj, int index);
    
    QJsonObject buildPanelBackground(const QJsonObject& sceneObj);
    QJsonObject buildPanelAtmosphere(const QJsonObject& sceneObj);
    QJsonArray extractCharactersFromScene(const QJsonObject& sceneObj, const QString& visualDesc);
    QJsonArray extractCharactersFromText(const QString& text);
    
    void setPanelPageAndIndex(QJsonObject& panel, int index);
    void setPanelShotInfo(QJsonObject& panel, const QJsonObject& source);
    void extractSceneInfo(QJsonObject& panel, QMap<QString, QJsonObject>& extractedScenes, int& sceneCounter, const QMap<QString, QString>& existingSceneLookup);
    void reconcileSpeakerSideFromFramePosition(QJsonObject& panel);
    void injectPositionCueIntoVisualPromptCn(QJsonObject& panel);
    
    QJsonArray mergeJsonArrays(const QJsonArray& existing, const QJsonArray& extracted, 
                                const QString& keyField, QSet<QString>& seenKeys);
    QSet<QString> extractKeysFromArray(const QJsonArray& arr, const QString& keyField);
    
    QJsonObject convertScenesToPanelsIfNeeded(const QJsonObject& storyboard);

    QList<QJsonObject> processChunksParallel(
        const QStringList& chunks,
        const QJsonObject& jsonSchema,
        bool strictMode,
        const QJsonArray& existingCharacters,
        const QJsonArray& existingScenes,
        int chapterNumber
    );
    
    void exponentialBackoff(int attempt);
    void log(const QString& content);
    static bool containsChinese(const QString& text);
    void logRequest(const QString& model, const QString& text, const QString& systemPrompt, const QString& userMessage);
    void logResponse(const QString& content, const QString& finishReason, const QJsonObject& usage, qint64 elapsedMs);

    Config m_config;
    QString m_lastError;
    QString m_logFilePath;
    bool m_initialized;
    QString m_streamAccumulatedContent;
    QByteArray m_streamBuffer;
    bool m_streamInProgress = false;
    bool m_streamAborted = false;
};

#endif // QWENCLIENT_H
