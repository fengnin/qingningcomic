#include "components/PanelEditorWidget.h"
#include "components/EditorStyles.h"
#include "ImageService.h"
#include "Logger.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTextEdit>
#include <QPushButton>
#include <QFileDialog>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSizePolicy>
#include <QtConcurrent>

namespace {
    using EditorStyles::TRANSPARENT_BG;
    
    namespace Text {
        const QString PREVIEW = QString::fromUtf8("预览");
        const QString LOADING = QString::fromUtf8("加载中...");
        const QString LOAD_FAILED = QString::fromUtf8("加载失败");
        const QString WAIT_GENERATE = QString::fromUtf8("待生成");
        const QString SCENE = QString::fromUtf8("场景");
        const QString POSITION = QString::fromUtf8("位置");
        const QString SCENE_DESC = QString::fromUtf8("场景描述");
        const QString EDIT_MODE = QString::fromUtf8("编辑模式");
        const QString EDIT_INSTRUCTION = QString::fromUtf8("编辑指令");
        const QString INPAINT = QString::fromUtf8("局部重绘");
        const QString OUTPAINT = QString::fromUtf8("扩展绘制");
        const QString BG_SWAP = QString::fromUtf8("背景替换");
        const QString SAVE_SCENE = QString::fromUtf8("保存场景");
        const QString SUBMIT_EDIT = QString::fromUtf8("提交编辑");
        const QString SELECT_FILE = QString::fromUtf8("选择文件");
        const QString MASK_FILE = QString::fromUtf8("遮罩文件");
        const QString NOT_SELECTED = QString::fromUtf8("未选择");
        const QString SELECTED = QString::fromUtf8("已选择");
        const QString TASK_STATUS = QString::fromUtf8("任务状态");
        const QString PENDING = QString::fromUtf8("等待中");
        const QString PROCESSING = QString::fromUtf8("处理中");
        const QString SUCCESS = QString::fromUtf8("成功");
        const QString FAILED = QString::fromUtf8("失败");
        const QString SCENE_HINT = QString::fromUtf8("用于生成当前分镜画面");
        const QString EDIT_HINT = QString::fromUtf8("输入修改要求，例如局部重绘、扩展画面或背景替换...");
    }
    
    namespace Size {
        constexpr int DIALOG_WIDTH = 860;
        constexpr int DIALOG_HEIGHT = 660;
        constexpr int PREVIEW_WIDTH = 240;
        constexpr int PREVIEW_HEIGHT = 320;
        constexpr int SCENE_DESC_HEIGHT = 120;
        constexpr int INSTRUCTION_HEIGHT = 104;
    }
    
