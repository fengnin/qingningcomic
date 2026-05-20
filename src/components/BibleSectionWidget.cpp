#include "BibleSectionWidget.h"
#include "services/CharacterExtractor.h"
#include "services/SceneExtractor.h"
#include "data/FileStorage.h"
#include "components/ImageViewerDialog.h"
#include "utils/EncodingUtils.h"
#include "utils/Logger.h"
#include "utils/LayoutUtils.h"
#include <QApplication>
#include <QFileDialog>
#include <QLabel>
#include <QMessageBox>
#include <QScrollArea>
#include <QScrollBar>
#include <QTimer>
#include <QtConcurrent/QtConcurrent>

namespace {
const QString COLOR_HINT = "#6B7280";

QLabel* createEmptyStateLabel(const QString& text, QWidget* parent)
{
    QLabel* label = new QLabel(text, parent);
    label->setStyleSheet(QString("font-size: 14px; color: %1; border: none; background: transparent;").arg(COLOR_HINT));
    label->setAlignment(Qt::AlignCenter);
    return label;
}

inline void connectBibleItemSignals(BibleSectionWidget* widget, BibleItem* item, const Character&)
{
    QObject::connect(item, &BibleItem::characterDataChanged, widget, 
        [widget](const QString &id, const Character &character) {
            emit widget->characterDataChanged(id, character);
        });
}

inline void connectBibleItemSignals(BibleSectionWidget* widget, BibleItem* item, const Scene&)
{
    QObject::connect(item, &BibleItem::sceneDataChanged, widget, 
        [widget](const QString &id, const Scene &scene) {
            emit widget->sceneDataChanged(id, scene);
        });
}

 template <typename Model, typename NameFn, typename DetailsFn, typename IdFn, typename DataFn, typename ImageFn,
           typename EditFn, typename DataChangedFn, typename UploadFn, typename DeleteImageFn, typename DeleteFn>
void populateBibleItems(BibleSectionWidget* widget, QVBoxLayout* layout, const QList<Model>& items,
    BibleType type, const QString& emptyText, const QString& dialogTitle,
    NameFn nameFn, DetailsFn detailsFn, IdFn idFn, DataFn dataFn, ImageFn imageFn,
    EditFn editFn, DataChangedFn dataChangedFn, UploadFn uploadFn, DeleteImageFn deleteImageFn, DeleteFn deleteFn)
{
    if (!layout) {
        return;
    }

    if (items.isEmpty()) {
        layout->addWidget(createEmptyStateLabel(emptyText, widget));
        layout->addStretch();
        return;
    }

    for (const Model& model : items) {
        BibleItem *item = new BibleItem(nameFn(model), detailsFn(model), type);
        item->setItemId(idFn(model));
        dataFn(item, model);

        const QString imagePath = imageFn(model);
        if (!imagePath.isEmpty()) {
            item->setImage(FileStorage::instance()->getFullPath(imagePath));
        }

        QObject::connect(item, &BibleItem::editClicked, widget, [editFn](const QString &id, BibleType bibleType) {
            editFn(id, bibleType);
        });
        QObject::connect(item, &BibleItem::dataChanged, widget, [dataChangedFn](const QString &id, const QStringList& details) {
            dataChangedFn(id, details);
        });
        QObject::connect(item, &BibleItem::imageClicked, widget, [](const QString &imagePath) {
            ImageViewerDialog::showImage(nullptr, imagePath);
        });
        QObject::connect(item, &BibleItem::uploadClicked, widget, [uploadFn, dialogTitle](const QString &id, BibleType bibleType) {
            const QString imagePath = QFileDialog::getOpenFileName(nullptr, dialogTitle,
                QString(), QStringLiteral("图片文件 (*.png *.jpg *.jpeg)"));
            if (!imagePath.isEmpty()) {
                uploadFn(id, imagePath, bibleType);
            }
        });
        QObject::connect(item, &BibleItem::deleteImageClicked, widget, [deleteImageFn](const QString &id, BibleType bibleType) {
            deleteImageFn(id, bibleType);
        });
        QObject::connect(item, &BibleItem::deleteRequested, widget, [deleteFn](const QString &id, BibleType bibleType) {
            deleteFn(id, bibleType);
        });

        if (type == BibleType::Character) {
            QObject::connect(item, &BibleItem::versionClicked, widget, [widget](const QString &id, const QPoint &anchorPos) {
                emit widget->characterVersionClicked(id, anchorPos);
            });
        }

        connectBibleItemSignals(widget, item, Model());

        layout->addWidget(item);
    }

    layout->addStretch();
}
}

BibleSectionWidget::BibleSectionWidget(QWidget* parent)
    : QWidget(parent)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(16);

    QHBoxLayout *cardsLayout = new QHBoxLayout();
    cardsLayout->setContentsMargins(0, 0, 0, 0);
    cardsLayout->setSpacing(16);

    cardsLayout->addWidget(createBibleCard(tr("角色圣经"), m_characterCountLabel, m_characterContainer, BibleType::Character), 1);
    cardsLayout->addWidget(createBibleCard(tr("场景圣经"), m_sceneCountLabel, m_sceneContainer, BibleType::Scene), 1);

    mainLayout->addLayout(cardsLayout);
}

