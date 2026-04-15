#ifndef CHANGEREQUESTSERVICE_H
#define CHANGEREQUESTSERVICE_H

#include "ChangeRequest.h"
#include "DatabaseManager.h"
#include "Panel.h"
#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include <QFuture>

class QwenClient;
class ImageService;

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
     * @brief 更新改稿请求状态
     */
    bool updateChangeRequestStatus(const QString& crId, const QString& novelId, 
                                    const QString& status, const QString& error = QString());

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
    QJsonObject executeDialogueOperation(const ChangeRequestDsl& dsl, 
                                          const ChangeRequestOp& op,
                                          const QString& crId);
    QJsonObject executeLayoutOperation(const ChangeRequestDsl& dsl, 
                                        const ChangeRequestOp& op,
                                        const QString& crId);
    QJsonObject executeStyleOperation(const ChangeRequestDsl& dsl, 
                                       const ChangeRequestOp& op,
                                       const QString& crId);

    Panel loadPanelById(const QString& panelId);
    bool updatePanel(const Panel& panel);
    bool updatePanelDialogue(const QString& panelId, const QList<DialogueLine>& dialogue);
    bool updatePanelLayout(const QString& panelId, int width, int height);
    bool updateStoryboardStyle(const QString& storyboardId, const QJsonObject& styleOverrides);
    bool reorderPanel(const QString& panelId, int fromIndex, int toIndex);
    QJsonObject buildPanelContentForWrite(const Panel& panel) const;
    bool ensureChangeRequestColumns();
    bool ensureStoryboardStyleOverridesColumn();

    QString generateEditS3Key(const QString& panelId, const QString& crId, 
                               const QString& action, const QString& mode);

    DatabaseManager* m_db;
    QwenClient* m_qwenClient;
    ImageService* m_imageService;
    QString m_lastError;
};

#endif // CHANGEREQUESTSERVICE_H