    namespace Style {
        const QString SECTION_TITLE = "font-size: 13px; font-weight: 600; color: #374151; background: transparent;";
        const QString INFO_TEXT = "font-size: 12px; color: #6B7280; background: transparent;";
        const QString HERO_CARD = R"(
            QWidget#heroCard {
                background: rgba(255, 255, 255, 0.82);
                border-radius: 18px;
                border: 1px solid #E5E7EB;
            }
        )";
        const QString CARD_BG = R"(
            QWidget#sectionCard {
                background: #F9FAFB;
                border-radius: 12px;
                border: 1px solid #E5E7EB;
            }
        )";
        
        const QString PREVIEW_PLACEHOLDER = R"(
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                stop:0 #F3F4F6, stop:1 #E5E7EB);
            border-radius: 12px;
            border: 2px dashed #D1D5DB;
            color: #9CA3AF;
            font-size: 13px;
            font-weight: 500;
        )";
        
        const QString PANEL_ID_STYLE = "font-size: 16px; font-weight: bold; color: #1F2937; background: transparent;";
        const QString PANEL_META_STYLE = "font-size: 13px; color: #6B7280; background: transparent;";
        const QString SCENE_SUMMARY_STYLE = "font-size: 13px; color: #4B5563; background: transparent; line-height: 1.5;";
        
        const QString SCENE_EDIT_STYLE = R"(
            QTextEdit {
                padding: 14px 16px;
                font-size: 13px;
                font-family: 'Microsoft YaHei', sans-serif;
                border: 1.5px solid #E5E7EB;
                border-radius: 10px;
                background: #ffffff;
                color: #374151;
                line-height: 1.5;
            }
            QTextEdit:focus {
                border-color: #84cc16;
                background: #FEFCE8;
            }
            QScrollBar:vertical {
                background: #F3F4F6;
                width: 6px;
                margin: 4px 2px 4px 0;
                border-radius: 3px;
            }
            QScrollBar::handle:vertical {
                background: #D1D5DB;
                border-radius: 3px;
                min-height: 20px;
            }
            QScrollBar::handle:vertical:hover {
                background: #9CA3AF;
            }
            QScrollBar::handle:vertical:pressed {
                background: #84cc16;
            }
            QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
                height: 0px;
                background: transparent;
            }
            QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {
                background: transparent;
            }
        )";
        
        const QString HINT_LABEL_STYLE = "font-size: 11px; color: #9CA3AF; background: transparent; padding-left: 4px;";
        
        const QString MODE_GROUP_CONTAINER = R"(
            QWidget#modeGroup {
                background: #F3F4F6;
                border-radius: 10px;
                padding: 4px;
            }
        )";
        
        const QString MODE_BTN_NORMAL = R"(
            QPushButton {
                background: transparent;
                color: #6B7280;
                font-size: 13px;
                font-weight: 500;
                border: none;
                border-radius: 7px;
                padding: 8px 16px;
            }
            QPushButton:hover {
                background: #b3ffffff;
                color: #374151;
            }
        )";
        
        const QString MODE_BTN_ACTIVE = R"(
            QPushButton {
                background: #ffffff;
                color: #4d7c0f;
                font-size: 13px;
                font-weight: 600;
                border: none;
                border-radius: 7px;
                padding: 8px 16px;
            }
        )";
        
        const QString MASK_ROW_STYLE = R"(
            QPushButton {
                padding: 6px 14px;
                font-size: 12px;
                border: 1.5px solid #E5E7EB;
                border-radius: 8px;
                background: white;
                color: #4B5563;
            }
            QPushButton:hover {
                border-color: #84cc16;
                background: #F7FEE7;
                color: #4d7c0f;
            }
        )";
        
        const QString SUBMIT_BTN_STYLE = R"(
            QPushButton {
                padding: 14px 32px;
                font-size: 15px;
                font-weight: bold;
                background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                    stop:0 #84cc16, stop:1 #65a30d);
                color: white;
                border: none;
                border-radius: 12px;
            }
            QPushButton:hover {
                background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                    stop:0 #a3e635, stop:1 #84cc16);
            }
            QPushButton:pressed {
                background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                    stop:0 #65a30d, stop:1 #4d7c0f);
            }
            QPushButton:disabled {
                background: #D1D5DB;
                color: #9CA3AF;
            }
        )";
        
        const QString SAVE_BTN_STYLE = R"(
            QPushButton {
                padding: 10px 24px;
                font-size: 13px;
                font-weight: 600;
                background: white;
                color: #4d7c0f;
                border: 1.5px solid #84cc16;
                border-radius: 10px;
            }
            QPushButton:hover {
                background: #F7FEE7;
            }
            QPushButton:pressed {
                background: #ECFCCB;
            }
        )";
        
        const QString STATUS_READY = "font-size: 12px; color: #9CA3AF; background: transparent; font-weight: 500;";
        const QString STATUS_PROCESSING = "font-size: 12px; color: #F59E0B; background: transparent; font-weight: 600;";
        const QString STATUS_SUCCESS = "font-size: 12px; color: #10B981; background: transparent; font-weight: 600;";
        const QString STATUS_ERROR = "font-size: 12px; color: #EF4444; background: transparent; font-weight: 600;";
        
        const QString CLOSE_BTN_STYLE = R"(
            QPushButton {
                background: #F3F4F6;
                color: #6B7280;
                border: none;
                border-radius: 8px;
                font-size: 18px;
                font-weight: bold;
            }
            QPushButton:hover {
                background: #E5E7EB;
                color: #374151;
            }
        )";
    }
}