BibleSectionWidget::~BibleSectionWidget()
{
    if (m_watcher) {
        m_watcher->cancel();
        m_watcher->waitForFinished();
    }
}

QFrame* BibleSectionWidget::createBibleCard(const QString& title, QLabel*& countLabel, QWidget*& container, BibleType type)
{
    QFrame *card = new QFrame(this);
    card->setObjectName(type == BibleType::Character ? "characterBibleCard" : "sceneBibleCard");
    card->setStyleSheet(QString(R"(
        #%1 {
            background: white;
            border-radius: 16px;
            border: 1px solid #E5E7EB;
        }
    )").arg(card->objectName()));

    QVBoxLayout *cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(24, 20, 24, 20);
    cardLayout->setSpacing(16);

    QWidget *headerRow = new QWidget(card);
    QHBoxLayout *headerLayout = new QHBoxLayout(headerRow);
    headerLayout->setContentsMargins(0, 0, 0, 0);
    headerLayout->setSpacing(8);

    QLabel *titleLabel = new QLabel(title, headerRow);
    titleLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #212121; border: none; background: transparent;");
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();

    countLabel = new QLabel(card);
    countLabel->setStyleSheet(QString("font-size: 14px; color: %1; border: none; background: transparent;").arg(COLOR_HINT));
    headerLayout->addWidget(countLabel);

    cardLayout->addWidget(headerRow);

    QScrollArea *scrollArea = new QScrollArea(card);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setStyleSheet("QScrollArea { background: transparent; border: none; }");
    scrollArea->setMinimumHeight(420);

    container = new QWidget(scrollArea);
    QVBoxLayout *containerLayout = new QVBoxLayout(container);
    containerLayout->setContentsMargins(0, 0, 0, 0);
    containerLayout->setSpacing(8);
    scrollArea->setWidget(container);
    
    if (type == BibleType::Character) {
        m_characterScrollArea = scrollArea;
    } else {
        m_sceneScrollArea = scrollArea;
    }

    cardLayout->addWidget(scrollArea);
    return card;
}

void BibleSectionWidget::setNovelId(const QString& novelId)
{
    LOG_INFO("BibleSectionWidget", QString("setNovelId called: %1").arg(novelId));
    m_novelId = novelId;
    QTimer::singleShot(0, this, [this, novelId]() {
        if (m_novelId != novelId) {
            return;
        }
        refreshBible();
    });
}

void BibleSectionWidget::refreshBible()
{
    LOG_INFO("BibleSectionWidget", QString("refreshBible called, novelId=%1").arg(m_novelId));
    if (m_novelId.isEmpty()) {
        clearBible();
        return;
    }

    const QString novelId = m_novelId;

    // Cancel any in-flight fetch for a previous novelId.
    if (m_watcher) {
        disconnect(m_watcher, nullptr, this, nullptr);
        m_watcher->cancel();
        m_watcher->deleteLater();
        m_watcher = nullptr;
    }

    m_watcher = new QFutureWatcher<BibleData>(this);
    QFutureWatcher<BibleData>* watcher = m_watcher;
    connect(watcher, &QFutureWatcher<BibleData>::finished, this, [this, novelId, watcher]() {
        LOG_INFO("BibleSectionWidget", QString("finished signal received, canceled=%1, novelId=%2, m_novelId=%3")
            .arg(watcher->isCanceled()).arg(novelId).arg(m_novelId));
        if (watcher->isCanceled()) {
            watcher->deleteLater();
            if (m_watcher == watcher) m_watcher = nullptr;
            return;
        }
        if (m_novelId != novelId) {
            watcher->deleteLater();
            if (m_watcher == watcher) m_watcher = nullptr;
            return;
        }

        BibleData data = watcher->result();
        watcher->deleteLater();
        m_watcher = nullptr;

        applyBibleData(data);
    });

    m_watcher->setFuture(QtConcurrent::run([novelId]() -> BibleData {
        BibleData d;
        d.characters = CharacterExtractor::instance()->getCharactersByNovel(novelId);
        d.scenes     = SceneExtractor::instance()->getScenesByNovel(novelId);
        LOG_INFO("BibleSectionWidget", QString("Background fetch done: %1 chars, %2 scenes for novel %3")
            .arg(d.characters.size()).arg(d.scenes.size()).arg(novelId));
        return d;
    }));
}

