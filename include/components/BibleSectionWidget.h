#ifndef BIBLESECTIONWIDGET_H
#define BIBLESECTIONWIDGET_H

#include "models/Character.h"
#include "models/Scene.h"
#include "components/BibleItem.h"
#include <QFrame>
#include <QFutureWatcher>
#include <QHBoxLayout>
#include <QLabel>
#include <QWidget>
#include <QVBoxLayout>
#include <QScrollArea>

class Novel;

class BibleSectionWidget : public QWidget
{
    Q_OBJECT

public:
    explicit BibleSectionWidget(QWidget* parent = nullptr);
    ~BibleSectionWidget();

    void setNovelId(const QString& novelId);
    void refreshBible();
    void clearBible();

    int characterCount() const { return m_characterCount; }
    int sceneCount() const { return m_sceneCount; }

signals:
    void characterCountChanged(int count);
    void sceneCountChanged(int count);
    void bibleItemEditRequested(const QString& id, BibleType type);
    void bibleItemDataChanged(const QString& id, const QStringList& details);
    void bibleItemImageUpdated(const QString& id, const QString& imagePath, BibleType type);
    void bibleItemDeleteRequested(const QString& id, BibleType type);
    void characterDataChanged(const QString& id, const Character& character);
    void sceneDataChanged(const QString& id, const Scene& scene);
    void characterVersionClicked(const QString& id, const QPoint& anchorGlobalPos);

private:
    QFrame* createBibleCard(const QString& title, QLabel*& countLabel, QWidget*& container, BibleType type);
    QStringList buildCharacterDetails(const Character& character) const;
    void populateCharacterBible(QVBoxLayout* layout, const QList<Character>& characters);
    void populateSceneBible(QVBoxLayout* layout, const QList<Scene>& scenes);
    struct BibleData {
        QList<Character> characters;
        QList<Scene>     scenes;
    };

    void updateCountLabels();
    void clearBibleContents();
    void applyBibleData(const BibleData& data);

    QString m_novelId;
    QWidget* m_characterContainer = nullptr;
    QWidget* m_sceneContainer = nullptr;
    QScrollArea* m_characterScrollArea = nullptr;
    QScrollArea* m_sceneScrollArea = nullptr;
    QLabel* m_characterCountLabel = nullptr;
    QLabel* m_sceneCountLabel = nullptr;
    int m_characterCount = 0;
    int m_sceneCount = 0;

    QFutureWatcher<BibleData>* m_watcher = nullptr;
};

#endif // BIBLESECTIONWIDGET_H