PanelEditorWidget::PanelEditorWidget(QWidget *parent)
    : QFrame(parent)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_imageService(ImageService::instance())
{
    setupUI();
    setupConnections();
}

PanelEditorWidget::~PanelEditorWidget()
{
}

void PanelEditorWidget::setupUI()
{
    setObjectName("panelEditorWidget");
    setFrameShape(QFrame::NoFrame);
    setStyleSheet(R"(
        #panelEditorWidget {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #FAFBFF, stop:1 #F5F3FF);
            border-radius: 20px;
            border: 1px solid #266366f1;
        }
        #panelEditorWidget QLabel {
            background: transparent;
            border: none;
        }
        #panelEditorWidget QWidget {
            background: transparent;
        }
    )");
    setMinimumWidth(Size::DIALOG_WIDTH);
    setMaximumWidth(Size::DIALOG_WIDTH);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24, 20, 24, 24);
    mainLayout->setSpacing(14);

    QWidget *heroCard = new QWidget();
    heroCard->setObjectName("heroCard");
    heroCard->setStyleSheet(Style::HERO_CARD);
    QHBoxLayout *heroLayout = new QHBoxLayout(heroCard);
    heroLayout->setContentsMargins(20, 20, 20, 20);
    heroLayout->setSpacing(14);
    heroLayout->addWidget(createHeaderPreview());
    heroLayout->addWidget(createHeaderInfo(), 1);
    heroLayout->addWidget(createCloseButton(), 0, Qt::AlignTop);
    mainLayout->addWidget(heroCard);

    QWidget *bodyRow = new QWidget();
    QHBoxLayout *bodyLayout = new QHBoxLayout(bodyRow);
    bodyLayout->setContentsMargins(0, 0, 0, 0);
    bodyLayout->setSpacing(14);

    QWidget *leftColumn = new QWidget();
    QVBoxLayout *leftLayout = new QVBoxLayout(leftColumn);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(14);
    leftLayout->addWidget(createSceneDescSection());

    QWidget *rightColumn = new QWidget();
    QVBoxLayout *rightLayout = new QVBoxLayout(rightColumn);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(14);
    rightLayout->addWidget(createEditModeSection());
    rightLayout->addWidget(createInstructionSection());
    rightLayout->addWidget(createFooter());
    rightLayout->addStretch();

    bodyLayout->addWidget(leftColumn, 1);
    bodyLayout->addWidget(rightColumn, 1);
    mainLayout->addWidget(bodyRow);
    adjustSize();
}

void PanelEditorWidget::setupConnections()
{
    if (m_imageService) {
        connect(m_imageService, &ImageService::panelGenerated,
                this, [this](const ImageService::GenerateResult& result) {
                    if (result.panelId == m_panelId) {
                        if (result.success) {
                            setState(State::Success);
                            setPreviewUrl(result.imageUrl);
                            emit imageGenerated(result.imageUrl);
                        } else {
                            setState(State::Error);
                        }
                    }
                });
    }
}

void PanelEditorWidget::setPanelInfo(int chapterNumber, int panelNumber, const QString &panelId)
{
    m_chapterNumber = chapterNumber;
    m_panelNumber = panelNumber;
    m_panelId = panelId;
    
    if (m_idLabel) {
        m_idLabel->setText(QString::fromUtf8("面板 %1-%2").arg(m_chapterNumber).arg(m_panelNumber));
    }
    if (m_posLabel) {
        m_posLabel->setText(Text::POSITION + QString("P%1-%2").arg(m_chapterNumber).arg(m_panelNumber));
    }
}

