#include "components/PanelEditorWidget.h"
#include "components/EditorStyles.h"
#include "services/ImageService.h"
#include "utils/Logger.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTextEdit>
#include <QPushButton>
#include <QFileDialog>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QtConcurrent>

namespace {
    constexpr int DIALOG_WIDTH = 860;
    constexpr int DIALOG_HEIGHT = 660;
    constexpr int PREVIEW_WIDTH = 240;
    constexpr int PREVIEW_HEIGHT = 320;
    constexpr int SCENE_DESC_HEIGHT = 120;
    constexpr int INSTRUCTION_HEIGHT = 100;

    QString sectionCardStyle(const QString& objectName)
    {
        return QStringLiteral(
            "#%1 { background: #F9FAFB; border-radius: 12px; border: 1px solid #E5E7EB; }"
        ).arg(objectName);
    }

    QString modeButtonStyle(bool active)
    {
        return active ? QStringLiteral(R"(
            QPushButton {
                background: #ffffff;
                color: #4d7c0f;
                font-size: 12px;
                font-weight: 600;
                border: none;
                border-radius: 6px;
                padding: 6px 12px;
            }
        )") : QStringLiteral(R"(
            QPushButton {
                background: transparent;
                color: #6B7280;
                font-size: 12px;
                font-weight: 500;
                border: none;
                border-radius: 6px;
                padding: 6px 12px;
            }
            QPushButton:hover {
                background: rgba(255,255,255,0.5);
                color: #374151;
            }
        )");
    }

    void applyTextEditStyle(QTextEdit* edit)
    {
        if (!edit) {
            return;
        }

        edit->setStyleSheet(R"(
            QTextEdit {
                padding: 12px;
                font-size: 13px;
                border: 1px solid #E5E7EB;
                border-radius: 8px;
                background: #ffffff;
                color: #374151;
            }
            QTextEdit:focus {
                border-color: #84cc16;
            }
        )");
    }

    void applyModeButtonStyle(QPushButton* button, bool active)
    {
        if (!button) {
            return;
        }

        button->setStyleSheet(modeButtonStyle(active));
    }

