#include "components/StoryboardItem.h"
#include "components/EditorStyles.h"
#include "utils/ShotTypeHelper.h"
#include "utils/Logger.h"
#include "utils/UIFactory.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QApplication>
#include <QMap>

namespace {
    using EditorStyles::TRANSPARENT_BG;
}

StoryboardItem::StoryboardItem(int panelNumber, const QString &panelId,
                               const QString &scene, 
                               const QString &shotType, const QString &cameraAngle,
                               const QString &characters, const QString &dialogue,
                               const QString &visualPrompt, const QString &visualPromptEn, 
                               QWidget *parent)
    : EditorCardBase(parent)
    , m_panelNumberLabel(nullptr)
    , m_sceneLabel(nullptr)
    , m_shotTypeLabel(nullptr)
    , m_charactersLabel(nullptr)
    , m_dialogueLabel(nullptr)
    , m_editBtn(nullptr)
    , m_panelId(panelId)
    , m_panelNumber(panelNumber)
    , m_scene(scene)
    , m_shotType(shotType)
    , m_cameraAngle(cameraAngle)
    , m_characters(characters)
    , m_dialogue(dialogue)
    , m_visualPrompt(visualPrompt)
    , m_visualPromptEn(visualPromptEn)
    , m_sceneEdit(nullptr)
    , m_shotTypeCombo(nullptr)
    , m_cameraCombo(nullptr)
    , m_charactersEdit(nullptr)
    , m_dialogueEdit(nullptr)
    , m_imagenPromptEdit(nullptr)
    , m_saveBtn(nullptr)
    , m_cancelBtn(nullptr)
{
    setupUI();
}

void StoryboardItem::setupUI()
{
    setObjectName("storyboardItem");
    setStyleSheet(EditorStyles::storyboardItemStyle());
    
    setMinimumHeight(180);
    setMinimumWidth(300);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    
    QGraphicsDropShadowEffect *shadow = UIFactory::createShadowEffect(12, QColor(0, 0, 0, 20), QPointF(0, 2));
    setGraphicsEffect(shadow);
    
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 16, 20, 16);
    mainLayout->setSpacing(12);
    
    QString sceneTitle = QString::fromUtf8("场景");
    QString shotTypeTitle = QString::fromUtf8("景别/机位");
    QString charactersTitle = QString::fromUtf8("角色");
    QString dialogueTitle = QString::fromUtf8("对白");
    QString emptyDialogue = QString::fromUtf8("(空)");
    
    QString shotTypeDisplay = ShotTypeHelper::ShotType::toChinese(m_shotType);
    QString cameraAngleDisplay = ShotTypeHelper::CameraAngle::toChinese(m_cameraAngle);
    QString shotTypeInfo = shotTypeDisplay;
    if (!cameraAngleDisplay.isEmpty()) {
        shotTypeInfo = QString("%1 / %2").arg(shotTypeDisplay, cameraAngleDisplay);
    }
    
    mainLayout->addWidget(createHeaderRow());
    mainLayout->addWidget(createInfoRow(sceneTitle, m_sceneLabel, m_scene, true));
    mainLayout->addWidget(createInfoRow(shotTypeTitle, m_shotTypeLabel, shotTypeInfo));
    mainLayout->addWidget(createInfoRow(charactersTitle, m_charactersLabel, m_characters));
    mainLayout->addWidget(createInfoRow(dialogueTitle, m_dialogueLabel, m_dialogue.isEmpty() ? emptyDialogue : m_dialogue, true));
}

QWidget* StoryboardItem::createHeaderRow()
{
    QWidget *row = UIFactory::createTransparentWidget();
    row->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    row->setMinimumHeight(32);
    QHBoxLayout *layout = UIFactory::createHBoxLayout(0, 8);
    
    QString panelText = QString::fromUtf8("面板 #%1").arg(m_panelNumber);
    m_panelNumberLabel = UIFactory::createLabel(panelText, EditorStyles::panelNumberLabelStyle());
    layout->addWidget(m_panelNumberLabel);
    layout->addStretch();
    
    QString editText = QString::fromUtf8("编辑");
    m_editBtn = UIFactory::createButton(editText, 90, 32, EditorStyles::editButtonStyle());
    m_editBtn->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    connect(m_editBtn, &QPushButton::clicked, this, &StoryboardItem::onEditBtnClicked);
    layout->addWidget(m_editBtn);
    
    row->setLayout(layout);
    return row;
}

