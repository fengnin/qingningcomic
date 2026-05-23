#ifndef NOVELVIEWMODEL_H
#define NOVELVIEWMODEL_H

#include "viewmodels/BaseViewModel.h"
#include "models/Novel.h"
#include "utils/SingletonUtils.h"
#include <QList>
#include <QVariantMap>

class NovelService;

class NovelViewModel : public BaseViewModel
{
    Q_OBJECT
    DECLARE_SINGLETON(NovelViewModel)

public:
    explicit NovelViewModel(QObject* parent = nullptr);
    ~NovelViewModel();
    
    void initialize() override;
    
    QList<Novel> novels() const { return m_novels; }
    Novel currentNovel() const { return m_currentNovel; }
    int totalCount() const { return m_totalCount; }
    
    void loadNovels(const QString& userId = QString(), int page = 1, int pageSize = 20);
    void loadNovel(const QString& novelId);
    void createNovel(const QString& userId, const QString& title, const QString& text,
                     const QVariantMap& metadata = QVariantMap());
    bool updateNovel(const QString& novelId, const QVariantMap& data);
    void updateNovelStatus(const QString& novelId, NovelStatus status);
    bool deleteNovel(const QString& novelId);
    void setCurrentNovel(const Novel& novel);
    
    void refreshCurrentNovel();

signals:
    void novelsLoaded(const QList<Novel>& novels, int totalCount);
    void novelLoaded(const Novel& novel);
    void novelCreated(const Novel& novel);
    void novelUpdated(const QString& novelId);
    void novelDeleted(const QString& novelId);
    void currentNovelChanged(const Novel& novel);
    void novelStatusChanged(const QString& novelId, NovelStatus status);

private slots:
    void onNovelCreated(const Novel& novel);
    void onNovelUpdated(const QString& novelId);
    void onNovelDeleted(const QString& novelId);
    void onNovelServiceStatusChanged(const QString& novelId, NovelStatus status);

private:
    void connectServiceSignals();
    void removeNovelFromList(const QString& novelId);
    
    NovelService* m_novelService;
    QList<Novel> m_novels;
    Novel m_currentNovel;
    int m_totalCount = 0;
};

#endif // NOVELVIEWMODEL_H
