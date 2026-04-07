#include "components/BibleItem.h"
#include "components/EditorStyles.h"
#include "utils/AsyncImageLoader.h"
#include "Logger.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QApplication>
#include <QEvent>
#include <QGraphicsDropShadowEffect>
#include <QRegularExpression>
#include <QFileInfo>
#include <QPixmapCache>

namespace {
    using EditorStyles::TRANSPARENT_BG;
    const QString SEPARATOR_STYLE = "background: #d4d4d4; border: none; max-height: 1px;";
}

BibleItem::BibleItem(const QString &name, const QStringList &details, BibleType type, QWidget *parent)
    : QFrame(parent)
    , m_nameLabel(nullptr)
    , m_detailsWidget(nullptr)
    , m_imageLabel(nullptr)
    , m_editBtn(nullptr)
    , m_name(name)
    , m_details(details)
    , m_isExpanded(false)
    , m_bibleType(type)
    , m_overlayWidget(nullptr)
    , m_editorCard(nullptr)
    , m_genderCombo(nullptr)
    , m_ageSpin(nullptr)
    , m_eyeColorEdit(nullptr)
    , m_bodyTypeEdit(nullptr)
    , m_hairColorEdit(nullptr)
    , m_hairStyleEdit(nullptr)
    , m_clothingEdit(nullptr)
    , m_featuresEdit(nullptr)
    , m_personalityEdit(nullptr)
    , m_sceneNameEdit(nullptr)
    , m_sceneDescEdit(nullptr)
    , m_buildingEdit(nullptr)
    , m_colorEdit(nullptr)
    , m_landmarkEdit(nullptr)
    , m_layoutEdit(nullptr)
    , m_atmosphereEdit(nullptr)
    , m_saveBtn(nullptr)
    , m_cancelBtn(nullptr)
{
    try {
        setupUI();
        setDetails(details);
    } catch (const std::exception& e) {
        LOG_ERROR("BibleItem", QString("Exception in BibleItem constructor: %1").arg(e.what()));
    } catch (...) {
        LOG_ERROR("BibleItem", "Unknown exception in BibleItem constructor");
    }
}

