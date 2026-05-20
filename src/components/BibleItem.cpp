#include "components/BibleItem.h"
#include "components/EditorStyles.h"
#include "utils/AnalysisFieldUtils.h"
#include "utils/BibleUtils.h"
#include "utils/AsyncImageLoader.h"
#include "utils/Logger.h"
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
    const QString MANUAL_SOURCE = QStringLiteral("manual");
    
    // Content boundary helpers
    // direction: 0=top, 1=bottom, 2=left, 3=right
    int findContentBoundary(const QImage& img, int threshold, int direction)
    {
        int w = img.width();
        int h = img.height();
        
        auto isContentPixel = [threshold](QRgb pixel) {
            int r = qRed(pixel), g = qGreen(pixel), b = qBlue(pixel), a = qAlpha(pixel);
            return a > 10 && (r < threshold || g < threshold || b < threshold);
        };
        
        switch (direction) {
        case 0: // top
            for (int y = 0; y < h; ++y) {
                for (int x = 0; x < w; ++x) {
                    if (isContentPixel(img.pixel(x, y))) {
                        return y;
                    }
                }
            }
            break;
        case 1: // bottom
            for (int y = h - 1; y >= 0; --y) {
                for (int x = 0; x < w; ++x) {
                    if (isContentPixel(img.pixel(x, y))) {
                        return y;
                    }
                }
            }
            break;
        case 2: // left
            for (int x = 0; x < w; ++x) {
                for (int y = 0; y < h; ++y) {
                    if (isContentPixel(img.pixel(x, y))) {
                        return x;
                    }
                }
            }
            break;
        case 3: // right
            for (int x = w - 1; x >= 0; --x) {
                for (int y = 0; y < h; ++y) {
                    if (isContentPixel(img.pixel(x, y))) {
                        return x;
                    }
                }
            }
            break;
        default:
            break;
        }
        return -1;
    }

    void setComboText(QComboBox* combo, const QString& value)
    {
        if (!combo || value.isEmpty()) {
            return;
        }

        const int index = combo->findText(value);
        if (index >= 0) {
            combo->setCurrentIndex(index);
        }
    }

    void setModeComboText(ModeComboBox* combo, const QString& value)
    {
        if (!combo || value.isEmpty()) {
            return;
        }

        const int index = combo->findText(value);
        if (index >= 0) {
            combo->setCurrentIndex(index);
        }
    }

    void setLineEditText(QLineEdit* edit, const QString& value)
    {
        if (edit && !value.isEmpty()) {
            edit->setText(value);
        }
    }

    void setTextEditText(QTextEdit* edit, const QString& value)
    {
        if (edit && !value.isEmpty()) {
            edit->setPlainText(value);
        }
    }

    int parsePositiveInt(const QString& value)
    {
        const QRegularExpression digitsRe(QStringLiteral("(\\d+)"));
        const QRegularExpressionMatch match = digitsRe.match(value);
        return match.hasMatch() ? match.captured(1).toInt() : 0;
    }

    void markCharacterAppearanceManual(CharacterAppearance& app, const CharacterAppearance& original, bool markAllFields)
    {
        AnalysisFieldUtils::markFieldSources(app.fieldSources, {
            {"gender", markAllFields || AnalysisFieldUtils::valueChanged(app.gender, original.gender)},
            {"age", markAllFields || AnalysisFieldUtils::valueChanged(app.age, original.age)},
            {"hairColor", markAllFields || AnalysisFieldUtils::valueChanged(app.hairColor, original.hairColor)},
            {"hairStyle", markAllFields || AnalysisFieldUtils::valueChanged(app.hairStyle, original.hairStyle)},
            {"eyeColor", markAllFields || AnalysisFieldUtils::valueChanged(app.eyeColor, original.eyeColor)},
            {"build", markAllFields || AnalysisFieldUtils::valueChanged(app.build, original.build)},
            {"clothing", markAllFields || AnalysisFieldUtils::valueChanged(app.clothing, original.clothing)},
            {"accessories", markAllFields || AnalysisFieldUtils::valueChanged(app.accessories, original.accessories)},
            {"distinctiveFeatures", markAllFields || AnalysisFieldUtils::valueChanged(app.distinctiveFeatures, original.distinctiveFeatures)}
        }, MANUAL_SOURCE);
    }

    void markSceneDetailsManual(SceneDetails& details, const SceneDetails& original, bool markAllFields)
    {
        AnalysisFieldUtils::markFieldSources(details.fieldSources, {
            {"description", markAllFields || AnalysisFieldUtils::valueChanged(details.description, original.description)},
            {"building", markAllFields || AnalysisFieldUtils::valueChanged(details.building, original.building)},
            {"color", markAllFields || AnalysisFieldUtils::valueChanged(details.color, original.color)},
            {"landmark", markAllFields || AnalysisFieldUtils::valueChanged(details.landmark, original.landmark)},
            {"layout", markAllFields || AnalysisFieldUtils::valueChanged(details.layout, original.layout)},
            {"atmosphere", markAllFields || AnalysisFieldUtils::valueChanged(details.atmosphere, original.atmosphere)},
            {"type", markAllFields || AnalysisFieldUtils::valueChanged(details.type, original.type)},
            {"setting", markAllFields || AnalysisFieldUtils::valueChanged(details.setting, original.setting)},
            {"timeOfDay", markAllFields || AnalysisFieldUtils::valueChanged(details.timeOfDay, original.timeOfDay)},
            {"weather", markAllFields || AnalysisFieldUtils::valueChanged(details.weather, original.weather)},
            {"narrativeRole", markAllFields || AnalysisFieldUtils::valueChanged(details.narrativeRole, original.narrativeRole)}
        }, MANUAL_SOURCE);
    }

    QPixmap scalePixmapToFillCenterCrop(const QPixmap& pixmap, const QSize& size)
    {
        QPixmap scaled = pixmap.scaled(size, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
        const int cropX = qMax(0, (scaled.width() - size.width()) / 2);
        const int cropY = qMax(0, (scaled.height() - size.height()) / 2);
        return scaled.copy(cropX, cropY, size.width(), size.height());
    }

}
BibleItem::BibleItem(const QString &name, const QStringList &details, BibleType type, QWidget *parent)
    : QFrame(parent)
    , m_nameLabel(nullptr)
    , m_detailsWidget(nullptr)
    , m_imageLabel(nullptr)
    , m_editBtn(nullptr)
    , m_deleteBtn(nullptr)
    , m_name(name)
    , m_itemId()  // 默认不设置 itemId
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

    if (m_bibleType == BibleType::Character) {
        m_versionBtn = new QPushButton(QString::fromUtf8("版本"));
        m_versionBtn->setFixedSize(64, 32);
        m_versionBtn->setStyleSheet(R"(
            QPushButton {
                padding: 6px 16px;
                border-radius: 6px;
                border: 1px solid #4deab308;
                background: #14eab308;
                color: #854d0e;
                font-size: 13px;
            }
            QPushButton:hover {
                background: #26eab308;
                border-color: #80eab308;
            }
        )");
        m_versionBtn->setCursor(Qt::PointingHandCursor);
        connect(m_versionBtn, &QPushButton::clicked, [this]() {
            QPoint anchor = m_versionBtn->mapToGlobal(QPoint(0, m_versionBtn->height()));
            emit versionClicked(m_itemId, anchor);
        });
        headerLayout->addWidget(m_versionBtn);
    }

    QString editText = QString::fromUtf8("编辑");
    m_editBtn = new QPushButton(editText);
    m_editBtn->setFixedSize(64, 32);
    m_editBtn->setStyleSheet(EditorStyles::editButtonStyle());
    m_editBtn->setCursor(Qt::PointingHandCursor);
    connect(m_editBtn, &QPushButton::clicked, this, &BibleItem::onEditBtnClicked);
    headerLayout->addWidget(m_editBtn);

    QString deleteText = QString::fromUtf8("删除");
    m_deleteBtn = new QPushButton(deleteText);
    m_deleteBtn->setFixedSize(64, 32);
    m_deleteBtn->setStyleSheet(EditorStyles::deleteButtonStyle());
    m_deleteBtn->setCursor(Qt::PointingHandCursor);
    connect(m_deleteBtn, &QPushButton::clicked, [this]() {
        emit deleteRequested(m_itemId, m_bibleType);
    });
    headerLayout->addWidget(m_deleteBtn);
    
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
    imageContainer->setFixedWidth(128);
    imageContainer->setMinimumHeight(216);
    QVBoxLayout *imageContainerLayout = new QVBoxLayout(imageContainer);
    imageContainerLayout->setContentsMargins(0, 0, 0, 0);
    imageContainerLayout->setSpacing(4);
    
    m_imageLabel = new QLabel();
    m_imageLabel->setFixedSize(128, 180);
    m_imageLabel->setStyleSheet(EditorStyles::bibleImagePlaceholderStyle());
    m_imageLabel->setAlignment(Qt::AlignCenter);
    m_imageLabel->setCursor(Qt::ArrowCursor);
    m_imageLabel->installEventFilter(this);
    QString noImageText = QString::fromUtf8("暂无图片");
    m_imageLabel->setText(noImageText);
    imageContainerLayout->addWidget(m_imageLabel);
    
    QString deleteImageText = QString::fromUtf8("删除图片");
    QPushButton *deleteImageBtn = new QPushButton(deleteImageText);
    deleteImageBtn->setFixedHeight(24);
    deleteImageBtn->setStyleSheet(EditorStyles::bibleDeleteButtonStyle());
    deleteImageBtn->setCursor(Qt::PointingHandCursor);
    connect(deleteImageBtn, &QPushButton::clicked, [this]() {
        emit deleteImageClicked(m_itemId, m_bibleType);
    });
    
    imageContainerLayout->addWidget(deleteImageBtn);
    
    imageLayout->addWidget(imageContainer);
    
    QWidget *uploadContainer = new QWidget();
    uploadContainer->setStyleSheet(TRANSPARENT_BG);
    uploadContainer->setFixedWidth(128);
    uploadContainer->setMinimumHeight(216);
    QVBoxLayout *uploadContainerLayout = new QVBoxLayout(uploadContainer);
    uploadContainerLayout->setContentsMargins(0, 0, 0, 0);
    uploadContainerLayout->setSpacing(4);
    
    QString uploadText = QString::fromUtf8("上传图片");
    QPushButton *uploadBtn = new QPushButton(uploadText);
    uploadBtn->setFixedSize(128, 180);
    uploadBtn->setStyleSheet(EditorStyles::bibleUploadButtonStyle());
    uploadBtn->setCursor(Qt::PointingHandCursor);
    connect(uploadBtn, &QPushButton::clicked, [this]() {
        emit uploadClicked(m_itemId, m_bibleType);
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
    QWidget *row = createLabeledControl(label, nullptr);

    edit = new QLineEdit();
    edit->setPlaceholderText(placeholder);
    edit->setStyleSheet(EditorStyles::inputStyle());
    edit->setMinimumHeight(40);
    qobject_cast<QVBoxLayout*>(row->layout())->addWidget(edit);
    
    return row;
}

QWidget* BibleItem::createInputField(const QString &label, QTextEdit *&edit, const QString &placeholder, int height)
{
    QWidget *row = createLabeledControl(label, nullptr);

    edit = new QTextEdit();
    edit->setPlaceholderText(placeholder);
    edit->setStyleSheet(EditorStyles::textEditStyle());
    edit->setMinimumHeight(height);
    edit->setMaximumHeight(height);
    qobject_cast<QVBoxLayout*>(row->layout())->addWidget(edit);
    
    return row;
}

QWidget* BibleItem::createComboBoxField(const QString &label, ModeComboBox *&combo, const QStringList &items)
{
    QWidget *row = createLabeledControl(label, nullptr);

    combo = new ModeComboBox();
    combo->setFixedHeight(40);
    for (const QString &item : items) {
        combo->addItem(item);
    }
    qobject_cast<QVBoxLayout*>(row->layout())->addWidget(combo);
    
    return row;
}

QWidget* BibleItem::createLabeledControl(const QString& label, QWidget* controlWidget)
{
    QWidget *row = new QWidget();
    row->setStyleSheet(TRANSPARENT_BG);
    QVBoxLayout *layout = new QVBoxLayout(row);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);

    QLabel *labelWidget = new QLabel(label);
    labelWidget->setStyleSheet(EditorStyles::bibleInputFieldLabelStyle());
    layout->addWidget(labelWidget);

    if (controlWidget) {
        layout->addWidget(controlWidget);
    }

    return row;
}

