#ifndef NOVELSERVICE_H
#define NOVELSERVICE_H

#include "services/BaseService.h"
#include "models/Novel.h"
#include "utils/SingletonUtils.h"

struct NovelPageResult {
    QList<Novel> novels;
    int total;
    int page;
    int pageSize;
    int totalPages;
    
    NovelPageResult() : total(0), page(1), pageSize(10), totalPages(0) {}
};

struct WhereClauseResult {
    QString clause;
    QVariantList values;
};

class NovelService : public BaseService
{
    Q_OBJECT
    DECLARE_SINGLETON_MEMBERS(NovelService)

public:
    NovelPageResult getNovels(const QString& userId, int page = 1, int pageSize = 10, const QString& statusFilter = QString());
    Novel getNovelById(const QString& novelId);
    Novel createNovel(const QString& userId, const QString& title, const QString& text = QString(), const QVariantMap& metadata = QVariantMap());
    bool updateNovel(const QString& novelId, const QVariantMap& data);
    bool deleteNovel(const QString& novelId);
    
    bool updateStatus(const QString& novelId, NovelStatus status);
    int countNovels(const QString& userId, const QString& status = QString());
    
    bool saveText(const QString& novelId, const QString& text);
    QString loadText(const QString& novelId);
    
    bool exists(const QString& novelId);
    bool hasPermission(const QString& novelId, const QString& userId);

    NovelService(DatabaseManager* db = nullptr, QObject* parent = nullptr);
    ~NovelService();

signals:
    void novelCreated(const Novel& novel);
    void novelUpdated(const QString& novelId);
    void novelDeleted(const QString& novelId);
    void statusChanged(const QString& novelId, NovelStatus status);

private:
    
    WhereClauseResult buildUserWhereClause(const QString& userId, const QString& statusFilter = QString());
    QString generateNovelId();
    Novel buildNovelFromMap(const QVariantMap& data);
    
    static const QString TABLE_NOVELS;
    static const QString TABLE_STORYBOARDS;
    static const QString TABLE_PANELS;
};

#endif // NOVELSERVICE_H