QWidget* StoryboardItem::createInfoRow(const QString &title, QLabel *&contentLabel, const QString &content, bool wordWrap)
{
    QWidget *row = UIFactory::createTransparentWidget();
    QHBoxLayout *layout = UIFactory::createHBoxLayout(0, 8);
    
    QLabel *titleLabel = UIFactory::createLabel(title, EditorStyles::storyboardInfoTitleStyle());
    titleLabel->setFixedWidth(60);
    layout->addWidget(titleLabel);
    
    contentLabel = UIFactory::createLabel(content, EditorStyles::storyboardInfoContentStyle());
    contentLabel->setWordWrap(wordWrap);
    layout->addWidget(contentLabel);
    
    row->setLayout(layout);
    return row;
}

void StoryboardItem::setScene(const QString &scene)
{
    m_scene = scene;
    if (m_sceneLabel) m_sceneLabel->setText(scene);
}

void StoryboardItem::setShotType(const QString &shotType)
{
    m_shotType = shotType;
    if (m_shotTypeLabel) m_shotTypeLabel->setText(shotType);
}

void StoryboardItem::setCameraAngle(const QString &cameraAngle)
{
    m_cameraAngle = cameraAngle;
}

void StoryboardItem::setCharacters(const QString &characters)
{
    m_characters = characters;
    if (m_charactersLabel) m_charactersLabel->setText(characters);
}

void StoryboardItem::setDialogue(const QString &dialogue)
{
    m_dialogue = dialogue;
    QString emptyText = QString::fromUtf8("(空)");
    if (m_dialogueLabel) m_dialogueLabel->setText(dialogue.isEmpty() ? emptyText : dialogue);
}

void StoryboardItem::setVisualPrompt(const QString &prompt)
{
    m_visualPrompt = prompt;
}

void StoryboardItem::setVisualPromptEn(const QString &prompt)
{
    m_visualPromptEn = prompt;
}

void StoryboardItem::onEditBtnClicked()
{
    showEditorCard();
}