QWidget* BibleItem::createSpinBoxField(const QString& label, QSpinBox*& spinBox, int minValue, int maxValue, int defaultValue)
{
    QWidget *row = createLabeledControl(label, nullptr);
    spinBox = new QSpinBox();
    spinBox->setRange(minValue, maxValue);
    spinBox->setValue(defaultValue);
    spinBox->setStyleSheet(EditorStyles::spinBoxStyle());
    spinBox->setMinimumHeight(40);
    qobject_cast<QVBoxLayout*>(row->layout())->addWidget(spinBox);
    return row;
}

QWidget* BibleItem::createComboBoxField(const QString& label, QComboBox*& combo, const QStringList& items)
{
    QWidget *row = createLabeledControl(label, nullptr);
    combo = new QComboBox();
    combo->setStyleSheet(EditorStyles::comboBoxStyle());
    combo->setMinimumHeight(40);
    for (const QString& item : items) {
        combo->addItem(item);
    }
    qobject_cast<QVBoxLayout*>(row->layout())->addWidget(combo);
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
    
    QString cancelText = QString::fromUtf8("取消");
    m_cancelBtn = new QPushButton(cancelText);
    m_cancelBtn->setStyleSheet(EditorStyles::cancelButtonStyle());
    m_cancelBtn->setCursor(Qt::PointingHandCursor);
    m_cancelBtn->setMinimumHeight(40);
    connect(m_cancelBtn, &QPushButton::clicked, this, &BibleItem::onCancelClicked);
    btnLayout->addWidget(m_cancelBtn);
    
    QString saveText = QString::fromUtf8("保存");
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
    
    titleLayout->addWidget(titleLabel);
    titleLayout->addStretch();
    
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

BibleItem::EditorCardContext BibleItem::initEditorCard(const QString& title, int height)
{
    EditorCardContext ctx;
    
    m_overlayWidget = createEditorOverlay();
    if (!m_overlayWidget) {
        ctx.scrollContent = nullptr;
        ctx.cardLayout = nullptr;
        ctx.scrollArea = nullptr;
        ctx.cardMainLayout = nullptr;
        return ctx;
    }
    
    m_editorCard = createEditorCardBase(title, height);
    
    ctx.cardMainLayout = new QVBoxLayout(m_editorCard);
    ctx.cardMainLayout->setContentsMargins(0, 0, 0, 0);
    ctx.cardMainLayout->setSpacing(0);
    
    ctx.scrollArea = createEditorScrollArea(ctx.scrollContent);
    ctx.cardLayout = new QVBoxLayout(ctx.scrollContent);
    ctx.cardLayout->setContentsMargins(24, 20, 24, 20);
    ctx.cardLayout->setSpacing(16);
    
    ctx.cardLayout->addWidget(createEditorTitleRow(title));
    
    return ctx;
}

void BibleItem::finishEditorCard(EditorCardContext& ctx)
{
    if (!ctx.scrollContent || !ctx.cardLayout) return;
    
    ctx.cardLayout->addStretch();
    ctx.scrollArea->setWidget(ctx.scrollContent);
    ctx.cardMainLayout->addWidget(ctx.scrollArea);
    ctx.cardMainLayout->addWidget(createButtonRow());
}

void BibleItem::createCharacterEditorCard()
{
    QString charEditorTitle = QString::fromUtf8("角色编辑");
    EditorCardContext ctx = initEditorCard(charEditorTitle, 620);
    if (!ctx.scrollContent) return;
    
    QWidget *row1 = new QWidget();
    row1->setStyleSheet(TRANSPARENT_BG);
    QHBoxLayout *row1Layout = new QHBoxLayout(row1);
    row1Layout->setContentsMargins(0, 0, 0, 0);
    row1Layout->setSpacing(12);
    QWidget *genderGroup = createComboBoxField(
        QString::fromUtf8("性别"),
        m_genderCombo,
        {QString::fromUtf8("男"), QString::fromUtf8("女")}
    );
    QWidget *ageGroup = createSpinBoxField(QString::fromUtf8("年龄"), m_ageSpin, 0, 200, 17);
    
    QString eyeColorLabel = QString::fromUtf8("瞳色");
    QString eyeColorPlaceholder = QString::fromUtf8("输入瞳色");
    QString bodyTypeLabel = QString::fromUtf8("体型");
    QString bodyTypePlaceholder = QString::fromUtf8("输入体型");
    QString hairColorLabel = QString::fromUtf8("发色");
    QString hairColorPlaceholder = QString::fromUtf8("输入发色");
    QString hairStyleLabel = QString::fromUtf8("发型");
    QString hairStylePlaceholder = QString::fromUtf8("输入发型");
    QString clothingLabel = QString::fromUtf8("服装");
    QString clothingPlaceholder = QString::fromUtf8("输入服装描述");
    QString featuresLabel = QString::fromUtf8("特征");
    QString featuresPlaceholder = QString::fromUtf8("输入特征");
    QString personalityLabel = QString::fromUtf8("性格");
    QString personalityPlaceholder = QString::fromUtf8("输入性格");
    
    row1Layout->addWidget(genderGroup, 1);
    row1Layout->addWidget(ageGroup, 1);
    row1Layout->addWidget(createInputField(eyeColorLabel, m_eyeColorEdit, eyeColorPlaceholder), 1);
    ctx.cardLayout->addWidget(row1);
    
    ctx.cardLayout->addWidget(createInputField(bodyTypeLabel, m_bodyTypeEdit, bodyTypePlaceholder));
    
    QWidget *hairRow = new QWidget();
    hairRow->setStyleSheet(TRANSPARENT_BG);
    QHBoxLayout *hairLayout = new QHBoxLayout(hairRow);
    hairLayout->setContentsMargins(0, 0, 0, 0);
    hairLayout->setSpacing(12);
    hairLayout->addWidget(createInputField(hairColorLabel, m_hairColorEdit, hairColorPlaceholder), 1);
    hairLayout->addWidget(createInputField(hairStyleLabel, m_hairStyleEdit, hairStylePlaceholder), 1);
    ctx.cardLayout->addWidget(hairRow);
    
    ctx.cardLayout->addWidget(createInputField(clothingLabel, m_clothingEdit, clothingPlaceholder));
    ctx.cardLayout->addWidget(createInputField(featuresLabel, m_featuresEdit, featuresPlaceholder));
    ctx.cardLayout->addWidget(createInputField(personalityLabel, m_personalityEdit, personalityPlaceholder));
    
    finishEditorCard(ctx);
}

void BibleItem::createSceneEditorCard()
{
    QString sceneEditorTitle = QString::fromUtf8("场景编辑");
    EditorCardContext ctx = initEditorCard(sceneEditorTitle, 720);
    if (!ctx.scrollContent) return;
    
    QString sceneNameLabel = QString::fromUtf8("场景名称");
    QString sceneNamePlaceholder = QString::fromUtf8("输入场景名称");
    QString sceneDescLabel = QString::fromUtf8("场景描述");
    QString sceneDescPlaceholder = QString::fromUtf8("输入场景描述");
    QString buildingLabel = QString::fromUtf8("建筑");
    QString buildingPlaceholder = QString::fromUtf8("输入建筑");
    QString colorLabel = QString::fromUtf8("色调");
    QString colorPlaceholder = QString::fromUtf8("输入色调");
    QString landmarkLabel = QString::fromUtf8("地标");
    QString landmarkPlaceholder = QString::fromUtf8("输入地标");
    QString layoutLabel = QString::fromUtf8("布局");
    QString layoutPlaceholder = QString::fromUtf8("输入布局");
    QString atmosphereLabel = QString::fromUtf8("氛围");
    QString atmospherePlaceholder = QString::fromUtf8("输入氛围");
    QString typeLabel = QString::fromUtf8("类型");
    QString settingLabel = QString::fromUtf8("背景设定");
    QString timeOfDayLabel = QString::fromUtf8("时间段");
    QString weatherLabel = QString::fromUtf8("天气");
    QString narrativeRoleLabel = QString::fromUtf8("叙事角色");
    QString narrativeRolePlaceholder = QString::fromUtf8("输入叙事角色");
    
    QWidget *nameRow = createInputField(sceneNameLabel, m_sceneNameEdit, sceneNamePlaceholder);
    m_sceneNameEdit->setText(m_name);
    ctx.cardLayout->addWidget(nameRow);
    
    ctx.cardLayout->addWidget(createInputField(sceneDescLabel, m_sceneDescEdit, sceneDescPlaceholder, 80));
    
    ctx.cardLayout->addWidget(createInputField(buildingLabel, m_buildingEdit, buildingPlaceholder));
    ctx.cardLayout->addWidget(createInputField(colorLabel, m_colorEdit, colorPlaceholder));
    ctx.cardLayout->addWidget(createInputField(landmarkLabel, m_landmarkEdit, landmarkPlaceholder));
    ctx.cardLayout->addWidget(createInputField(layoutLabel, m_layoutEdit, layoutPlaceholder));
    ctx.cardLayout->addWidget(createInputField(atmosphereLabel, m_atmosphereEdit, atmospherePlaceholder));
    
    ctx.cardLayout->addWidget(createComboBoxField(typeLabel, m_typeCombo, {
        QString::fromUtf8("室内"),
        QString::fromUtf8("室外"),
        QString::fromUtf8("自然"),
        QString::fromUtf8("城市"),
        QString::fromUtf8("幻想"),
        QString::fromUtf8("科幻"),
        QString::fromUtf8("其他")
    }));
    
    ctx.cardLayout->addWidget(createComboBoxField(settingLabel, m_settingCombo, {
        QString::fromUtf8("现代"),
        QString::fromUtf8("古代"),
        QString::fromUtf8("未来"),
        QString::fromUtf8("奇幻"),
        QString::fromUtf8("末日"),
        QString::fromUtf8("校园"),
        QString::fromUtf8("宫廷"),
        QString::fromUtf8("其他")
    }));
    
    ctx.cardLayout->addWidget(createComboBoxField(timeOfDayLabel, m_timeOfDayCombo, {
        QString::fromUtf8("黎明"),
        QString::fromUtf8("早晨"),
        QString::fromUtf8("中午"),
        QString::fromUtf8("下午"),
        QString::fromUtf8("傍晚"),
        QString::fromUtf8("夜晚"),
        QString::fromUtf8("深夜"),
        QString::fromUtf8("午夜")
    }));
    
    ctx.cardLayout->addWidget(createComboBoxField(weatherLabel, m_weatherCombo, {
        QString::fromUtf8("晴朗"),
        QString::fromUtf8("多云"),
        QString::fromUtf8("阴天"),
        QString::fromUtf8("小雨"),
        QString::fromUtf8("大雨"),
        QString::fromUtf8("雪"),
        QString::fromUtf8("雾")
    }));
    
    ctx.cardLayout->addWidget(createInputField(narrativeRoleLabel, m_narrativeRoleEdit, narrativeRolePlaceholder));
    
    finishEditorCard(ctx);
}

void BibleItem::setImage(const QString &imageUrl)
{
    m_currentImagePath = imageUrl;
    
    if (imageUrl.isEmpty()) {
        QString noImg = QString::fromUtf8("暂无图片");
        m_imageLabel->setStyleSheet(EditorStyles::bibleImagePlaceholderStyle());
        m_imageLabel->setText(noImg);
        m_imageLabel->setCursor(Qt::ArrowCursor);
        return;
    }
    
    QString id = QString("bible_%1_%2").arg(m_name).arg(QFileInfo(imageUrl).fileName());
    QString cacheKey = AsyncImageLoader::makeCacheKey(id, BibleItemConstants::LOAD_SIZE);
    
    QPixmap cachedPixmap;
    if (AsyncImageLoader::instance()->findCached(cacheKey, &cachedPixmap)) {
        displayProcessedImage(cachedPixmap);
        return;
    }
    
    QString loadingText = QString::fromUtf8("加载中...");
    m_imageLabel->setStyleSheet(EditorStyles::bibleImagePlaceholderStyle());
    m_imageLabel->setText(loadingText);
    m_imageLabel->setCursor(Qt::ArrowCursor);
    
    m_loadingImageId = id;
    
    disconnect(AsyncImageLoader::instance(), nullptr, this, nullptr);
    connect(AsyncImageLoader::instance(), &AsyncImageLoader::imageLoaded,
            this, &BibleItem::onAsyncImageLoaded);
    
    AsyncImageLoader::instance()->loadAsync(id, imageUrl, BibleItemConstants::LOAD_SIZE);
}

void BibleItem::onAsyncImageLoaded(const QString& id, const QString& cacheKey, const QPixmap& pixmap)
{
    if (id != m_loadingImageId) {
        return;
    }
    
    if (!pixmap.isNull()) {
        AsyncImageLoader::instance()->insertCache(cacheKey, pixmap);
        displayProcessedImage(pixmap);
    } else {
        QString loadFailed = QString::fromUtf8("加载失败");
        m_imageLabel->setStyleSheet(EditorStyles::bibleImagePlaceholderStyle());
        m_imageLabel->setText(loadFailed);
        m_imageLabel->setCursor(Qt::ArrowCursor);
    }
}

void BibleItem::displayProcessedImage(const QPixmap& pixmap)
{
    QPixmap displayPixmap = (m_bibleType == BibleType::Scene)
        ? trimWhiteBorders(pixmap)
        : pixmap;
    
    if (displayPixmap.isNull()) {
        return;
    }

    if (m_bibleType == BibleType::Character) {
        displayPixmap = scalePixmapToFillCenterCrop(displayPixmap, BibleItemConstants::LABEL_SIZE);
        m_imageLabel->setAlignment(Qt::AlignCenter);
    } else {
        displayPixmap = scalePixmapToFillCenterCrop(displayPixmap, BibleItemConstants::LABEL_SIZE);
        m_imageLabel->setAlignment(Qt::AlignCenter);
    }

    m_imageLabel->setPixmap(displayPixmap);
    m_imageLabel->setStyleSheet(EditorStyles::bibleImageHasImageStyle());
    m_imageLabel->setText("");
    m_imageLabel->setCursor(Qt::PointingHandCursor);
}

QPixmap BibleItem::trimWhiteBorders(const QPixmap& pixmap, int threshold, int margin)
{
    if (pixmap.isNull() || pixmap.width() <= 0 || pixmap.height() <= 0) {
        return pixmap;
    }
    
    QImage img = pixmap.toImage().convertToFormat(QImage::Format_ARGB32);
    
    int w = img.width();
    int h = img.height();
    
    // 查找内容边界
    int top = findContentBoundary(img, threshold, 0);
    int bottom = findContentBoundary(img, threshold, 1);
    int left = findContentBoundary(img, threshold, 2);
    int right = findContentBoundary(img, threshold, 3);
    
    // 找不到内容边界时直接返回原图
    if (top < 0 || bottom < 0 || left < 0 || right < 0) {
        return pixmap;
    }
    
    top = qMax(0, top - margin);
    bottom = qMin(h - 1, bottom + margin);
    left = qMax(0, left - margin);
    right = qMin(w - 1, right + margin);
    
    int cropW = right - left + 1;
    int cropH = bottom - top + 1;
    
    if (cropW <= 0 || cropH <= 0 || cropW >= w || cropH >= h) {
        return pixmap;
    }
    
    return pixmap.copy(left, top, cropW, cropH);
}

void BibleItem::setItemId(const QString &id)
{
    m_itemId = id;
}

void BibleItem::setDetails(const QStringList &details)
{
    m_details = details;
    populateEditorData();
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
    QString collapseText = QString::fromUtf8("收起");
    m_editBtn->setText(collapseText);
}

void BibleItem::hideEditorCard()
{
    if (m_overlayWidget) {
        m_overlayWidget->hide();
    }
    m_isExpanded = false;
    QString editText = QString::fromUtf8("编辑");
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
    
    emit editClicked(m_itemId, m_bibleType);
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

void BibleItem::setCharacterData(const Character& character)
{
    m_characterData = character;
    m_hasCharacterData = true;
}

void BibleItem::setSceneData(const Scene& scene)
{
    m_sceneData = scene;
    m_hasSceneData = true;
}

void BibleItem::populateCharacterEditorData()
{
    if (m_hasCharacterData) {
        populateCharacterEditorFromData(m_characterData);
        return;
    }
    
    if (m_details.isEmpty()) return;
    if (!m_genderCombo || !m_ageSpin || !m_hairColorEdit || !m_eyeColorEdit) return;
    
    for (const QString &detail : m_details) {
        const QString gender = BibleUtils::extractDetailValue(
            detail, QStringList{QStringLiteral("gender"), QString::fromUtf8("性别")});
        if (!gender.isEmpty()) {
            setComboText(m_genderCombo, gender);
        }

        const int age = parsePositiveInt(BibleUtils::extractDetailValue(
            detail, QStringList{QStringLiteral("age"), QString::fromUtf8("年龄")}));
        if (age > 0) {
            m_ageSpin->setValue(age);
        }

        setLineEditText(m_hairColorEdit, BibleUtils::extractDetailValue(
            detail, QStringList{QStringLiteral("hairColor"), QString::fromUtf8("发色")}));
        setLineEditText(m_eyeColorEdit, BibleUtils::extractDetailValue(
            detail, QStringList{QStringLiteral("eyeColor"), QString::fromUtf8("瞳色"), QString::fromUtf8("眼睛")}));
        setLineEditText(m_bodyTypeEdit, BibleUtils::extractDetailValue(
            detail, QStringList{QStringLiteral("bodyType"), QString::fromUtf8("体型")}));
        setLineEditText(m_hairStyleEdit, BibleUtils::extractDetailValue(
            detail, QStringList{QStringLiteral("hairStyle"), QString::fromUtf8("发型")}));

        const QString clothing = BibleUtils::extractDetailValue(
            detail, QStringList{QStringLiteral("clothing"), QString::fromUtf8("服饰")});
        if (!clothing.isEmpty()) {
            setLineEditText(m_clothingEdit, BibleUtils::splitCommaSeparatedList(clothing).join(QStringLiteral(", ")));
        }

        const QString features = BibleUtils::extractDetailValue(
            detail, QStringList{QStringLiteral("distinctiveFeatures"), QString::fromUtf8("明显特征"), QString::fromUtf8("特征")});
        if (!features.isEmpty()) {
            setLineEditText(m_featuresEdit, BibleUtils::splitCommaSeparatedList(features).join(QStringLiteral(", ")));
        }

        const QString personality = BibleUtils::extractDetailValue(
            detail, QStringList{QStringLiteral("personality"), QString::fromUtf8("个性"), QString::fromUtf8("性格")});
        if (!personality.isEmpty()) {
            setLineEditText(m_personalityEdit, BibleUtils::splitCommaSeparatedList(personality).join(QStringLiteral(", ")));
        }
    }
}

void BibleItem::populateCharacterEditorFromData(const Character& character)
{
    if (!m_genderCombo || !m_ageSpin || !m_hairColorEdit || !m_eyeColorEdit) return;
    
    CharacterAppearance app = character.appearance();
    
    // 填充角色信息
    setComboText(m_genderCombo, app.gender);
    
    // 年龄
    if (app.age > 0) {
        m_ageSpin->setValue(app.age);
    }
    
    // 发色
    setLineEditText(m_hairColorEdit, app.hairColor);
    
    // 眼睛颜色
    setLineEditText(m_eyeColorEdit, app.eyeColor);
    
    // 体型
    setLineEditText(m_bodyTypeEdit, app.build);
    
    // 发型
    setLineEditText(m_hairStyleEdit, app.hairStyle);
    
    // 服饰
    setLineEditText(m_clothingEdit, app.clothing.join(", "));
    
    // 特征
    setLineEditText(m_featuresEdit, app.distinctiveFeatures.join(", "));
    
    // 性格
    setLineEditText(m_personalityEdit, character.personality().join(", "));
}

void BibleItem::populateSceneEditorData()
{
    if (m_hasSceneData) {
        populateSceneEditorFromData(m_sceneData);
        return;
    }
    
    if (m_details.isEmpty()) return;
    if (!m_sceneNameEdit || !m_sceneDescEdit) return;
    
    m_sceneNameEdit->setText(m_name);
    
    for (const QString &detail : m_details) {
        setTextEditText(m_sceneDescEdit, BibleUtils::extractDetailValue(
            detail, QStringList{QStringLiteral("description"), QString::fromUtf8("描述")}));
        setLineEditText(m_buildingEdit, BibleUtils::extractDetailValue(
            detail, QStringList{QStringLiteral("building"), QString::fromUtf8("建筑")}));
        setLineEditText(m_colorEdit, BibleUtils::extractDetailValue(
            detail, QStringList{QStringLiteral("color"), QString::fromUtf8("色调")}));
        setLineEditText(m_landmarkEdit, BibleUtils::extractDetailValue(
            detail, QStringList{QStringLiteral("landmark"), QString::fromUtf8("地标")}));
        setLineEditText(m_layoutEdit, BibleUtils::extractDetailValue(
            detail, QStringList{QStringLiteral("layout"), QString::fromUtf8("布局")}));
        setLineEditText(m_atmosphereEdit, BibleUtils::extractDetailValue(
            detail, QStringList{QStringLiteral("atmosphere"), QString::fromUtf8("氛围")}));
    }
}

void BibleItem::populateSceneEditorFromData(const Scene& scene)
{
    if (!m_sceneNameEdit || !m_sceneDescEdit) return;
    
    m_sceneNameEdit->setText(scene.name());
    
    SceneDetails det = scene.details();
    
    setTextEditText(m_sceneDescEdit, det.description);
    setLineEditText(m_buildingEdit, det.building);
    setLineEditText(m_colorEdit, det.color);
    setLineEditText(m_landmarkEdit, det.landmark);
    setLineEditText(m_layoutEdit, det.layout);
    setLineEditText(m_atmosphereEdit, det.atmosphere);
    
    // 选择类型
    setModeComboText(m_typeCombo, det.type);
    setModeComboText(m_settingCombo, det.setting);
    setModeComboText(m_timeOfDayCombo, det.timeOfDay);
    setModeComboText(m_weatherCombo, det.weather);
    setLineEditText(m_narrativeRoleEdit, det.narrativeRole);
}

void BibleItem::saveCharacterData()
{
    Character updated = m_hasCharacterData ? m_characterData : Character();
    if (!m_hasCharacterData) {
        updated.setId(m_itemId);
    }
    const bool isNewCharacter = !m_hasCharacterData;
    const CharacterAppearance originalApp = updated.appearance();
    const QStringList originalPersonality = updated.personality();
    
    CharacterAppearance app = updated.appearance();
    
    app.gender = m_genderCombo->currentText();
    app.age = m_ageSpin->value();
    app.hairColor = m_hairColorEdit->text().trimmed();
    app.eyeColor = m_eyeColorEdit->text().trimmed();
    app.build = m_bodyTypeEdit->text().trimmed();
    app.hairStyle = m_hairStyleEdit->text().trimmed();
    
    app.clothing = BibleUtils::splitCommaSeparatedList(m_clothingEdit->text());
    
    app.distinctiveFeatures = BibleUtils::splitCommaSeparatedList(m_featuresEdit->text());
    const QStringList personality = BibleUtils::splitCommaSeparatedList(m_personalityEdit->text());
    updated.setPersonality(personality);
    markCharacterAppearanceManual(app, originalApp, isNewCharacter);
    if (isNewCharacter || personality != originalPersonality) {
        app.fieldSources["personality"] = MANUAL_SOURCE;
    }
    updated.setAppearance(app);
    
    m_characterData = updated;
    m_hasCharacterData = true;
    
    QStringList newDetails = updated.toDisplayStrings();
    m_details = newDetails;
    setDetails(newDetails);
    
    emit characterDataChanged(m_itemId, updated);
}

void BibleItem::saveSceneData()
{
    QString newName = m_sceneNameEdit->text().trimmed();
    if (!newName.isEmpty()) {
        m_name = newName;
        m_nameLabel->setText(QString("<b>%1</b>").arg(m_name));
    }
    
    Scene updated = m_hasSceneData ? m_sceneData : Scene();
    if (!m_hasSceneData) {
        updated.setId(m_itemId);
    }
    const bool isNewScene = !m_hasSceneData;
    updated.setName(m_name);
    const SceneDetails originalDetails = updated.details();
    
    SceneDetails det = collectSceneDetailsFromEditors();
    markSceneDetailsManual(det, originalDetails, isNewScene);
    updated.setDetails(det);
    
    m_sceneData = updated;
    m_hasSceneData = true;
    
    QStringList newDetails = det.toDisplayStrings();
    m_details = newDetails;
    setDetails(newDetails);
    
    emit sceneDataChanged(m_itemId, updated);
}

SceneDetails BibleItem::collectSceneDetailsFromEditors() const
{
    SceneDetails det = m_hasSceneData ? m_sceneData.details() : SceneDetails();

    if (m_sceneDescEdit) det.description = m_sceneDescEdit->toPlainText().trimmed();
    if (m_buildingEdit) det.building = m_buildingEdit->text().trimmed();
    if (m_colorEdit) det.color = m_colorEdit->text().trimmed();
    if (m_landmarkEdit) det.landmark = m_landmarkEdit->text().trimmed();
    if (m_layoutEdit) det.layout = m_layoutEdit->text().trimmed();
    if (m_atmosphereEdit) det.atmosphere = m_atmosphereEdit->text().trimmed();

    if (m_typeCombo) det.type = m_typeCombo->currentText();
    if (m_settingCombo) det.setting = m_settingCombo->currentText();
    if (m_timeOfDayCombo) det.timeOfDay = m_timeOfDayCombo->currentText();
    if (m_weatherCombo) det.weather = m_weatherCombo->currentText();
    if (m_narrativeRoleEdit) det.narrativeRole = m_narrativeRoleEdit->text().trimmed();
    return det;
}

void BibleItem::onCancelClicked()
{
    hideEditorCard();
}