void BibleItem::setupUI()
{
    setObjectName("bibleItem");
    setStyleSheet(EditorStyles::bibleItemStyle());
    
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 14, 16, 14);
    mainLayout->setSpacing(12);
    
    QWidget *headerRow = new QWidget();
    headerRow->setStyleSheet(TRANSPARENT_BG);
    QHBoxLayout *headerLayout = new QHBoxLayout(headerRow);
    headerLayout->setContentsMargins(0, 0, 0, 0);
    headerLayout->setSpacing(8);
    
    m_nameLabel = new QLabel(QString("<b>%1</b>").arg(m_name));
    m_nameLabel->setStyleSheet(EditorStyles::bibleNameLabelStyle());
    m_nameLabel->setTextFormat(Qt::RichText);
    headerLayout->addWidget(m_nameLabel);
    
    headerLayout->addStretch();
    
    QString editText = QString::fromUtf8("\u7f16\u8f91");
    m_editBtn = new QPushButton(editText);
    m_editBtn->setFixedSize(64, 32);
    m_editBtn->setStyleSheet(EditorStyles::editButtonStyle());
    m_editBtn->setCursor(Qt::PointingHandCursor);
    connect(m_editBtn, &QPushButton::clicked, this, &BibleItem::onEditBtnClicked);
    headerLayout->addWidget(m_editBtn);
    
    mainLayout->addWidget(headerRow);
    
    m_detailsWidget = new QWidget();
    m_detailsWidget->setStyleSheet(TRANSPARENT_BG);
    QVBoxLayout *detailsLayout = new QVBoxLayout(m_detailsWidget);
    detailsLayout->setContentsMargins(0, 0, 0, 0);
    detailsLayout->setSpacing(6);
    mainLayout->addWidget(m_detailsWidget);
    
    QWidget *imageRow = new QWidget();
    imageRow->setStyleSheet(TRANSPARENT_BG);
    QHBoxLayout *imageLayout = new QHBoxLayout(imageRow);
    imageLayout->setContentsMargins(0, 8, 0, 0);
    imageLayout->setSpacing(12);
    imageLayout->setAlignment(Qt::AlignTop);
    
    QWidget *imageContainer = new QWidget();
    imageContainer->setStyleSheet(TRANSPARENT_BG);
    imageContainer->setFixedWidth(96);
    QVBoxLayout *imageContainerLayout = new QVBoxLayout(imageContainer);
    imageContainerLayout->setContentsMargins(0, 0, 0, 0);
    imageContainerLayout->setSpacing(4);
    
    m_imageLabel = new QLabel();
    m_imageLabel->setFixedSize(96, 140);
    m_imageLabel->setStyleSheet(EditorStyles::bibleImagePlaceholderStyle());
    m_imageLabel->setAlignment(Qt::AlignCenter);
    m_imageLabel->setCursor(Qt::ArrowCursor);
    m_imageLabel->installEventFilter(this);
    QString noImageText = QString::fromUtf8("\u65e0\u56fe");
    m_imageLabel->setText(noImageText);
    imageContainerLayout->addWidget(m_imageLabel);
    
    QString deleteImageText = QString::fromUtf8("\u5220\u9664\u56fe\u7247");
    QPushButton *deleteImageBtn = new QPushButton(deleteImageText);
    deleteImageBtn->setFixedHeight(24);
    deleteImageBtn->setStyleSheet(EditorStyles::bibleDeleteButtonStyle());
    deleteImageBtn->setCursor(Qt::PointingHandCursor);
    connect(deleteImageBtn, &QPushButton::clicked, [this]() {
        emit deleteImageClicked(m_name, m_bibleType);
    });
    imageContainerLayout->addWidget(deleteImageBtn);
    
    imageLayout->addWidget(imageContainer);
    
    QWidget *uploadContainer = new QWidget();
    uploadContainer->setStyleSheet(TRANSPARENT_BG);
    uploadContainer->setFixedWidth(96);
    QVBoxLayout *uploadContainerLayout = new QVBoxLayout(uploadContainer);
    uploadContainerLayout->setContentsMargins(0, 0, 0, 0);
    uploadContainerLayout->setSpacing(4);
    
    QString uploadText = QString::fromUtf8("\u4e0a\u4f20\u65b0\u56fe");
    QPushButton *uploadBtn = new QPushButton(uploadText);
    uploadBtn->setFixedSize(96, 140);
    uploadBtn->setStyleSheet(EditorStyles::bibleUploadButtonStyle());
    uploadBtn->setCursor(Qt::PointingHandCursor);
    connect(uploadBtn, &QPushButton::clicked, [this]() {
        emit uploadClicked(m_name, m_bibleType);
    });
    uploadContainerLayout->addWidget(uploadBtn);
    
    QWidget *spacer = new QWidget();
    spacer->setFixedHeight(24);
    spacer->setStyleSheet(TRANSPARENT_BG);
    uploadContainerLayout->addWidget(spacer);
    
    imageLayout->addWidget(uploadContainer);
    imageLayout->addStretch();
    mainLayout->addWidget(imageRow);
}

void BibleItem::createEditorCard()
{
    if (m_bibleType == BibleType::Character) {
        createCharacterEditorCard();
    } else {
        createSceneEditorCard();
    }
}

QWidget* BibleItem::createEditorOverlay()
{
    QWidget *topLevel = window();
    if (!topLevel) return nullptr;
    
    QWidget *overlay = new QWidget(topLevel);
    overlay->setStyleSheet(EditorStyles::bibleEditorOverlayStyle());
    overlay->installEventFilter(this);
    overlay->hide();
    
    return overlay;
}

QFrame* BibleItem::createEditorCardBase(const QString &title, int height)
{
    Q_UNUSED(title);
    
    QFrame *card = new QFrame(m_overlayWidget);
    card->setObjectName("editorCard");
    card->setStyleSheet(EditorStyles::cardStyle());
    card->setFixedSize(480, height);
    
    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(card);
    shadow->setBlurRadius(30);
    shadow->setColor(QColor(0, 0, 0, 60));
    shadow->setOffset(0, 8);
    card->setGraphicsEffect(shadow);
    
    return card;
}

QWidget* BibleItem::createInputField(const QString &label, QLineEdit *&edit, const QString &placeholder)
{
    QWidget *row = new QWidget();
    row->setStyleSheet(TRANSPARENT_BG);
    QVBoxLayout *layout = new QVBoxLayout(row);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);
    
    QLabel *labelWidget = new QLabel(label);
    labelWidget->setStyleSheet(EditorStyles::bibleInputFieldLabelStyle());
    layout->addWidget(labelWidget);
    
    edit = new QLineEdit();
    edit->setPlaceholderText(placeholder);
    edit->setStyleSheet(EditorStyles::inputStyle());
    edit->setMinimumHeight(40);
    layout->addWidget(edit);
    
    return row;
}

