#include "pages/NovelUploadPage.h"
#include "viewmodels/NovelViewModel.h"
#include "viewmodels/StoryboardViewModel.h"
#include "services/BibleGenerator.h"
#include "utils/EncodingUtils.h"
#include "utils/Logger.h"
#include "utils/UserSession.h"
#include "utils/NovelUploadUtils.h"
#include "components/EditorStyles.h"
#include "components/ChapterSelectDialog.h"
#include <QFileDialog>
#include <QFileInfo>
#include <QUuid>
#include <QGraphicsDropShadowEffect>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QDateTime>
#include <QTimer>

using namespace EditorStyles;

namespace {
    namespace Size {
        constexpr int INPUT_HEIGHT = 48;
        constexpr int TEXT_EDIT_HEIGHT = 280;
        constexpr int BUTTON_MIN_WIDTH = 160;
    }
    
    namespace Limits {
    constexpr int TEXT_MAX = 6000;
    constexpr int CHAPTER_MIN = 1;
    constexpr int CHAPTER_MAX = 999;
}
    
    using EditorStyles::UI::createTransparentWidget;
    using EditorStyles::UI::createLabel;
    using EditorStyles::UI::createLineEdit;
    
    QTextEdit* createTextEdit(const QString &placeholder, int height)
    {
        QTextEdit *edit = new QTextEdit();
        edit->setPlaceholderText(placeholder);
        edit->setStyleSheet(uploadTextEditStyle());
        edit->setMinimumHeight(height);
        edit->setCursor(Qt::IBeamCursor);
        return edit;
    }
    
    QProgressBar* createProgressBar()
    {
        QProgressBar *bar = new QProgressBar();
        bar->setStyleSheet(uploadProgressBarStyle());
        bar->setTextVisible(false);
        bar->setFixedHeight(6);
        bar->setVisible(false);
        return bar;
    }
    
    QLabel* createMessageLabel(const QString &style)
    {
        QLabel *label = new QLabel();
        label->setStyleSheet(style);
        label->setWordWrap(true);
        label->setVisible(false);
        return label;
    }
    
    void setupLayout(QLayout *layout, int left, int top, int right, int bottom, int spacing)
    {
        layout->setContentsMargins(left, top, right, bottom);
        layout->setSpacing(spacing);
    }
    
    QWidget* createFieldGroup(const QString &labelText, QWidget *inputWidget)
    {
        QWidget *group = createTransparentWidget();
        QVBoxLayout *layout = new QVBoxLayout(group);
        setupLayout(layout, 0, 0, 0, 0, 10);
        layout->addWidget(createLabel(labelText, uploadFormLabelStyle()));
        layout->addWidget(inputWidget);
        return group;
    }
    
    QWidget* createTextFieldGroup(const QString &labelText, QTextEdit *textEdit)
    {
        QWidget *group = createTransparentWidget();
        QVBoxLayout *layout = new QVBoxLayout(group);
        setupLayout(layout, 0, 0, 0, 0, 10);
        
        QWidget *header = createTransparentWidget();
        QHBoxLayout *headerLayout = new QHBoxLayout(header);
        setupLayout(headerLayout, 0, 0, 0, 0, 0);
        headerLayout->addWidget(createLabel(labelText, uploadFormLabelStyle()));
        headerLayout->addStretch();
        headerLayout->addWidget(createLabel(QString::fromUtf8("建议 %1 字以内").arg(Limits::TEXT_MAX), uploadFormHintStyle()));
        
        layout->addWidget(header);
        layout->addWidget(textEdit);
        return group;
    }
}

NovelUploadPage::NovelUploadPage(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
    connectAnalysisSignals();
}

NovelUploadPage::~NovelUploadPage()
{
}

void NovelUploadPage::setupUI()
{
    setObjectName("novelUploadPage");
    setStyleSheet(uploadPageBackgroundStyle());

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    setupLayout(mainLayout, 0, 0, 0, 0, 0);
    mainLayout->addWidget(createScrollArea());
}

