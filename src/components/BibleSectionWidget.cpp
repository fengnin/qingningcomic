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

namespace {
const QString COLOR_HINT = "#6B7280";

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
        QLabel *emptyLabel = new QLabel(emptyText);
        emptyLabel->setStyleSheet(QString("font-size: 14px; color: %1; border: none; background: transparent;").arg(COLOR_HINT));
        emptyLabel->setAlignment(Qt::AlignCenter);
        layout->addWidget(emptyLabel);
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
    m_novelId = novelId;
    refreshBible();
}

void BibleSectionWidget::refreshBible()
{
    if (m_novelId.isEmpty()) {
        clearBible();
        return;
    }

    int characterScrollPos = 0;
    int sceneScrollPos = 0;
    if (m_characterScrollArea) {
        characterScrollPos = m_characterScrollArea->verticalScrollBar()->value();
    }
    if (m_sceneScrollArea) {
        sceneScrollPos = m_sceneScrollArea->verticalScrollBar()->value();
    }

    setUpdatesEnabled(false);
    
    LayoutUtils::clearContainerLayout(m_characterContainer);
    LayoutUtils::clearContainerLayout(m_sceneContainer);

    const QList<Character> characters = CharacterExtractor::instance()->getCharactersByNovel(m_novelId);
    const QList<Scene> scenes = SceneExtractor::instance()->getScenesByNovel(m_novelId);

    if (characters.isEmpty() && scenes.isEmpty() && (m_characterCount > 0 || m_sceneCount > 0)) {
        setUpdatesEnabled(true);
        if (m_characterScrollArea) {
            m_characterScrollArea->verticalScrollBar()->setValue(characterScrollPos);
        }
        if (m_sceneScrollArea) {
            m_sceneScrollArea->verticalScrollBar()->setValue(sceneScrollPos);
        }
        LOG_WARNING("BibleSectionWidget", QString("Ignoring transient empty bible refresh for novel %1").arg(m_novelId));
        return;
    }

    m_characterCount = characters.size();
    m_sceneCount = scenes.size();

    if (m_characterContainer) {
        populateCharacterBible(qobject_cast<QVBoxLayout*>(m_characterContainer->layout()), characters);
    }
    if (m_sceneContainer) {
        populateSceneBible(qobject_cast<QVBoxLayout*>(m_sceneContainer->layout()), scenes);
    }

    if (m_characterCountLabel) {
        m_characterCountLabel->setText(tr("角色: %1").arg(m_characterCount));
    }
    if (m_sceneCountLabel) {
        m_sceneCountLabel->setText(tr("场景: %1").arg(m_sceneCount));
    }
    
    setUpdatesEnabled(true);

    if (m_characterScrollArea) {
        m_characterScrollArea->verticalScrollBar()->setValue(characterScrollPos);
    }
    if (m_sceneScrollArea) {
        m_sceneScrollArea->verticalScrollBar()->setValue(sceneScrollPos);
    }

    emit characterCountChanged(m_characterCount);
    emit sceneCountChanged(m_sceneCount);

    LOG_INFO("BibleSectionWidget", QString("Refreshed bible: %1 characters, %2 scenes")
        .arg(m_characterCount).arg(m_sceneCount));
}

void BibleSectionWidget::clearBible()
{
    LayoutUtils::clearContainerLayout(m_characterContainer);
    LayoutUtils::clearContainerLayout(m_sceneContainer);

    m_characterCount = 0;
    m_sceneCount = 0;

    if (m_characterCountLabel) {
        m_characterCountLabel->setText(tr("角色: %1").arg(m_characterCount));
    }
    if (m_sceneCountLabel) {
        m_sceneCountLabel->setText(tr("场景: %1").arg(m_sceneCount));
    }
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
