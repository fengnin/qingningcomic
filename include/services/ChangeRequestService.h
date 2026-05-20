#ifndef CHANGEREQUESTSERVICE_H
#define CHANGEREQUESTSERVICE_H

#include "models/ChangeRequest.h"
#include "data/DatabaseManager.h"
#include "models/Panel.h"
#include "services/ImageService.h"
#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include <QFuture>

class QwenClient;
class ImageService;
struct ChangeRequestOp;

/**
 * @brief 自然语言改稿服务
 * 
 * 处理用户的自然语言改稿请求，包括：
 * 1. 解析自然语言生成 DSL
 * 2. 执行改稿操作（art/dialogue/layout/style）
 * 3. 更新面板数据
 */
class ChangeRequestService : public QObject
{
    Q_OBJECT

public:
    explicit ChangeRequestService(DatabaseManager* db, QObject* parent = nullptr);
    ~ChangeRequestService() = default;

    /**
     * @brief 创建改稿请求
     * @param novelId 小说 ID
     * @param naturalLanguage 自然语言指令
     * @param context 上下文信息
     * @return 改稿请求对象
     */
    ChangeRequest createChangeRequest(
        const QString& novelId,
        const QString& naturalLanguage,
        const QJsonObject& context = QJsonObject()
    );

    /**
     * @brief 执行改稿请求
     * @param crId 改稿请求 ID
     * @return 执行结果
     */
    QJsonObject executeChangeRequest(const QString& crId);

    /**
     * @brief 异步执行改稿请求
     */
    void executeChangeRequestAsync(const QString& crId);

    /**
     * @brief 获取改稿请求
     */
    ChangeRequest getChangeRequest(const QString& crId, const QString& novelId);

    /**
     * @brief 获取小说的所有改稿请求
     */
    QList<ChangeRequest> getChangeRequestsByNovel(const QString& novelId);

    /**
     * @brief 保存 DSL 到数据库
     */
    bool saveChangeRequestDsl(const QString& crId, const QString& novelId, 
                               const ChangeRequestDsl& dsl);

    QString lastError() const { return m_lastError; }

signals:
    void changeRequestCreated(const ChangeRequest& cr);
    void changeRequestCompleted(const QString& crId, const QJsonObject& result);
    void changeRequestFailed(const QString& crId, const QString& error);
    void progressChanged(const QString& crId, int current, int total, const QString& message);

private:
    ChangeRequestDsl parseNaturalLanguage(const QString& naturalLanguage, 
                                           const QJsonObject& context);

    QJsonObject executeArtOperation(const ChangeRequestDsl& dsl, 
                                     const ChangeRequestOp& op,
                                     const QString& crId);
    QJsonObject executeSetExpressionOperation(const ChangeRequestOp& op,
                                              Panel panel,
                                              const QString& panelId,
                                              const QString& mode);
    QJsonObject executeDialogueOperation(const ChangeRequestDsl& dsl, 
                                          const ChangeRequestOp& op,
                                          const QString& crId);
    QJsonObject executeLayoutOperation(const ChangeRequestDsl& dsl, 
                                        const ChangeRequestOp& op,
                                        const QString& crId);
    QJsonObject executeStyleOperation(const ChangeRequestDsl& dsl, 
                                       const ChangeRequestOp& op,
                                       const QString& crId);
    ChangeRequestDsl prepareChangeRequestDsl(const ChangeRequest& cr);
    void resolveDslTarget(ChangeRequestDsl& dsl, const QString& novelId, const QJsonObject& context);
    QJsonArray executeDslOperations(const QString& crId, const ChangeRequestDsl& dsl);
    QJsonObject executeOperationByType(const ChangeRequestDsl& dsl,
                                       const ChangeRequestOp& op,
                                       const QString& crId);

    Panel loadPanelById(const QString& panelId);
    Panel requirePanelForDsl(const ChangeRequestDsl& dsl, const QString& operationKind);
    bool updatePanel(const Panel& panel);
    bool updatePanelDialogue(const QString& panelId, const QList<DialogueLine>& dialogue, const QString& newText = QString());
    bool updatePanelLayout(const QString& panelId, int width, int height);
    bool updateStoryboardStyle(const QString& storyboardId, const QJsonObject& styleOverrides);
    QJsonObject buildPanelContentForWrite(const Panel& panel) const;
    ImageService::EditHint buildExpressionEditHint(const QString& expression) const;
    ImageService::EditHint buildSubjectReplacementEditHint(const QJsonObject& params) const;
    ImageService::EditHint buildLocalObjectReplacementEditHint() const;
    struct PromptAndHint {
        QString prompt;
        ImageService::EditHint hint;
    };
    PromptAndHint buildPromptAndHintForEditIntent(const QJsonObject& params,
                                                  const QString& action,
                                                  const QString& editIntent,
                                                  const QString& rawPrompt,
                                                  const QString& sourceText,
                                                  const Panel& panel) const;
    bool ensureChangeRequestColumns();
    QString resolveStoryboardIdForChangeRequest(const QString& novelId, const QJsonObject& context);
    QString resolveImageModeForPanel(const ChangeRequestOp& op, const Panel& panel) const;
    bool runPanelImageGeneration(const QString& panelId,
                                 const QString& mode,
                                 QJsonObject& output,
                                 const ImageService::EditHint& editHint = ImageService::EditHint());
    bool executeDirectPanelEdit(const QString& panelId,
                                const QString& editPrompt,
                                const QString& mode,
                                QJsonObject& output);
    QJsonObject buildStyleOverrides(const ChangeRequestOp& op) const;
    void restorePanelAfterFailedArtEdit(const Panel& originalPanel, const QString& panelId);
    bool writeChangeRequestStatus(const QString& crId, const QString& novelId, const QString& status, const QString& error = QString());

    QString generateEditS3Key(const QString& panelId, const QString& crId, 
                               const QString& action, const QString& mode);

    DatabaseManager* m_db;
    QwenClient* m_qwenClient;
    ImageService* m_imageService;
    QString m_lastError;
};

#endif // CHANGEREQUESTSERVICE_H