void PanelEditorWidget::setSceneDescription(const QString &description)
{
    if (m_sceneDescEdit) {
        m_sceneDescEdit->setPlainText(description);
    }
}

void PanelEditorWidget::setPreviewPixmap(const QPixmap &pixmap)
{
    if (m_previewLabel && !pixmap.isNull()) {
        m_previewLabel->setPixmap(pixmap.scaled(
            Size::PREVIEW_WIDTH, Size::PREVIEW_HEIGHT,
            Qt::KeepAspectRatio, Qt::SmoothTransformation));
        m_previewLabel->setText("");
    }
}

void PanelEditorWidget::setPreviewUrl(const QString &url)
{
    if (url.isEmpty()) {
        if (m_previewLabel) {
            m_previewLabel->setText(Text::PREVIEW);
            m_previewLabel->setPixmap(QPixmap());
        }
        return;
    }
    
    if (url.startsWith("http://") || url.startsWith("https://")) {
        loadPreviewFromUrl(url);
    } else if (url.startsWith("data:image")) {
        QString base64Data = url.section(",", 1);
        QByteArray imageData = QByteArray::fromBase64(base64Data.toLatin1());
        QPixmap pixmap;
        if (pixmap.loadFromData(imageData)) {
            setPreviewPixmap(pixmap);
        }
    } else {
        QPixmap pixmap(url);
        if (!pixmap.isNull()) {
            setPreviewPixmap(pixmap);
        }
    }
}

void PanelEditorWidget::setState(State state)
{
    m_state = state;
    
    if (!m_statusLabel) return;
    
    switch (state) {
        case State::Ready:
            m_statusLabel->setText(Text::TASK_STATUS + Text::PENDING);
            m_statusLabel->setStyleSheet(Style::STATUS_READY);
            if (m_submitEditBtn) m_submitEditBtn->setEnabled(true);
            break;
        case State::Processing:
            m_statusLabel->setText(Text::TASK_STATUS + Text::PROCESSING);
            m_statusLabel->setStyleSheet(Style::STATUS_PROCESSING);
            if (m_submitEditBtn) m_submitEditBtn->setEnabled(false);
            break;
        case State::Success:
            m_statusLabel->setText(Text::TASK_STATUS + Text::SUCCESS);
            m_statusLabel->setStyleSheet(Style::STATUS_SUCCESS);
            if (m_submitEditBtn) m_submitEditBtn->setEnabled(true);
            break;
        case State::Error:
            m_statusLabel->setText(Text::TASK_STATUS + Text::FAILED);
            m_statusLabel->setStyleSheet(Style::STATUS_ERROR);
            if (m_submitEditBtn) m_submitEditBtn->setEnabled(true);
            break;
    }
}

QString PanelEditorWidget::sceneDescription() const
{
    return m_sceneDescEdit ? m_sceneDescEdit->toPlainText().trimmed() : QString();
}

QString PanelEditorWidget::editInstruction() const
{
    return m_editInstructionEdit ? m_editInstructionEdit->toPlainText().trimmed() : QString();
}

QWidget* PanelEditorWidget::createHeader()
{
    QWidget *header = new QWidget();
    header->setStyleSheet(TRANSPARENT_BG);
    QHBoxLayout *layout = new QHBoxLayout(header);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(16);
    
    layout->addWidget(createHeaderPreview());
    layout->addWidget(createHeaderInfo(), 1);
    layout->addStretch();
    layout->addWidget(createCloseButton());
    
    return header;
}

QWidget* PanelEditorWidget::createHeaderPreview()
{
    m_previewLabel = new QLabel();
    m_previewLabel->setFixedSize(Size::PREVIEW_WIDTH, Size::PREVIEW_HEIGHT);
    m_previewLabel->setStyleSheet(Style::PREVIEW_PLACEHOLDER);
    m_previewLabel->setAlignment(Qt::AlignCenter);
    m_previewLabel->setText(Text::PREVIEW);
    return m_previewLabel;
}