QWidget* BibleItem::createInputField(const QString &label, QTextEdit *&edit, const QString &placeholder, int height)
{
    QWidget *row = new QWidget();
    row->setStyleSheet(TRANSPARENT_BG);
    QVBoxLayout *layout = new QVBoxLayout(row);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);
    
    QLabel *labelWidget = new QLabel(label);
    labelWidget->setStyleSheet(EditorStyles::bibleInputFieldLabelStyle());
    layout->addWidget(labelWidget);
    
    edit = new QTextEdit();
    edit->setPlaceholderText(placeholder);
    edit->setStyleSheet(EditorStyles::textEditStyle());
    edit->setMinimumHeight(height);
    edit->setMaximumHeight(height);
    layout->addWidget(edit);
    
    return row;
}

QWidget* BibleItem::createButtonRow()
{
    QWidget *btnRow = new QWidget();
    btnRow->setStyleSheet(TRANSPARENT_BG);
    btnRow->setFixedHeight(60);
    
    QHBoxLayout *btnLayout = new QHBoxLayout(btnRow);
    btnLayout->setContentsMargins(24, 10, 24, 10);
    btnLayout->setSpacing(12);
    
    btnLayout->addStretch();
    
    QString cancelText = QString::fromUtf8("\u53d6\u6d88");
    m_cancelBtn = new QPushButton(cancelText);
    m_cancelBtn->setStyleSheet(EditorStyles::cancelButtonStyle());
    m_cancelBtn->setCursor(Qt::PointingHandCursor);
    m_cancelBtn->setMinimumHeight(40);
    connect(m_cancelBtn, &QPushButton::clicked, this, &BibleItem::onCancelClicked);
    btnLayout->addWidget(m_cancelBtn);
    
    QString saveText = QString::fromUtf8("\u4fdd\u5b58");
    m_saveBtn = new QPushButton(saveText);
    m_saveBtn->setStyleSheet(EditorStyles::saveButtonStyle());
    m_saveBtn->setCursor(Qt::PointingHandCursor);
    m_saveBtn->setMinimumHeight(40);
    connect(m_saveBtn, &QPushButton::clicked, this, &BibleItem::onSaveClicked);
    btnLayout->addWidget(m_saveBtn);
    
    return btnRow;
}

QWidget* BibleItem::createEditorTitleRow(const QString &title)
{
    QWidget *titleRow = new QWidget();
    titleRow->setStyleSheet(TRANSPARENT_BG);
    QHBoxLayout *titleLayout = new QHBoxLayout(titleRow);
    titleLayout->setContentsMargins(0, 0, 0, 0);
    
    QLabel *titleLabel = new QLabel(title);
    titleLabel->setStyleSheet(EditorStyles::bibleEditorTitleStyle());
    
    QString closeText = QString::fromUtf8("\u00d7");
    QPushButton *closeBtn = new QPushButton(closeText);
    closeBtn->setFixedSize(28, 28);
    closeBtn->setStyleSheet(EditorStyles::closeButtonStyle());
    closeBtn->setCursor(Qt::PointingHandCursor);
    connect(closeBtn, &QPushButton::clicked, this, &BibleItem::hideEditorCard);
    
    titleLayout->addWidget(titleLabel);
    titleLayout->addStretch();
    titleLayout->addWidget(closeBtn);
    
    return titleRow;
}

QFrame* BibleItem::createSeparator()
{
    QFrame *separator = new QFrame();
    separator->setFrameShape(QFrame::HLine);
    separator->setStyleSheet(SEPARATOR_STYLE);
    return separator;
}

QScrollArea* BibleItem::createEditorScrollArea(QWidget *&contentWidget)
{
    QScrollArea *scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setStyleSheet(EditorStyles::scrollAreaStyle());
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    
    contentWidget = new QWidget();
    contentWidget->setStyleSheet(TRANSPARENT_BG);
    
    return scrollArea;
}