void NovelUploadPage::connectAnalysisSignals()
{
    StoryboardViewModel* vm = StoryboardViewModel::instance();

    connect(vm, &StoryboardViewModel::analysisCompleted,
            this, &NovelUploadPage::onAnalysisCompleted);
    connect(vm, &StoryboardViewModel::analysisFailed,
            this, &NovelUploadPage::onAnalysisFailed);
    connect(vm, &StoryboardViewModel::analysisProgress,
            this, [this](const QString& stage, int progress) {
                Q_UNUSED(stage)
                if (m_progressBar && m_isWaitingAnalysis) {
                    int displayProgress = progress;
                    if (!m_analysisCompleted && displayProgress >= 100) {
                        displayProgress = 99;
                    }
                    m_progressBar->setValue(displayProgress);
                }
            });
}

QScrollArea* NovelUploadPage::createScrollArea()
{
    QScrollArea *scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setStyleSheet(uploadScrollAreaStyle());
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    QWidget *contentWidget = createTransparentWidget();
    contentWidget->setObjectName("uploadContent");
    contentWidget->setStyleSheet("#uploadContent { background: transparent; }");

    QVBoxLayout *contentLayout = new QVBoxLayout(contentWidget);
    setupLayout(contentLayout, 40, 40, 40, 60, 0);

    QWidget *centerWrapper = createTransparentWidget();
    QHBoxLayout *centerLayout = new QHBoxLayout(centerWrapper);
    setupLayout(centerLayout, 0, 0, 0, 0, 0);
    centerLayout->addStretch();
    centerLayout->addWidget(createMainCard(), 1);
    centerLayout->addStretch();

    contentLayout->addWidget(centerWrapper);
    contentLayout->addStretch();

    scrollArea->setWidget(contentWidget);
    return scrollArea;
}

QWidget* NovelUploadPage::createMainCard()
{
    QWidget *card = new QWidget();
    card->setObjectName("uploadContainer");
    card->setStyleSheet(uploadCardStyle());
    card->setMinimumWidth(600);
    card->setMaximumWidth(800);
    card->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(card);
    shadow->setBlurRadius(40);
    shadow->setColor(QColor(99, 102, 241, 30));
    shadow->setOffset(0, 10);
    card->setGraphicsEffect(shadow);

    QVBoxLayout *layout = new QVBoxLayout(card);
    setupLayout(layout, 36, 36, 36, 36, 28);
    layout->addWidget(createHeaderSection());
    layout->addWidget(createFormSection());
    layout->addWidget(createActionSection());
    layout->addWidget(createMessageSection());
    layout->addWidget(createTipsSection());

    return card;
}

QWidget* NovelUploadPage::createHeaderSection()
{
    QWidget *header = createTransparentWidget();
    QVBoxLayout *layout = new QVBoxLayout(header);
    setupLayout(layout, 0, 0, 0, 0, 12);

    QWidget *titleRow = createTransparentWidget();
    QHBoxLayout *titleLayout = new QHBoxLayout(titleRow);
    setupLayout(titleLayout, 0, 0, 0, 0, 12);

    QLabel *iconLabel = new QLabel();
    iconLabel->setFixedSize(48, 48);
    iconLabel->setStyleSheet(uploadHeaderIconStyle());
    iconLabel->setText("📝");
    iconLabel->setFont(QFont(QStringLiteral("Segoe UI Emoji"), 24));
    iconLabel->setAlignment(Qt::AlignCenter);

    m_headerTitleLabel = createLabel(QString::fromUtf8("上传小说"), uploadPageTitleStyle());
    QFont titleFont = m_headerTitleLabel->font();
    titleFont.setPointSize(26);
    titleFont.setBold(true);
    m_headerTitleLabel->setFont(titleFont);

    titleLayout->addWidget(iconLabel);
    titleLayout->addWidget(m_headerTitleLabel);
    titleLayout->addStretch();
    
    layout->addWidget(titleRow);
    return header;
}

QWidget* NovelUploadPage::createFormSection()
{
    QWidget *section = createTransparentWidget();
    QVBoxLayout *layout = new QVBoxLayout(section);
    setupLayout(layout, 0, 0, 0, 0, 24);

    m_titleEdit = createLineEdit(QString::fromUtf8("输入小说标题..."), Size::INPUT_HEIGHT);
    connect(m_titleEdit, &QLineEdit::textChanged, this, &NovelUploadPage::validateInput);
    layout->addWidget(createFieldGroup(QString::fromUtf8("小说标题"), m_titleEdit));

    QWidget *metaRow = createTransparentWidget();
    QHBoxLayout *metaLayout = new QHBoxLayout(metaRow);
    setupLayout(metaLayout, 0, 0, 0, 0, 20);

    m_genreEdit = createLineEdit(QString::fromUtf8("如：奇幻、科幻、都市..."), Size::INPUT_HEIGHT);
    metaLayout->addWidget(createFieldGroup(QString::fromUtf8("小说类型"), m_genreEdit), 1);
    metaLayout->addStretch(1);
    layout->addWidget(metaRow);

    m_textEdit = createTextEdit(QString::fromUtf8("在此粘贴小说文本内容...\n\n支持直接粘贴或通过「选择文件」导入 TXT 文件。"), Size::TEXT_EDIT_HEIGHT);
    connect(m_textEdit, &QTextEdit::textChanged, this, &NovelUploadPage::validateInput);
    layout->addWidget(createTextFieldGroup(QString::fromUtf8("小说正文"), m_textEdit));

    return section;
}