QWidget* PanelEditorWidget::createHeaderInfo()
{
    QWidget *info = new QWidget();
    info->setStyleSheet(TRANSPARENT_BG);
    QVBoxLayout *layout = new QVBoxLayout(info);
    layout->setContentsMargins(0, 4, 0, 0);
    layout->setSpacing(6);
    
    m_idLabel = new QLabel(QString::fromUtf8("面板 %1-%2").arg(m_chapterNumber).arg(m_panelNumber));
    m_idLabel->setStyleSheet(Style::PANEL_ID_STYLE);
    layout->addWidget(m_idLabel);
    
    m_posLabel = new QLabel(Text::POSITION + QString("P%1-%2").arg(m_chapterNumber).arg(m_panelNumber));
    m_posLabel->setStyleSheet(Style::PANEL_META_STYLE);
    layout->addWidget(m_posLabel);
    
    QLabel *sceneLabel = new QLabel(QString::fromUtf8("场景：%1").arg(Text::WAIT_GENERATE));
    sceneLabel->setStyleSheet(Style::PANEL_META_STYLE);
    layout->addWidget(sceneLabel);
    layout->addStretch();
    
    return info;
}

QPushButton* PanelEditorWidget::createCloseButton()
{
    QString closeText = QString::fromUtf8("关闭");
    QPushButton *btn = new QPushButton(closeText);
    btn->setFixedSize(32, 32);
    btn->setStyleSheet(Style::CLOSE_BTN_STYLE);
    btn->setCursor(Qt::PointingHandCursor);
    connect(btn, &QPushButton::clicked, this, &PanelEditorWidget::closed);
    return btn;
}

QWidget* PanelEditorWidget::createSceneDescSection()
{
    QWidget *card = new QWidget();
    card->setObjectName("sectionCard");
    card->setStyleSheet(Style::CARD_BG);
    QVBoxLayout *layout = new QVBoxLayout(card);
    layout->setContentsMargins(16, 14, 16, 14);
    layout->setSpacing(10);
    
    QLabel *titleLabel = new QLabel(Text::SCENE_DESC);
    titleLabel->setStyleSheet(Style::SECTION_TITLE);
    layout->addWidget(titleLabel);
    
    m_sceneDescEdit = new QTextEdit();
    m_sceneDescEdit->setStyleSheet(Style::SCENE_EDIT_STYLE);
    m_sceneDescEdit->setFixedHeight(Size::SCENE_DESC_HEIGHT);
    layout->addWidget(m_sceneDescEdit);
    
    QLabel *hintLabel = new QLabel(Text::SCENE_HINT);
    hintLabel->setStyleSheet(Style::HINT_LABEL_STYLE);
    layout->addWidget(hintLabel);
    
    m_saveSceneBtn = new QPushButton(Text::SAVE_SCENE);
    m_saveSceneBtn->setStyleSheet(Style::SAVE_BTN_STYLE);
    m_saveSceneBtn->setCursor(Qt::PointingHandCursor);
    m_saveSceneBtn->setFixedHeight(38);
    connect(m_saveSceneBtn, &QPushButton::clicked, this, &PanelEditorWidget::onSaveSceneClicked);
    layout->addWidget(m_saveSceneBtn);
    
    return card;
}