    void updateStatusLabel(QLabel* label, PanelEditorWidget::State state)
    {
        if (!label) {
            return;
        }

        switch (state) {
            case PanelEditorWidget::State::Ready:
                label->setText(QString::fromUtf8("任务状态: 等待中"));
                label->setStyleSheet("font-size: 12px; color: #9CA3AF; background: transparent;");
                break;
            case PanelEditorWidget::State::Processing:
                label->setText(QString::fromUtf8("任务状态: 处理中"));
                label->setStyleSheet("font-size: 12px; color: #F59E0B; background: transparent; font-weight: 600;");
                break;
            case PanelEditorWidget::State::Success:
                label->setText(QString::fromUtf8("任务状态: 成功"));
                label->setStyleSheet("font-size: 12px; color: #10B981; background: transparent; font-weight: 600;");
                break;
            case PanelEditorWidget::State::Error:
                label->setText(QString::fromUtf8("任务状态: 失败"));
                label->setStyleSheet("font-size: 12px; color: #EF4444; background: transparent; font-weight: 600;");
                break;
        }
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
    setFixedSize(DIALOG_WIDTH, DIALOG_HEIGHT);
    setStyleSheet(R"(
        #panelEditorWidget {
            background: #ffffff;
            border-radius: 16px;
            border: 1px solid #E8E8F0;
        }
    )");
    
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24, 20, 24, 24);
    mainLayout->setSpacing(14);

    QWidget *topRow = new QWidget();
    topRow->setStyleSheet("background: transparent;");
    QHBoxLayout *topLayout = new QHBoxLayout(topRow);
    topLayout->setContentsMargins(0, 0, 0, 0);
    topLayout->setSpacing(16);
    
    m_previewLabel = new QLabel();
    m_previewLabel->setFixedSize(PREVIEW_WIDTH, PREVIEW_HEIGHT);
    m_previewLabel->setStyleSheet(R"(
        background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #F3F4F6, stop:1 #E5E7EB);
        border-radius: 12px;
        border: 2px dashed #D1D5DB;
        color: #9CA3AF;
        font-size: 13px;
    )");
    m_previewLabel->setAlignment(Qt::AlignCenter);
    m_previewLabel->setText(QString::fromUtf8("预览"));
    topLayout->addWidget(m_previewLabel);
    
    QWidget *infoWidget = new QWidget();
    infoWidget->setStyleSheet("background: transparent;");
    QVBoxLayout *infoLayout = new QVBoxLayout(infoWidget);
    infoLayout->setContentsMargins(0, 4, 0, 0);
    infoLayout->setSpacing(6);
    
    m_idLabel = new QLabel(QString::fromUtf8("面板 0-0"));
    m_idLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #1F2937; background: transparent;");
    infoLayout->addWidget(m_idLabel);
    
    m_posLabel = new QLabel(QString::fromUtf8("位置: P0-0"));
    m_posLabel->setStyleSheet("font-size: 13px; color: #6B7280; background: transparent;");
    infoLayout->addWidget(m_posLabel);
    
    QLabel *sceneLabel = new QLabel(QString::fromUtf8("场景: 待生成"));
    sceneLabel->setStyleSheet("font-size: 13px; color: #6B7280; background: transparent;");
    infoLayout->addWidget(sceneLabel);
    infoLayout->addStretch();
    
    topLayout->addWidget(infoWidget, 1);
    
    mainLayout->addWidget(topRow);

    QWidget *bodyRow = new QWidget();
    bodyRow->setStyleSheet("background: transparent;");
    QHBoxLayout *bodyLayout = new QHBoxLayout(bodyRow);
    bodyLayout->setContentsMargins(0, 0, 0, 0);
    bodyLayout->setSpacing(14);
    
    QWidget *leftColumn = new QWidget();
    leftColumn->setStyleSheet("background: transparent;");
    QVBoxLayout *leftLayout = new QVBoxLayout(leftColumn);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(10);
    leftLayout->addWidget(createSceneDescSection());
    leftLayout->addStretch();
    bodyLayout->addWidget(leftColumn, 1);
    
    QWidget *rightColumn = new QWidget();
    rightColumn->setStyleSheet("background: transparent;");
    QVBoxLayout *rightLayout = new QVBoxLayout(rightColumn);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(10);
    rightLayout->addWidget(createEditModeSection());
    rightLayout->addWidget(createInstructionSection());
    rightLayout->addWidget(createFooter());
    rightLayout->addStretch();
    bodyLayout->addWidget(rightColumn, 1);
    
    mainLayout->addWidget(bodyRow, 1);
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
        m_posLabel->setText(QString::fromUtf8("位置: P%1-%2").arg(m_chapterNumber).arg(m_panelNumber));
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
            PREVIEW_WIDTH, PREVIEW_HEIGHT,
            Qt::KeepAspectRatio, Qt::SmoothTransformation));
        m_previewLabel->setText("");
        m_previewLabel->setStyleSheet("background: transparent; border: none;");
    }
}