void BibleItem::createCharacterEditorCard()
{
    m_overlayWidget = createEditorOverlay();
    if (!m_overlayWidget) return;
    
    QString charEditorTitle = QString::fromUtf8("\u7f16\u8f91\u89d2\u8272\u8bbe\u5b9a");
    m_editorCard = createEditorCardBase(charEditorTitle, 620);
    
    QVBoxLayout *cardMainLayout = new QVBoxLayout(m_editorCard);
    cardMainLayout->setContentsMargins(0, 0, 0, 0);
    cardMainLayout->setSpacing(0);
    
    QWidget *scrollContent;
    QScrollArea *scrollArea = createEditorScrollArea(scrollContent);
    QVBoxLayout *cardLayout = new QVBoxLayout(scrollContent);
    cardLayout->setContentsMargins(24, 20, 24, 20);
    cardLayout->setSpacing(16);
    
    cardLayout->addWidget(createEditorTitleRow(charEditorTitle));
    
    QWidget *row1 = new QWidget();
    row1->setStyleSheet(TRANSPARENT_BG);
    QHBoxLayout *row1Layout = new QHBoxLayout(row1);
    row1Layout->setContentsMargins(0, 0, 0, 0);
    row1Layout->setSpacing(12);
    
    QWidget *genderGroup = new QWidget();
    genderGroup->setStyleSheet(TRANSPARENT_BG);
    QVBoxLayout *genderLayout = new QVBoxLayout(genderGroup);
    genderLayout->setContentsMargins(0, 0, 0, 0);
    genderLayout->setSpacing(6);
    QString genderLabel = QString::fromUtf8("\u6027\u522b");
    QLabel *genderLabelWidget = new QLabel(genderLabel);
    genderLabelWidget->setStyleSheet(EditorStyles::bibleInputFieldLabelStyle());
    m_genderCombo = new QComboBox();
    m_genderCombo->addItems({"female", "male"});
    m_genderCombo->setStyleSheet(EditorStyles::comboBoxStyle());
    m_genderCombo->setMinimumHeight(40);
    genderLayout->addWidget(genderLabelWidget);
    genderLayout->addWidget(m_genderCombo);
    
    QWidget *ageGroup = new QWidget();
    ageGroup->setStyleSheet(TRANSPARENT_BG);
    QVBoxLayout *ageLayout = new QVBoxLayout(ageGroup);
    ageLayout->setContentsMargins(0, 0, 0, 0);
    ageLayout->setSpacing(6);
    QString ageLabel = QString::fromUtf8("\u5e74\u9f84");
    QLabel *ageLabelWidget = new QLabel(ageLabel);
    ageLabelWidget->setStyleSheet(EditorStyles::bibleInputFieldLabelStyle());
    m_ageSpin = new QSpinBox();
    m_ageSpin->setRange(0, 200);
    m_ageSpin->setValue(17);
    m_ageSpin->setStyleSheet(EditorStyles::spinBoxStyle());
    m_ageSpin->setMinimumHeight(40);
    ageLayout->addWidget(ageLabelWidget);
    ageLayout->addWidget(m_ageSpin);
    
    QString eyeColorLabel = QString::fromUtf8("\u773c\u775b\u989c\u8272");
    QString eyeColorPlaceholder = QString::fromUtf8("\u84dd\u8272");
    QString bodyTypeLabel = QString::fromUtf8("\u8eab\u6750");
    QString bodyTypePlaceholder = QString::fromUtf8("\u9ad8\u6311\u82d7\u6761");
    QString hairColorLabel = QString::fromUtf8("\u53d1\u8272");
    QString hairColorPlaceholder = QString::fromUtf8("\u9ed1\u8272\u957f\u53d1");
    QString hairStyleLabel = QString::fromUtf8("\u53d1\u578b");
    QString hairStylePlaceholder = QString::fromUtf8("\u76f4\u53d1\u6377\u5f84");
    QString clothingLabel = QString::fromUtf8("\u670d\u9970");
    QString clothingPlaceholder = QString::fromUtf8("\u767d\u8272\u886c\u886b\u9ed1\u8272\u77ed\u88d9");
    QString featuresLabel = QString::fromUtf8("\u7279\u5f81\u63cf\u8ff0");
    QString featuresPlaceholder = QString::fromUtf8("\u5e26\u773c\u955c\u3001\u6709\u7b11\u7a9d");
    QString personalityLabel = QString::fromUtf8("\u6027\u683c\u7279\u70b9");
    QString personalityPlaceholder = QString::fromUtf8("\u6e29\u67d4\u5584\u826f");
    
    row1Layout->addWidget(genderGroup, 1);
    row1Layout->addWidget(ageGroup, 1);
    row1Layout->addWidget(createInputField(eyeColorLabel, m_eyeColorEdit, eyeColorPlaceholder), 1);
    cardLayout->addWidget(row1);
    
    cardLayout->addWidget(createInputField(bodyTypeLabel, m_bodyTypeEdit, bodyTypePlaceholder));
    
    QWidget *hairRow = new QWidget();
    hairRow->setStyleSheet(TRANSPARENT_BG);
    QHBoxLayout *hairLayout = new QHBoxLayout(hairRow);
    hairLayout->setContentsMargins(0, 0, 0, 0);
    hairLayout->setSpacing(12);
    hairLayout->addWidget(createInputField(hairColorLabel, m_hairColorEdit, hairColorPlaceholder), 1);
    hairLayout->addWidget(createInputField(hairStyleLabel, m_hairStyleEdit, hairStylePlaceholder), 1);
    cardLayout->addWidget(hairRow);
    
    cardLayout->addWidget(createInputField(clothingLabel, m_clothingEdit, clothingPlaceholder));
    cardLayout->addWidget(createInputField(featuresLabel, m_featuresEdit, featuresPlaceholder));
    cardLayout->addWidget(createInputField(personalityLabel, m_personalityEdit, personalityPlaceholder));
    
    cardLayout->addStretch();
    
    scrollArea->setWidget(scrollContent);
    cardMainLayout->addWidget(scrollArea);
    cardMainLayout->addWidget(createButtonRow());
}