QWidget* PanelEditorWidget::createEditModeSection()
{
    QWidget *card = new QWidget();
    card->setObjectName("sectionCard");
    card->setStyleSheet(Style::CARD_BG);
    QVBoxLayout *layout = new QVBoxLayout(card);
    layout->setContentsMargins(16, 14, 16, 14);
    layout->setSpacing(10);
    
    QLabel *titleLabel = new QLabel(Text::EDIT_MODE);
    titleLabel->setStyleSheet(Style::SECTION_TITLE);
    layout->addWidget(titleLabel);
    
    QWidget *modeGroup = new QWidget();
    modeGroup->setObjectName("modeGroup");
    modeGroup->setStyleSheet(Style::MODE_GROUP_CONTAINER);
    QHBoxLayout *modeLayout = new QHBoxLayout(modeGroup);
    modeLayout->setContentsMargins(4, 4, 4, 4);
    modeLayout->setSpacing(0);
    
    m_inpaintBtn = createModeButton(Text::INPAINT, static_cast<int>(EditMode::Inpaint));
    m_outpaintBtn = createModeButton(Text::OUTPAINT, static_cast<int>(EditMode::Outpaint));
    m_bgSwapBtn = createModeButton(Text::BG_SWAP, static_cast<int>(EditMode::BgSwap));
    
    m_inpaintBtn->setStyleSheet(Style::MODE_BTN_ACTIVE);
    
    modeLayout->addWidget(m_inpaintBtn);
    modeLayout->addWidget(m_outpaintBtn);
    modeLayout->addWidget(m_bgSwapBtn);
    
    layout->addWidget(modeGroup);
    return card;
}

QWidget* PanelEditorWidget::createInstructionSection()
{
    QWidget *card = new QWidget();
    card->setObjectName("sectionCard");
    card->setStyleSheet(Style::CARD_BG);
    QVBoxLayout *layout = new QVBoxLayout(card);
    layout->setContentsMargins(16, 14, 16, 14);
    layout->setSpacing(10);
    
    QLabel *titleLabel = new QLabel(Text::EDIT_INSTRUCTION);
    titleLabel->setStyleSheet(Style::SECTION_TITLE);
    layout->addWidget(titleLabel);
    
    m_editInstructionEdit = new QTextEdit();
    m_editInstructionEdit->setPlaceholderText(Text::EDIT_HINT);
    m_editInstructionEdit->setStyleSheet(Style::SCENE_EDIT_STYLE);
    m_editInstructionEdit->setFixedHeight(Size::INSTRUCTION_HEIGHT);
    layout->addWidget(m_editInstructionEdit);
    
    layout->addWidget(createMaskFileRow());
    return card;
}

QWidget* PanelEditorWidget::createMaskFileRow()
{
    QWidget *row = new QWidget();
    row->setStyleSheet(TRANSPARENT_BG);
    QHBoxLayout *layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(10);
    
    QLabel *maskLabel = new QLabel(Text::MASK_FILE);
    maskLabel->setStyleSheet(Style::INFO_TEXT);
    maskLabel->setFixedWidth(90);
    layout->addWidget(maskLabel);
    
    m_selectMaskBtn = new QPushButton(Text::SELECT_FILE);
    m_selectMaskBtn->setStyleSheet(Style::MASK_ROW_STYLE);
    m_selectMaskBtn->setCursor(Qt::PointingHandCursor);
    m_selectMaskBtn->setFixedHeight(32);
    connect(m_selectMaskBtn, &QPushButton::clicked, this, &PanelEditorWidget::onSelectMaskFileClicked);
    layout->addWidget(m_selectMaskBtn);
    
    m_maskFileLabel = new QLabel(Text::NOT_SELECTED);
    m_maskFileLabel->setStyleSheet("font-size: 12px; color: #9CA3AF; background: transparent;");
    layout->addWidget(m_maskFileLabel);
    layout->addStretch();
    
    return row;
}