void PanelEditorWidget::setPreviewUrl(const QString &url)
{
    if (url.isEmpty()) {
        if (m_previewLabel) {
            m_previewLabel->setText(QString::fromUtf8("预览"));
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

    updateStatusLabel(m_statusLabel, state);
    if (m_submitEditBtn) {
        m_submitEditBtn->setEnabled(state != State::Processing);
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

QWidget* PanelEditorWidget::createSceneDescSection()
{
    QWidget *card = new QWidget();
    card->setObjectName("sceneCard");
    card->setStyleSheet(sectionCardStyle("sceneCard"));
    QVBoxLayout *layout = new QVBoxLayout(card);
    layout->setContentsMargins(16, 14, 16, 14);
    layout->setSpacing(10);
    
    QLabel *titleLabel = new QLabel(QString::fromUtf8("场景描述"));
    titleLabel->setStyleSheet("font-size: 13px; font-weight: 600; color: #374151; background: transparent;");
    layout->addWidget(titleLabel);
    
    m_sceneDescEdit = new QTextEdit();
    applyTextEditStyle(m_sceneDescEdit);
    m_sceneDescEdit->setFixedHeight(SCENE_DESC_HEIGHT);
    layout->addWidget(m_sceneDescEdit);
    
    QLabel *hintLabel = new QLabel(QString::fromUtf8("用于生成当前分镜画面"));
    hintLabel->setStyleSheet("font-size: 11px; color: #9CA3AF; background: transparent;");
    layout->addWidget(hintLabel);
    
    m_saveSceneBtn = new QPushButton(QString::fromUtf8("保存场景"));
    m_saveSceneBtn->setStyleSheet(R"(
        QPushButton {
            padding: 8px 16px;
            font-size: 13px;
            font-weight: 600;
            background: white;
            color: #4d7c0f;
            border: 1px solid #84cc16;
            border-radius: 8px;
        }
        QPushButton:hover {
            background: #F7FEE7;
        }
    )");
    m_saveSceneBtn->setCursor(Qt::PointingHandCursor);
    m_saveSceneBtn->setFixedHeight(36);
    connect(m_saveSceneBtn, &QPushButton::clicked, this, &PanelEditorWidget::onSaveSceneClicked);
    layout->addWidget(m_saveSceneBtn);
    
    return card;
}

QWidget* PanelEditorWidget::createEditModeSection()
{
    QWidget *card = new QWidget();
    card->setObjectName("modeCard");
    card->setStyleSheet(sectionCardStyle("modeCard"));
    QVBoxLayout *layout = new QVBoxLayout(card);
    layout->setContentsMargins(16, 14, 16, 14);
    layout->setSpacing(10);
    
    QLabel *titleLabel = new QLabel(QString::fromUtf8("编辑模式"));
    titleLabel->setStyleSheet("font-size: 13px; font-weight: 600; color: #374151; background: transparent;");
    layout->addWidget(titleLabel);
    
    QWidget *modeGroup = new QWidget();
    modeGroup->setObjectName("modeGroup");
    modeGroup->setStyleSheet(R"(
        #modeGroup {
            background: #F3F4F6;
            border-radius: 8px;
        }
    )");
    QHBoxLayout *modeLayout = new QHBoxLayout(modeGroup);
    modeLayout->setContentsMargins(4, 4, 4, 4);
    modeLayout->setSpacing(0);
    
    m_inpaintBtn = createModeButton(QString::fromUtf8("局部重绘"), static_cast<int>(EditMode::Inpaint));
    m_outpaintBtn = createModeButton(QString::fromUtf8("扩展绘制"), static_cast<int>(EditMode::Outpaint));
    m_bgSwapBtn = createModeButton(QString::fromUtf8("背景替换"), static_cast<int>(EditMode::BgSwap));
    applyModeButtonStyle(m_inpaintBtn, true);
    applyModeButtonStyle(m_outpaintBtn, false);
    applyModeButtonStyle(m_bgSwapBtn, false);
    
    modeLayout->addWidget(m_inpaintBtn);
    modeLayout->addWidget(m_outpaintBtn);
    modeLayout->addWidget(m_bgSwapBtn);
    
    layout->addWidget(modeGroup);
    return card;
}

QWidget* PanelEditorWidget::createInstructionSection()
{
    QWidget *card = new QWidget();
    card->setObjectName("instrCard");
    card->setStyleSheet(sectionCardStyle("instrCard"));
    QVBoxLayout *layout = new QVBoxLayout(card);
    layout->setContentsMargins(16, 14, 16, 14);
    layout->setSpacing(10);
    
    QLabel *titleLabel = new QLabel(QString::fromUtf8("编辑指令"));
    titleLabel->setStyleSheet("font-size: 13px; font-weight: 600; color: #374151; background: transparent;");
    layout->addWidget(titleLabel);
    
    m_editInstructionEdit = new QTextEdit();
    m_editInstructionEdit->setPlaceholderText(QString::fromUtf8("输入修改要求..."));
    applyTextEditStyle(m_editInstructionEdit);
    m_editInstructionEdit->setFixedHeight(INSTRUCTION_HEIGHT);
    layout->addWidget(m_editInstructionEdit);
    
    layout->addWidget(createMaskFileRow());
    return card;
}

QWidget* PanelEditorWidget::createMaskFileRow()
{
    QWidget *row = new QWidget();
    row->setStyleSheet("background: transparent;");
    QHBoxLayout *layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(10);
    
    QLabel *maskLabel = new QLabel(QString::fromUtf8("遮罩文件:"));
    maskLabel->setStyleSheet("font-size: 12px; color: #6B7280; background: transparent;");
    layout->addWidget(maskLabel);
    
    m_selectMaskBtn = new QPushButton(QString::fromUtf8("选择文件"));
    m_selectMaskBtn->setStyleSheet(R"(
        QPushButton {
            padding: 4px 12px;
            font-size: 12px;
            border: 1px solid #E5E7EB;
            border-radius: 6px;
            background: white;
            color: #4B5563;
        }
        QPushButton:hover {
            border-color: #84cc16;
            background: #F7FEE7;
        }
    )");
    m_selectMaskBtn->setCursor(Qt::PointingHandCursor);
    connect(m_selectMaskBtn, &QPushButton::clicked, this, &PanelEditorWidget::onSelectMaskFileClicked);
    layout->addWidget(m_selectMaskBtn);
    
    m_maskFileLabel = new QLabel(QString::fromUtf8("未选择"));
    m_maskFileLabel->setStyleSheet("font-size: 12px; color: #9CA3AF; background: transparent;");
    layout->addWidget(m_maskFileLabel);
    layout->addStretch();
    
    return row;
}

QWidget* PanelEditorWidget::createFooter()
{
    QWidget *footer = new QWidget();
    footer->setStyleSheet("background: transparent;");
    QVBoxLayout *layout = new QVBoxLayout(footer);
    layout->setContentsMargins(0, 8, 0, 0);
    layout->setSpacing(10);
    
    m_submitEditBtn = new QPushButton(QString::fromUtf8("提交编辑"));
    m_submitEditBtn->setStyleSheet(R"(
        QPushButton {
            padding: 12px 24px;
            font-size: 14px;
            font-weight: bold;
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #84cc16, stop:1 #65a30d);
            color: white;
            border: none;
            border-radius: 10px;
        }
        QPushButton:hover {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #a3e635, stop:1 #84cc16);
        }
        QPushButton:disabled {
            background: #D1D5DB;
            color: #9CA3AF;
        }
    )");
    m_submitEditBtn->setCursor(Qt::PointingHandCursor);
    m_submitEditBtn->setFixedHeight(44);
    connect(m_submitEditBtn, &QPushButton::clicked, this, &PanelEditorWidget::onSubmitEditClicked);
    layout->addWidget(m_submitEditBtn);
    
    m_statusLabel = new QLabel(QString::fromUtf8("任务状态: 等待中"));
    m_statusLabel->setStyleSheet("font-size: 12px; color: #9CA3AF; background: transparent;");
    m_statusLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_statusLabel);
    
    return footer;
}

QPushButton* PanelEditorWidget::createModeButton(const QString& text, int mode)
{
    QPushButton *btn = new QPushButton(text);
    btn->setStyleSheet(modeButtonStyle(false));
    btn->setCursor(Qt::PointingHandCursor);
    connect(btn, &QPushButton::clicked, [this, mode]() { onEditModeClicked(mode); });
    return btn;
}

void PanelEditorWidget::updateModeButtons(EditMode activeMode)
{
    applyModeButtonStyle(m_inpaintBtn, activeMode == EditMode::Inpaint);
    applyModeButtonStyle(m_outpaintBtn, activeMode == EditMode::Outpaint);
    applyModeButtonStyle(m_bgSwapBtn, activeMode == EditMode::BgSwap);
}

void PanelEditorWidget::loadPreviewFromUrl(const QString &url)
{
    if (m_previewLabel) {
        m_previewLabel->setText(QString::fromUtf8("加载中..."));
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
            m_previewLabel->setText(QString::fromUtf8("加载失败"));
        }
        return;
    }
    
    QByteArray imageData = reply->readAll();
    QPixmap pixmap;
    if (pixmap.loadFromData(imageData)) {
        setPreviewPixmap(pixmap);
    } else if (m_previewLabel) {
        m_previewLabel->setText(QString::fromUtf8("加载失败"));
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
    QString fileName = QFileDialog::getOpenFileName(this, QString::fromUtf8("选择文件"), "", "Images (*.png *.jpg)");
    if (!fileName.isEmpty()) {
        m_maskFilePath = fileName;
        if (m_maskFileLabel) {
            m_maskFileLabel->setText(QString::fromUtf8("已选择"));
            m_maskFileLabel->setStyleSheet("font-size: 12px; color: #10B981; background: transparent; font-weight: 500;");
        }
    }
}