void BibleItem::createSceneEditorCard()
{
    m_overlayWidget = createEditorOverlay();
    if (!m_overlayWidget) return;
    
    QString sceneEditorTitle = QString::fromUtf8("\u7f16\u8f91\u573a\u666f\u8bbe\u5b9a");
    m_editorCard = createEditorCardBase(sceneEditorTitle, 580);
    
    QVBoxLayout *cardMainLayout = new QVBoxLayout(m_editorCard);
    cardMainLayout->setContentsMargins(0, 0, 0, 0);
    cardMainLayout->setSpacing(0);
    
    QWidget *scrollContent;
    QScrollArea *scrollArea = createEditorScrollArea(scrollContent);
    QVBoxLayout *cardLayout = new QVBoxLayout(scrollContent);
    cardLayout->setContentsMargins(24, 20, 24, 20);
    cardLayout->setSpacing(16);
    
    cardLayout->addWidget(createEditorTitleRow(sceneEditorTitle));
    
    QString sceneNameLabel = QString::fromUtf8("\u573a\u666f\u540d\u79f0");
    QString sceneNamePlaceholder = QString::fromUtf8("\u573a\u666f\u540d\u79f0");
    QString sceneDescLabel = QString::fromUtf8("\u573a\u666f\u63cf\u8ff0");
    QString sceneDescPlaceholder = QString::fromUtf8("\u8be6\u7ec6\u63cf\u8ff0\u573a\u666f\u7684\u65f6\u95f4\u5730\u70b9");
    QString buildingLabel = QString::fromUtf8("\u5efa\u7b51\u7269");
    QString buildingPlaceholder = QString::fromUtf8("\u5efa\u7b51\u7269\u7684\u5916\u89c2\u7279\u5f81");
    QString colorLabel = QString::fromUtf8("\u8272\u8c03");
    QString colorPlaceholder = QString::fromUtf8("\u573a\u666f\u7684\u4e3b\u8981\u8272\u8c03");
    QString landmarkLabel = QString::fromUtf8("\u5730\u6807");
    QString landmarkPlaceholder = QString::fromUtf8("\u91cd\u8981\u5730\u6807");
    QString layoutLabel = QString::fromUtf8("\u5e03\u5c40");
    QString layoutPlaceholder = QString::fromUtf8("\u7a7a\u95f4\u5e03\u5c40");
    QString atmosphereLabel = QString::fromUtf8("\u6c1b\u56f4");
    QString atmospherePlaceholder = QString::fromUtf8("\u573a\u666f\u6c1b\u56f4");
    
    QWidget *nameRow = createInputField(sceneNameLabel, m_sceneNameEdit, sceneNamePlaceholder);
    m_sceneNameEdit->setText(m_name);
    cardLayout->addWidget(nameRow);
    
    cardLayout->addWidget(createInputField(sceneDescLabel, m_sceneDescEdit, sceneDescPlaceholder, 80));
    
    cardLayout->addWidget(createInputField(buildingLabel, m_buildingEdit, buildingPlaceholder));
    cardLayout->addWidget(createInputField(colorLabel, m_colorEdit, colorPlaceholder));
    cardLayout->addWidget(createInputField(landmarkLabel, m_landmarkEdit, landmarkPlaceholder));
    cardLayout->addWidget(createInputField(layoutLabel, m_layoutEdit, layoutPlaceholder));
    cardLayout->addWidget(createInputField(atmosphereLabel, m_atmosphereEdit, atmospherePlaceholder));
    
    cardLayout->addStretch();
    
    scrollArea->setWidget(scrollContent);
    cardMainLayout->addWidget(scrollArea);
    cardMainLayout->addWidget(createButtonRow());
}