QWidget* PanelEditorWidget::createFooter()
{
    QWidget *footer = new QWidget();
    footer->setStyleSheet(TRANSPARENT_BG);
    QVBoxLayout *layout = new QVBoxLayout(footer);
    layout->setContentsMargins(0, 8, 0, 0);
    layout->setSpacing(12);
    
    m_submitEditBtn = new QPushButton(Text::SUBMIT_EDIT);
    m_submitEditBtn->setStyleSheet(Style::SUBMIT_BTN_STYLE);
    m_submitEditBtn->setCursor(Qt::PointingHandCursor);
    m_submitEditBtn->setFixedHeight(48);
    connect(m_submitEditBtn, &QPushButton::clicked, this, &PanelEditorWidget::onSubmitEditClicked);
    layout->addWidget(m_submitEditBtn);
    
    m_statusLabel = new QLabel(Text::TASK_STATUS + Text::PENDING);
    m_statusLabel->setStyleSheet(Style::STATUS_READY);
    m_statusLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_statusLabel);
    
    return footer;
}

QPushButton* PanelEditorWidget::createModeButton(const QString& text, int mode)
{
    QPushButton *btn = new QPushButton(text);
    btn->setStyleSheet(Style::MODE_BTN_NORMAL);
    btn->setCursor(Qt::PointingHandCursor);
    connect(btn, &QPushButton::clicked, [this, mode]() { onEditModeClicked(mode); });
    return btn;
}

void PanelEditorWidget::updateModeButtons(EditMode activeMode)
{
    QString activeStyle = Style::MODE_BTN_ACTIVE;
    QString normalStyle = Style::MODE_BTN_NORMAL;
    
    if (m_inpaintBtn) m_inpaintBtn->setStyleSheet(activeMode == EditMode::Inpaint ? activeStyle : normalStyle);
    if (m_outpaintBtn) m_outpaintBtn->setStyleSheet(activeMode == EditMode::Outpaint ? activeStyle : normalStyle);
    if (m_bgSwapBtn) m_bgSwapBtn->setStyleSheet(activeMode == EditMode::BgSwap ? activeStyle : normalStyle);
}

void PanelEditorWidget::loadPreviewFromUrl(const QString &url)
{
    if (m_previewLabel) {
        m_previewLabel->setText(Text::LOADING);
    }
    
    QNetworkRequest request{QUrl(url)};
    QNetworkReply *reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onImageDownloadFinished(reply);
    });
}

void PanelEditorWidget::onImageDownloadFinished(QNetworkReply *reply)
{
    reply->deleteLater();
    
    if (reply->error() != QNetworkReply::NoError) {
        if (m_previewLabel) {
            m_previewLabel->setText(Text::LOAD_FAILED);
        }
        return;
    }
    
    QByteArray imageData = reply->readAll();
    QPixmap pixmap;
    if (pixmap.loadFromData(imageData)) {
        setPreviewPixmap(pixmap);
    } else if (m_previewLabel) {
        m_previewLabel->setText(Text::LOAD_FAILED);
    }
}

void PanelEditorWidget::onSaveSceneClicked()
{
    emit sceneDescriptionChanged(sceneDescription());
}

void PanelEditorWidget::onEditModeClicked(int mode)
{
    m_currentEditMode = static_cast<EditMode>(mode);
    updateModeButtons(m_currentEditMode);
}

void PanelEditorWidget::onSubmitEditClicked()
{
    if (m_imageService && m_imageService->isGenerating()) {
        LOG_WARNING("PanelEditorWidget", "已有生成任务在进行中，请等待完成");
        return;
    }
    
    setState(State::Processing);
    emit editSubmitted(static_cast<int>(m_currentEditMode), editInstruction(), m_maskFilePath);
    
    if (m_imageService && !m_panelId.isEmpty()) {
        QtConcurrent::run([this]() {
            m_imageService->generatePanelImage(m_panelId, ImageService::GenerateMode::Preview);
        });
    }
}

void PanelEditorWidget::onSelectMaskFileClicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, Text::SELECT_FILE, "", "Images (*.png *.jpg)");
    if (!fileName.isEmpty()) {
        m_maskFilePath = fileName;
        if (m_maskFileLabel) {
            m_maskFileLabel->setText(Text::SELECTED);
            m_maskFileLabel->setStyleSheet("font-size: 12px; color: #10B981; background: transparent; font-weight: 500;");
        }
    }
}