QWidget* NovelUploadPage::createActionSection()
{
    QWidget *section = createTransparentWidget();
    QVBoxLayout *layout = new QVBoxLayout(section);
    setupLayout(layout, 0, 8, 0, 0, 16);

    QWidget *buttonRow = createTransparentWidget();
    QHBoxLayout *buttonLayout = new QHBoxLayout(buttonRow);
    setupLayout(buttonLayout, 0, 0, 0, 0, 16);

    m_selectFileBtn = new QPushButton(QString::fromUtf8("📁 选择文件"));
    m_selectFileBtn->setStyleSheet(uploadSecondaryButtonStyle());
    m_selectFileBtn->setFixedHeight(Size::INPUT_HEIGHT);
    m_selectFileBtn->setCursor(Qt::PointingHandCursor);
    connect(m_selectFileBtn, &QPushButton::clicked, this, &NovelUploadPage::onSelectFileClicked);

    m_uploadBtn = new QPushButton(QString::fromUtf8("✅ 上传小说"));
    m_uploadBtn->setEnabled(false);
    m_uploadBtn->setStyleSheet(uploadPrimaryButtonStyle());
    m_uploadBtn->setMinimumWidth(Size::BUTTON_MIN_WIDTH);
    m_uploadBtn->setFixedHeight(Size::INPUT_HEIGHT);
    m_uploadBtn->setCursor(Qt::PointingHandCursor);
    connect(m_uploadBtn, &QPushButton::clicked, this, &NovelUploadPage::onUploadClicked);

    buttonLayout->addWidget(m_selectFileBtn);
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_uploadBtn);

    m_progressBar = createProgressBar();

    layout->addWidget(buttonRow);
    layout->addWidget(m_progressBar);
    return section;
}

QWidget* NovelUploadPage::createMessageSection()
{
    QWidget *section = createTransparentWidget();
    QVBoxLayout *layout = new QVBoxLayout(section);
    setupLayout(layout, 0, 0, 0, 0, 12);
    
    m_errorLabel = createMessageLabel(uploadErrorLabelStyle());
    m_successLabel = createMessageLabel(uploadSuccessLabelStyle());
    m_successLabel->setTextFormat(Qt::RichText);
    m_successLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
    connect(m_successLabel, &QLabel::linkActivated, this, [this](const QString&) {
        if (!m_pendingNovelId.isEmpty()) {
            emit viewDetailsRequested(m_pendingNovelId);
        }
    });

    layout->addWidget(m_errorLabel);
    layout->addWidget(m_successLabel);
    return section;
}

QWidget* NovelUploadPage::createTipsSection()
{
    QWidget *section = new QWidget();
    section->setObjectName("tipsSection");
    section->setStyleSheet(uploadTipsSectionStyle());

    QVBoxLayout *layout = new QVBoxLayout(section);
    setupLayout(layout, 20, 18, 20, 18, 12);

    layout->addWidget(createLabel(QString::fromUtf8("💡 温馨提示"), uploadTipsTitleStyle()));

    QWidget *tipsList = createTransparentWidget();
    QVBoxLayout *tipsLayout = new QVBoxLayout(tipsList);
    setupLayout(tipsLayout, 0, 0, 0, 0, 8);

    const QStringList tips = {
        QString::fromUtf8("上传后可在详情页管理角色"),
        QString::fromUtf8("支持多章节上传，后续可在作品详情中追加内容")
    };
    
    for (const QString &tip : tips) {
        QLabel *tipLabel = createLabel("• " + tip, uploadTipsItemStyle());
        tipLabel->setWordWrap(true);
        tipsLayout->addWidget(tipLabel);
    }

    layout->addWidget(tipsList);
    return section;
}