void BibleItem::setImage(const QString &imageUrl)
{
    m_currentImagePath = imageUrl;
    
    if (imageUrl.isEmpty()) {
        QString noImg = QString::fromUtf8("\u6682\u65e0\u56fe");
        m_imageLabel->setStyleSheet(EditorStyles::bibleImagePlaceholderStyle());
        m_imageLabel->setText(noImg);
        m_imageLabel->setCursor(Qt::ArrowCursor);
        return;
    }
    
    QString id = QString("bible_%1_%2").arg(m_name).arg(QFileInfo(imageUrl).fileName());
    constexpr int BIBLE_IMAGE_WIDTH = 96;
    constexpr int BIBLE_IMAGE_HEIGHT = 140;
    QSize targetSize(BIBLE_IMAGE_WIDTH, BIBLE_IMAGE_HEIGHT);
    QString cacheKey = AsyncImageLoader::makeCacheKey(id, targetSize);
    
    QPixmap cachedPixmap;
    if (AsyncImageLoader::instance()->findCached(cacheKey, &cachedPixmap)) {
        m_imageLabel->setPixmap(cachedPixmap);
        m_imageLabel->setStyleSheet(EditorStyles::bibleImageHasImageStyle());
        m_imageLabel->setText("");
        m_imageLabel->setCursor(Qt::PointingHandCursor);
        return;
    }
    
    QString loadingText = QString::fromUtf8("\u52a0\u8f7d\u4e2d...");
    m_imageLabel->setStyleSheet(EditorStyles::bibleImagePlaceholderStyle());
    m_imageLabel->setText(loadingText);
    m_imageLabel->setCursor(Qt::ArrowCursor);
    
    m_loadingImageId = id;
    
    disconnect(AsyncImageLoader::instance(), nullptr, this, nullptr);
    connect(AsyncImageLoader::instance(), &AsyncImageLoader::imageLoaded,
            this, &BibleItem::onAsyncImageLoaded);
    
    AsyncImageLoader::instance()->loadAsync(id, imageUrl, targetSize);
}

void BibleItem::onAsyncImageLoaded(const QString& id, const QString& cacheKey, const QPixmap& pixmap)
{
    if (id != m_loadingImageId) {
        return;
    }
    
    if (!pixmap.isNull()) {
        AsyncImageLoader::instance()->insertCache(cacheKey, pixmap);
        m_imageLabel->setPixmap(pixmap);
        m_imageLabel->setStyleSheet(EditorStyles::bibleImageHasImageStyle());
        m_imageLabel->setText("");
        m_imageLabel->setCursor(Qt::PointingHandCursor);
    } else {
        QString loadFailed = QString::fromUtf8("\u52a0\u8f7d\u5931\u8d25");
        m_imageLabel->setStyleSheet(EditorStyles::bibleImagePlaceholderStyle());
        m_imageLabel->setText(loadFailed);
        m_imageLabel->setCursor(Qt::ArrowCursor);
    }
}

void BibleItem::setDetails(const QStringList &details)
{
    m_details = details;
    if (!m_detailsWidget) return;
    
    QVBoxLayout *layout = qobject_cast<QVBoxLayout*>(m_detailsWidget->layout());
    if (!layout) return;
    
    QList<QWidget*> widgetsToDelete;
    QLayoutItem *item;
    while ((item = layout->takeAt(0)) != nullptr) {
        if (item->widget()) {
            widgetsToDelete << item->widget();
        }
        delete item;
    }
    for (QWidget *w : widgetsToDelete) {
        w->deleteLater();
    }
    
    for (const QString &detail : details) {
        QLabel *detailLabel = new QLabel(detail);
        detailLabel->setStyleSheet(EditorStyles::bibleDetailLabelStyle());
        detailLabel->setWordWrap(true);
        layout->addWidget(detailLabel);
    }
}

void BibleItem::expandEditor()
{
    if (!m_overlayWidget) {
        createEditorCard();
    }
    populateEditorData();
    showEditorCard();
}

void BibleItem::collapseEditor()
{
    hideEditorCard();
}

void BibleItem::showEditorCard()
{
    if (!m_overlayWidget || !m_editorCard) return;
    
    QWidget *parent = m_overlayWidget->parentWidget();
    if (parent) {
        m_overlayWidget->setGeometry(parent->rect());
    }
    
    m_overlayWidget->show();
    m_overlayWidget->raise();
    
    updateEditorCardPosition();
    
    m_isExpanded = true;
    QString collapseText = QString::fromUtf8("\u6536\u8d77");
    m_editBtn->setText(collapseText);
}

void BibleItem::hideEditorCard()
{
    if (m_overlayWidget) {
        m_overlayWidget->hide();
    }
    m_isExpanded = false;
    QString editText = QString::fromUtf8("\u7f16\u8f91");
    m_editBtn->setText(editText);
}