void BibleSectionWidget::applyBibleData(const BibleData& data)
{
    int characterScrollPos = m_characterScrollArea ? m_characterScrollArea->verticalScrollBar()->value() : 0;
    int sceneScrollPos     = m_sceneScrollArea     ? m_sceneScrollArea->verticalScrollBar()->value()     : 0;

    setUpdatesEnabled(false);
    clearBibleContents();

    m_characterCount = data.characters.size();
    m_sceneCount     = data.scenes.size();

    if (m_characterContainer)
        populateCharacterBible(qobject_cast<QVBoxLayout*>(m_characterContainer->layout()), data.characters);
    if (m_sceneContainer)
        populateSceneBible(qobject_cast<QVBoxLayout*>(m_sceneContainer->layout()), data.scenes);

    updateCountLabels();
    setUpdatesEnabled(true);

    if (m_characterScrollArea)
        m_characterScrollArea->verticalScrollBar()->setValue(characterScrollPos);
    if (m_sceneScrollArea)
        m_sceneScrollArea->verticalScrollBar()->setValue(sceneScrollPos);

    emit characterCountChanged(m_characterCount);
    emit sceneCountChanged(m_sceneCount);

    LOG_INFO("BibleSectionWidget", QString("Refreshed bible: %1 characters, %2 scenes")
        .arg(m_characterCount).arg(m_sceneCount));
}

void BibleSectionWidget::clearBible()
{
    m_characterCount = 0;
    m_sceneCount = 0;
    clearBibleContents();
    updateCountLabels();
}

void BibleSectionWidget::populateCharacterBible(QVBoxLayout* layout, const QList<Character>& characters)
{
    populateBibleItems(this, layout, characters, BibleType::Character, tr("暂无角色"),
        tr("选择角色图片"),
        [](const Character& character) { return character.name(); },
        [this](const Character& character) { return buildCharacterDetails(character); },
        [](const Character& character) { return character.id(); },
        [](BibleItem* item, const Character& character) { item->setCharacterData(character); },
        [](const Character& character) { return character.portraitPath(); },
        [this](const QString& id, BibleType type) { emit bibleItemEditRequested(id, type); },
        [this](const QString& id, const QStringList& details) { emit bibleItemDataChanged(id, details); },
        [this](const QString& id, const QString& imagePath, BibleType type) { emit bibleItemImageUpdated(id, imagePath, type); },
        [this](const QString& id, BibleType type) { emit bibleItemImageUpdated(id, QString(), type); },
        [this](const QString& id, BibleType type) { emit bibleItemDeleteRequested(id, type); });
}

QStringList BibleSectionWidget::buildCharacterDetails(const Character& character) const
{
    QStringList details;
    const CharacterAppearance app = character.appearance();

    LOG_INFO("BibleSectionWidget", QString("Rendering character bible: id=%1, name=%2, role=%3")
        .arg(character.id())
        .arg(character.name())
        .arg(character.role()));

    auto addDisplayLine = [&details](const QString& label, const QString& value) {
        details << QStringLiteral("%1: %2").arg(label, value.isEmpty() ? QString::fromUtf8("未填写") : value);
    };

    addDisplayLine(tr("性别"), app.gender);
    addDisplayLine(tr("年龄"), app.age > 0 ? tr("%1岁").arg(app.age) : QString());
    addDisplayLine(tr("发色"), app.hairColor);
    addDisplayLine(tr("发型"), app.hairStyle);
    addDisplayLine(tr("瞳色"), app.eyeColor);
    addDisplayLine(tr("体型"), app.build);
    addDisplayLine(tr("服饰"), app.clothing.join(tr("、")));
    addDisplayLine(tr("明显特征"), app.distinctiveFeatures.join(tr("、")));
    addDisplayLine(tr("个性"), character.personality().join(tr("、")));

    LOG_INFO("BibleSectionWidget", QString("Character bible details lines=%1, name=%2").arg(details.size()).arg(character.name()));

    return details;
}

void BibleSectionWidget::populateSceneBible(QVBoxLayout* layout, const QList<Scene>& scenes)
{
    populateBibleItems(this, layout, scenes, BibleType::Scene, tr("暂无场景"),
        tr("选择场景图片"),
        [](const Scene& scene) { return scene.name(); },
        [](const Scene& scene) { return scene.details().toDisplayStrings(); },
        [](const Scene& scene) { return scene.sceneId(); },
        [](BibleItem* item, const Scene& scene) { item->setSceneData(scene); },
        [](const Scene& scene) { return scene.referenceImagePath(); },
        [this](const QString& id, BibleType type) { emit bibleItemEditRequested(id, type); },
        [this](const QString& id, const QStringList& details) { emit bibleItemDataChanged(id, details); },
        [this](const QString& id, const QString& imagePath, BibleType type) { emit bibleItemImageUpdated(id, imagePath, type); },
        [this](const QString& id, BibleType type) { emit bibleItemImageUpdated(id, QString(), type); },
        [this](const QString& id, BibleType type) { emit bibleItemDeleteRequested(id, type); });
}

void BibleSectionWidget::updateCountLabels()
{
    if (m_characterCountLabel) {
        m_characterCountLabel->setText(tr("角色: %1").arg(m_characterCount));
    }
    if (m_sceneCountLabel) {
        m_sceneCountLabel->setText(tr("场景: %1").arg(m_sceneCount));
    }
}

void BibleSectionWidget::clearBibleContents()
{
    LayoutUtils::clearContainerLayout(m_characterContainer);
    LayoutUtils::clearContainerLayout(m_sceneContainer);
}