void NovelUploadPage::validateInput()
{
    using namespace NovelUploadUtils::Validation;
    bool isValid = checkInputValid(m_titleEdit->text(), m_textEdit->toPlainText(), Limits::TEXT_MAX);
    m_uploadBtn->setEnabled(isValid);
}

bool NovelUploadPage::validateFormData(const QString &title, const QString &text)
{
    using namespace NovelUploadUtils::Validation;
    ValidationResult result = checkFormData(title, text, Limits::TEXT_MAX);
    
    if (!result.isValid) {
        showError(result.errorMessage);
        return false;
    }
    
    return true;
}

void NovelUploadPage::onSelectFileClicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, QString::fromUtf8("选择小说文件"), "", QString::fromUtf8("文本文件 (*.txt);;所有文件 (*.*)"));
    
    if (fileName.isEmpty()) return;

    const QString content = EncodingUtils::readFile(fileName);
    if (content.isEmpty()) {
        showError(QString::fromUtf8("无法读取文件或文件内容为空"));
        return;
    }

    m_textEdit->setPlainText(content);

    if (m_titleEdit->text().isEmpty()) {
        m_titleEdit->setText(QFileInfo(fileName).completeBaseName());
    }
}

void NovelUploadPage::onUploadClicked()
{
    QString title = m_titleEdit->text().trimmed();
    QString text = m_textEdit->toPlainText().trimmed();
    QString genre = m_genreEdit->text().trimmed();

    if (!validateFormData(title, text)) return;

    clearMessages();
    setButtonLoadingState(true);

    int chapterNumber = showChapterNumberDialog();
    if (chapterNumber <= 0) {
        setButtonLoadingState(false);
        return;
    }
    
    bool saveSuccess = m_isEditMode 
        ? updateExistingNovel(title, text, genre)
        : createNewNovel(title, text, genre, chapterNumber);
    
    if (!saveSuccess) {
        showError(getErrorMessage());
        setButtonLoadingState(false);
        return;
    }
    
    startAnalysis(chapterNumber, text);
}

bool NovelUploadPage::updateExistingNovel(const QString &title, const QString &text, const QString &genre)
{
    QVariantMap updateData;
    updateData["title"] = title;
    updateData["original_text"] = text;
    QVariantMap metadata = NovelViewModel::instance()->currentNovel().metadata();
    if (!genre.trimmed().isEmpty()) {
        metadata["genre"] = genre.trimmed();
    }
    metadata["word_count"] = text.length();
    updateData["metadata"] = QJsonDocument(QJsonObject::fromVariantMap(metadata)).toJson();
    
    return NovelViewModel::instance()->updateNovel(m_editNovelId, updateData);
}

bool NovelUploadPage::createNewNovel(const QString& title, const QString& text, const QString& genre, int chapterNumber)
{
    NovelViewModel* vm = NovelViewModel::instance();
    vm->createNovel(UserSession::instance()->currentUserId(), title, text, buildMetadata(genre));
    
    Novel novel = vm->currentNovel();
    if (novel.id().isEmpty()) return false;
    
    m_editNovelId = novel.id();
    if (!StoryboardViewModel::instance()->createEmptyStoryboard(novel.id(), chapterNumber)) {
        vm->deleteNovel(novel.id());
        m_editNovelId.clear();
        return false;
    }
    
    return true;
}

QVariantMap NovelUploadPage::buildMetadata(const QString &genre)
{
    using namespace NovelUploadUtils::Metadata;
    return buildNovelMetadata(genre, 0);
}

void NovelUploadPage::startAnalysis(int chapterNumber, const QString& text)
{
    setWaitingAnalysisState();
    
    QJsonArray existingCharacters = BibleGenerator::instance()->collectExistingCharacters(m_editNovelId);
    QJsonArray existingScenes = BibleGenerator::instance()->collectExistingScenes(m_editNovelId);
    
    StoryboardViewModel::instance()->startAnalysisWithBible(m_editNovelId, text, chapterNumber, existingCharacters, existingScenes);
}

void NovelUploadPage::setWaitingAnalysisState()
{
    using namespace NovelUploadUtils::State;
    m_isWaitingAnalysis = true;
    m_analysisCompleted = false;
    m_pendingNovelId = m_editNovelId;
    applyWaitingAnalysisState(m_uploadBtn, m_progressBar, m_successLabel);
    m_successLabel->setStyleSheet(uploadWaitingLabelStyle());
}