void BibleItem::updateEditorCardPosition()
{
    if (!m_overlayWidget || !m_editorCard) return;
    
    QRect overlayRect = m_overlayWidget->rect();
    
    int cardWidth = m_editorCard->width();
    int cardHeight = m_editorCard->height();
    
    int x = (overlayRect.width() - cardWidth) / 2;
    int y = (overlayRect.height() - cardHeight) / 2;
    
    m_editorCard->move(x, y);
}

bool BibleItem::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress) {
        if (watched == m_overlayWidget) {
            hideEditorCard();
            return true;
        }
        if (watched == m_imageLabel && !m_currentImagePath.isEmpty()) {
            emit imageClicked(m_currentImagePath);
            return true;
        }
    }
    return QFrame::eventFilter(watched, event);
}

void BibleItem::onEditBtnClicked()
{
    if (m_isExpanded) {
        collapseEditor();
    } else {
        expandEditor();
    }
    emit editClicked(m_name);
}

void BibleItem::onSaveClicked()
{
    if (m_bibleType == BibleType::Character) {
        saveCharacterData();
    } else {
        saveSceneData();
    }
    
    hideEditorCard();
}

void BibleItem::populateEditorData()
{
    if (m_bibleType == BibleType::Character) {
        populateCharacterEditorData();
    } else {
        populateSceneEditorData();
    }
}

void BibleItem::populateCharacterEditorData()
{
    if (m_details.isEmpty()) return;
    
    for (const QString &detail : m_details) {
        if (detail.contains(QString::fromUtf8("\u5916\u89c2"))) {
            
            // 提取性别
            QRegularExpression genderRe(QString::fromUtf8("\u5916\u89c2\uff1a(\u7537|\u5973)"));
            QRegularExpressionMatch match = genderRe.match(detail);
            if (match.hasMatch()) {
                QString gender = match.captured(1);
                if (gender == QString::fromUtf8("\u7537")) gender = "male";
                else if (gender == QString::fromUtf8("\u5973")) gender = "female";
                int index = m_genderCombo->findText(gender);
                if (index >= 0) m_genderCombo->setCurrentIndex(index);
            }
            
            // 提取年龄
            QRegularExpression ageRe(QString::fromUtf8("(\\d+)\u5c81"));
            match = ageRe.match(detail);
            if (match.hasMatch()) {
                m_ageSpin->setValue(match.captured(1).toInt());
            }
            
            // 提取发色和瞳色
            QRegularExpression hairColorRe(QString::fromUtf8("\u53d1\u8272\uff1a(\\S+)"));
            match = hairColorRe.match(detail);
            if (match.hasMatch()) {
                m_hairColorEdit->setText(match.captured(1));
            }
            
            QRegularExpression eyeColorRe(QString::fromUtf8("\u77b3\u8272\uff1a(\\S+)"));
            match = eyeColorRe.match(detail);
            if (match.hasMatch()) {
                m_eyeColorEdit->setText(match.captured(1));
            }
        }
        else {
            // 其他字段使用通用方法填充
            populateTextField(detail, QString::fromUtf8("\u4f53\u578b"), m_bodyTypeEdit);
            populateTextField(detail, QString::fromUtf8("\u53d1\u578b"), m_hairStyleEdit);
            populateTextField(detail, QString::fromUtf8("\u670d\u9970"), m_clothingEdit);
            populateTextField(detail, QString::fromUtf8("\u7279\u5f81"), m_featuresEdit);
            populateTextField(detail, QString::fromUtf8("\u6027\u683c"), m_personalityEdit);
        }
    }
}

void BibleItem::populateSceneEditorData()
{
    if (m_details.isEmpty()) return;
    
    m_sceneNameEdit->setText(m_name);
    
    for (const QString &detail : m_details) {
        populateTextField(detail, QString::fromUtf8("\u573a\u666f\u63cf\u8ff0"), m_sceneDescEdit);
        populateTextField(detail, QString::fromUtf8("\u5efa\u7b51"), m_buildingEdit);
        populateTextField(detail, QString::fromUtf8("\u8272\u8c03"), m_colorEdit);
        populateTextField(detail, QString::fromUtf8("\u5730\u6807"), m_landmarkEdit);
        populateTextField(detail, QString::fromUtf8("\u5e03\u5c40"), m_layoutEdit);
        populateTextField(detail, QString::fromUtf8("\u6c1b\u56f4"), m_atmosphereEdit);
    }
}

QString BibleItem::extractValue(const QString &detail, const QString &key)
{
    QString prefix = key + QString::fromUtf8("\uff1a");
    if (detail.startsWith(prefix)) {
        return detail.mid(prefix.length()).trimmed();
    }
    return QString();
}