void StoryboardItem::setupEditorCard()
{
    m_overlayWidget = createOverlay();
    m_editorCard = createCardBase(560, 620);
    m_editorCard->setObjectName("storyboardEditorCard");
    
    QVBoxLayout *cardMainLayout = new QVBoxLayout(m_editorCard);
    cardMainLayout->setContentsMargins(0, 0, 0, 0);
    cardMainLayout->setSpacing(0);
    
    QString titleText = QString::fromUtf8("编辑面板 #%1").arg(m_panelNumber);
    QWidget *titleRow = createTitleRow(titleText);
    titleRow->setContentsMargins(24, 20, 24, 0);
    cardMainLayout->addWidget(titleRow);
    
    QScrollArea *scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setStyleSheet(EditorStyles::scrollAreaStyle());
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    
    QWidget *scrollContent = new QWidget();
    scrollContent->setStyleSheet("background: transparent;");
    QVBoxLayout *cardLayout = new QVBoxLayout(scrollContent);
    cardLayout->setContentsMargins(24, 12, 24, 12);
    cardLayout->setSpacing(12);
    
    QString scenePlaceholder = QString::fromUtf8("请输入场景描述...");
    QString charactersPlaceholder = QString::fromUtf8("请输入角色信息...");
    QString dialoguePlaceholder = QString::fromUtf8("请输入对白内容...");
    QString sceneHint = QString::fromUtf8("用于描述当前分镜场景");
    QString charactersHint = QString::fromUtf8("多个角色用 | 分隔，格式：角色名（表情、动作）");
    QString dialogueHint = QString::fromUtf8("格式：角色名：对白内容");
    QString promptPlaceholder = QString::fromUtf8("输入图像提示词...");
    QString promptHint = QString::fromUtf8("可选，用于补充分镜画面提示");
    
    QString wide = QString::fromUtf8("远景");
    QString medium = QString::fromUtf8("中景");
    QString closeShot = QString::fromUtf8("近景");
    QString closeup = QString::fromUtf8("特写");
    QString longShot = QString::fromUtf8("全景");
    QString panoramic = QString::fromUtf8("远全景");
    QString highAngle = QString::fromUtf8("俯视");
    QString lowAngle = QString::fromUtf8("仰视");
    QString eyeLevel = QString::fromUtf8("平视");
    QString dutchAngle = QString::fromUtf8("倾斜");
    QString birdEye = QString::fromUtf8("鸟瞰");
    QString wormEye = QString::fromUtf8("虫视");
    
    cardLayout->addWidget(createComboBoxGroup(QString::fromUtf8("景别"), m_shotTypeCombo,
        {wide, medium, closeShot, closeup, longShot, panoramic},
        QString::fromUtf8("机位"), m_cameraCombo,
        {highAngle, lowAngle, eyeLevel, dutchAngle, birdEye, wormEye}));
    cardLayout->addWidget(createTextEditSection(QString::fromUtf8("场景"), m_sceneEdit, m_scene, 100, scenePlaceholder, sceneHint));
    cardLayout->addWidget(createTextEditSection(QString::fromUtf8("角色"), m_charactersEdit, m_characters, 80, charactersPlaceholder, charactersHint));
    cardLayout->addWidget(createTextEditSection(QString::fromUtf8("对白"), m_dialogueEdit, m_dialogue, 80, dialoguePlaceholder, dialogueHint));
    cardLayout->addWidget(createTextEditSection(QString::fromUtf8("图像提示词"), m_imagenPromptEdit, QString(), 60, promptPlaceholder, promptHint));
    cardLayout->addStretch();
    
    scrollArea->setWidget(scrollContent);
    cardMainLayout->addWidget(scrollArea, 1);
    
    QString saveText = QString::fromUtf8("保存");
    QString cancelText = QString::fromUtf8("取消");
    QWidget *buttonRow = createButtonRow(m_saveBtn, saveText, m_cancelBtn, cancelText);
    buttonRow->setContentsMargins(24, 0, 24, 20);
    cardMainLayout->addWidget(buttonRow);
    
    connect(m_saveBtn, &QPushButton::clicked, this, &StoryboardItem::onSaveClicked);
    connect(m_cancelBtn, &QPushButton::clicked, this, &StoryboardItem::onCancelClicked);
}

void StoryboardItem::syncDataToEditor()
{
    if (m_sceneEdit) m_sceneEdit->setPlainText(m_scene);
    
    QString shotTypeChinese = ShotTypeHelper::ShotType::toChinese(m_shotType);
    if (m_shotTypeCombo) {
        int shotIndex = m_shotTypeCombo->findText(shotTypeChinese, Qt::MatchContains);
        if (shotIndex >= 0) m_shotTypeCombo->setCurrentIndex(shotIndex);
    }
    
    QString cameraAngleChinese = ShotTypeHelper::CameraAngle::toChinese(m_cameraAngle);
    if (m_cameraCombo) {
        int cameraIndex = m_cameraCombo->findText(cameraAngleChinese, Qt::MatchContains);
        if (cameraIndex >= 0) m_cameraCombo->setCurrentIndex(cameraIndex);
    }
    
    if (m_charactersEdit) m_charactersEdit->setPlainText(m_characters);
    if (m_dialogueEdit) m_dialogueEdit->setPlainText(m_dialogue);
    if (m_imagenPromptEdit) m_imagenPromptEdit->setPlainText(m_visualPrompt);
}

QWidget* StoryboardItem::createComboBoxGroup(const QString &title1, QComboBox *&combo1, const QStringList &items1,
                                              const QString &title2, QComboBox *&combo2, const QStringList &items2)
{
    QWidget *section = UIFactory::createTransparentWidget();
    QVBoxLayout *outerLayout = UIFactory::createVBoxLayout(0, 6);
    
    QString shotCameraTitle = QString::fromUtf8("景别 / 机位");
    outerLayout->addWidget(createLabel(shotCameraTitle, "#333333", 12, true));
    
    QWidget *row = UIFactory::createTransparentWidget();
    QHBoxLayout *rowLayout = UIFactory::createHBoxLayout(0, 12);
    
    QWidget *group1 = UIFactory::createTransparentWidget();
    QVBoxLayout *layout1 = UIFactory::createVBoxLayout(0, 4);
    layout1->addWidget(createLabel(title1, "#6B7280", 11));
    layout1->addWidget(createComboBoxWithArrow(combo1, items1));
    group1->setLayout(layout1);
    rowLayout->addWidget(group1, 1);
    
    QWidget *group2 = UIFactory::createTransparentWidget();
    QVBoxLayout *layout2 = UIFactory::createVBoxLayout(0, 4);
    layout2->addWidget(createLabel(title2, "#6B7280", 11));
    layout2->addWidget(createComboBoxWithArrow(combo2, items2));
    group2->setLayout(layout2);
    rowLayout->addWidget(group2, 1);
    
    row->setLayout(rowLayout);
    outerLayout->addWidget(row);
    section->setLayout(outerLayout);
    return section;
}