void NovelUploadPage::resetAfterAnalysis()
{
    using namespace NovelUploadUtils::State;
    m_isWaitingAnalysis = false;
    applyResetAfterAnalysis(m_uploadBtn, m_progressBar);
}

void NovelUploadPage::setButtonLoadingState(bool loading)
{
    using namespace NovelUploadUtils::State;
    applyLoadingState(m_uploadBtn, loading, m_isEditMode);
}

int NovelUploadPage::showChapterNumberDialog()
{
    return ChapterSelectDialog::selectChapter(this, 1, Limits::CHAPTER_MIN, Limits::CHAPTER_MAX);
}

void NovelUploadPage::showMessage(const QString &message, bool isError)
{
    using namespace NovelUploadUtils::Message;
    displayMessage(m_errorLabel, m_successLabel, message, isError);
}

void NovelUploadPage::showError(const QString &message)
{
    using namespace NovelUploadUtils::Message;
    displayError(m_errorLabel, m_successLabel, message);
}

void NovelUploadPage::showSuccess(const QString &novelId)
{
    using namespace NovelUploadUtils::Message;
    QString message = novelId.isEmpty() 
        ? successMessageText(m_isEditMode) 
        : QString("%1 作品ID: %2").arg(successMessageText(m_isEditMode), novelId);
    displaySuccess(m_errorLabel, m_successLabel, message);
    resetToCreateMode();
}

void NovelUploadPage::clearMessages()
{
    using namespace NovelUploadUtils::Message;
    clearAllMessages(m_errorLabel, m_successLabel);
}

void NovelUploadPage::setNovelForEdit(const Novel &novel)
{
    m_isEditMode = true;
    m_editNovelId = novel.id();
    
    m_headerTitleLabel->setText(QString::fromUtf8("编辑小说"));
    m_uploadBtn->setText(QString::fromUtf8("保存修改"));
    
    m_titleEdit->setText(novel.title());
    m_genreEdit->setText(novel.metadata().value("genre").toString());
    m_textEdit->setPlainText(novel.originalText());
    
    validateInput();
    clearMessages();
}

void NovelUploadPage::resetToCreateMode()
{
    m_isEditMode = false;
    m_editNovelId.clear();
    
    m_headerTitleLabel->setText(QString::fromUtf8("上传小说"));
    m_uploadBtn->setText(QString::fromUtf8("✅ 上传小说"));
    
    m_titleEdit->clear();
    m_genreEdit->clear();
    m_textEdit->clear();
    clearMessages();
}

bool NovelUploadPage::hasActiveProgressState() const
{
    return m_isWaitingAnalysis
        || (m_progressBar && m_progressBar->isVisible())
        || (m_successLabel && m_successLabel->isVisible())
        || (m_errorLabel && m_errorLabel->isVisible());
}

bool NovelUploadPage::isCurrentAnalysis(const QString& novelId) const
{
    return m_isWaitingAnalysis && novelId == m_editNovelId;
}

void NovelUploadPage::onAnalysisCompleted(const QString& novelId, int chapter)
{
    if (!isCurrentAnalysis(novelId)) return;
    
    m_analysisCompleted = true;
    resetAfterAnalysis();

    QTimer::singleShot(500, this, [this]() {
        m_progressBar->setVisible(false);
        m_progressBar->setValue(0);
    });

    m_successLabel->setText(QString::fromUtf8("作品创建成功！ <a href='#' style='color: #4d7c0f; text-decoration: none;'>查看详情</a>"));
    m_successLabel->setTextFormat(Qt::RichText);
    m_successLabel->setStyleSheet(uploadSuccessLabelStyle());
    m_successLabel->setVisible(true);
    emit novelUploaded(novelId, chapter);
}

void NovelUploadPage::onAnalysisFailed(const QString& novelId, const QString& error)
{
    if (!isCurrentAnalysis(novelId)) return;
    
    resetAfterAnalysis();
    showError(QString::fromUtf8("分析小说失败: %1").arg(error));
}

QString NovelUploadPage::getSuccessMessage() const
{
    using namespace NovelUploadUtils::Message;
    return successMessageText(m_isEditMode);
}

QString NovelUploadPage::getErrorMessage() const
{
    using namespace NovelUploadUtils::Message;
    return errorMessageText(m_isEditMode);
}