void BibleItem::populateTextField(const QString &detail, const QString &key, QLineEdit *edit)
{
    if (!edit) return;
    QString value = extractValue(detail, key);
    if (!value.isEmpty()) {
        edit->setText(value);
    }
}

void BibleItem::populateTextField(const QString &detail, const QString &key, QTextEdit *edit)
{
    if (!edit) return;
    QString value = extractValue(detail, key);
    if (!value.isEmpty()) {
        edit->setPlainText(value);
    }
}

void BibleItem::saveCharacterData()
{
    QStringList newDetails;
    
    QString gender = m_genderCombo->currentText();
    int age = m_ageSpin->value();
    QString hairColor = m_hairColorEdit->text().trimmed();
    QString eyeColor = m_eyeColorEdit->text().trimmed();
    
    QString appearanceLine = QString::fromUtf8("\u5916\u89c2\uff1a%1 \u00b7 %2\u5c81")
        .arg(gender).arg(age);
    if (!hairColor.isEmpty()) {
        appearanceLine += QString::fromUtf8(" \u00b7 \u53d1\u8272\uff1a%1").arg(hairColor);
    }
    if (!eyeColor.isEmpty()) {
        appearanceLine += QString::fromUtf8(" \u00b7 \u77b3\u8272\uff1a%1").arg(eyeColor);
    }
    newDetails.append(appearanceLine);
    
    QString bodyType = m_bodyTypeEdit->text().trimmed();
    if (!bodyType.isEmpty()) {
        newDetails.append(QString::fromUtf8("\u4f53\u578b\uff1a%1").arg(bodyType));
    }
    
    QString hairStyle = m_hairStyleEdit->text().trimmed();
    if (!hairStyle.isEmpty()) {
        newDetails.append(QString::fromUtf8("\u53d1\u578b\uff1a%1").arg(hairStyle));
    }
    
    QString clothing = m_clothingEdit->text().trimmed();
    if (!clothing.isEmpty()) {
        newDetails.append(QString::fromUtf8("\u670d\u9970\uff1a%1").arg(clothing));
    }
    
    QString features = m_featuresEdit->text().trimmed();
    if (!features.isEmpty()) {
        newDetails.append(QString::fromUtf8("\u7279\u5f81\uff1a%1").arg(features));
    }
    
    QString personality = m_personalityEdit->text().trimmed();
    if (!personality.isEmpty()) {
        newDetails.append(QString::fromUtf8("\u6027\u683c\uff1a%1").arg(personality));
    }
    
    m_details = newDetails;
    setDetails(newDetails);
    
    emit dataChanged(m_name, newDetails);
}

void BibleItem::saveSceneData()
{
    QString newName = m_sceneNameEdit->text().trimmed();
    if (!newName.isEmpty()) {
        m_name = newName;
        m_nameLabel->setText(QString("<b>%1</b>").arg(m_name));
    }
    
    QStringList newDetails;
    
    QString desc = m_sceneDescEdit->toPlainText().trimmed();
    if (!desc.isEmpty()) {
        newDetails.append(desc);
    }
    
    QString building = m_buildingEdit->text().trimmed();
    if (!building.isEmpty()) {
        QString buildingDetail = QString::fromUtf8("\u5efa\u7b51: %1").arg(building);
        newDetails.append(buildingDetail);
    }
    
    QString color = m_colorEdit->text().trimmed();
    if (!color.isEmpty()) {
        QString colorDetail = QString::fromUtf8("\u8272\u8c03: %1").arg(color);
        newDetails.append(colorDetail);
    }
    
    QString landmark = m_landmarkEdit->text().trimmed();
    if (!landmark.isEmpty()) {
        QString landmarkDetail = QString::fromUtf8("\u5730\u6807: %1").arg(landmark);
        newDetails.append(landmarkDetail);
    }
    
    QString layout = m_layoutEdit->text().trimmed();
    if (!layout.isEmpty()) {
        QString layoutDetail = QString::fromUtf8("\u5e03\u5c40: %1").arg(layout);
        newDetails.append(layoutDetail);
    }
    
    QString atmosphere = m_atmosphereEdit->text().trimmed();
    if (!atmosphere.isEmpty()) {
        QString atmosphereDetail = QString::fromUtf8("\u6c1b\u56f4: %1").arg(atmosphere);
        newDetails.append(atmosphereDetail);
    }
    
    m_details = newDetails;
    setDetails(newDetails);
    
    emit dataChanged(m_name, newDetails);
}

void BibleItem::onCancelClicked()
{
    hideEditorCard();
}