QWidget* StoryboardItem::createComboBoxWithArrow(QComboBox *&combo, const QStringList &items)
{
    QWidget *container = UIFactory::createTransparentWidget();
    container->setFixedHeight(36);
    
    QHBoxLayout *layout = UIFactory::createHBoxLayout(0, 0);
    
    combo = UIFactory::createComboBox(items);
    combo->setStyleSheet(EditorStyles::storyboardComboBoxStyle());
    combo->setFixedHeight(36);
    layout->addWidget(combo, 1);
    
    QWidget *btnContainer = new QWidget();
    btnContainer->setFixedWidth(28);
    btnContainer->setStyleSheet(EditorStyles::storyboardComboBoxBtnStyle());
    
    QVBoxLayout *btnLayout = UIFactory::createVBoxLayout(2, 0, 2, 0, 0);
    
    QString arrowText = QStringLiteral("▼");
    QPushButton *arrowBtn = UIFactory::createButton(arrowText, 24, 20);
    arrowBtn->setStyleSheet(R"(
        QPushButton {
            border: none;
            background: transparent;
            color: #4d7c0f;
            font-size: 8px;
            border-radius: 4px;
        }
        QPushButton:hover { 
            background: #2684cc16; 
        }
        QPushButton:pressed { 
            background: #4084cc16; 
        }
    )");
    connect(arrowBtn, &QPushButton::clicked, [this, combo]() { combo->showPopup(); });
    btnLayout->addWidget(arrowBtn);
    
    btnContainer->setLayout(btnLayout);
    layout->addWidget(btnContainer);
    
    container->setLayout(layout);
    return container;
}

void StoryboardItem::onSaveClicked()
{
    LOG_INFO("StoryboardItem", QString("onSaveClicked: panelId=%1, panelNumber=%2")
        .arg(m_panelId).arg(m_panelNumber));
    
    m_scene = m_sceneEdit->toPlainText().trimmed();
    m_shotType = m_shotTypeCombo->currentText();
    m_cameraAngle = m_cameraCombo->currentText();
    m_characters = m_charactersEdit->toPlainText().trimmed();
    m_dialogue = m_dialogueEdit->toPlainText().trimmed();
    m_visualPrompt = m_imagenPromptEdit->toPlainText().trimmed();
    
    m_sceneLabel->setText(m_scene);
    
    QString shotTypeDisplay = ShotTypeHelper::ShotType::toChinese(m_shotType);
    QString cameraAngleDisplay = ShotTypeHelper::CameraAngle::toChinese(m_cameraAngle);
    QString shotTypeInfo = shotTypeDisplay;
    if (!cameraAngleDisplay.isEmpty()) {
        shotTypeInfo = QString("%1 / %2").arg(shotTypeDisplay, cameraAngleDisplay);
    }
    m_shotTypeLabel->setText(shotTypeInfo);
    
    m_charactersLabel->setText(m_characters);
    
    QString emptyText = QString::fromUtf8("(空)");
    m_dialogueLabel->setText(m_dialogue.isEmpty() ? emptyText : m_dialogue);
    
    LOG_INFO("StoryboardItem", QString("onSaveClicked: emitting dataChanged signal"));
    
    emit dataChanged(m_panelId, m_panelNumber, m_scene, m_shotType, m_cameraAngle, m_characters, m_dialogue, m_visualPrompt, m_visualPromptEn);
    
    hideEditorCard();
}

void StoryboardItem::onCancelClicked()
{
    hideEditorCard();
}
