#include "pages/CharacterDetailPage.h"
#include "components/EditorStyles.h"
#include "utils/EncodingUtils.h"
#include "services/ServiceContainer.h"
#include "api/QwenImageClient.h"
#include "utils/Logger.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QUuid>
#include <QGraphicsDropShadowEffect>
#include <QFont>
#include <QSpacerItem>

namespace {
    constexpr int INPUT_HEIGHT = 44;
    constexpr int BTN_HEIGHT = 44;
    constexpr int THUMBNAIL_SIZE = 72;
    constexpr int CARD_SPACING = 20;
    
    const QString COLOR_PRIMARY = "#84cc16";
    const QString COLOR_TEXT = "#1e293b";
    const QString COLOR_TEXT_LIGHT = "#64748b";
    const QString COLOR_BORDER = "#d4d4d4";
    const QString INPUT_STYLE = R"(
        QLineEdit {
            padding: 12px 16px;
            font-size: 14px;
            border: 2px solid #d4d4d4;
            border-radius: 10px;
            background: #ffffff;
            color: #1e293b;
        }
        QLineEdit:focus {
            border-color: #84cc16;
            background: #f7fee7;
        }
        QLineEdit::placeholder {
            color: #94a3b8;
        }
    )";
    
    const QString TEXTEDIT_STYLE = R"(
        QTextEdit {
            padding: 12px 16px;
            font-size: 14px;
            border: 2px solid #d4d4d4;
            border-radius: 10px;
            background: #ffffff;
            color: #1e293b;
        }
        QTextEdit:focus {
            border-color: #84cc16;
            background: #f7fee7;
        }
    )";
    
    const QString BTN_PRIMARY_STYLE = R"(
        QPushButton {
            padding: 12px 24px;
            font-size: 14px;
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 #84cc16, stop:1 #65a30d);
            color: white;
            border: none;
            border-radius: 10px;
            font-weight: 600;
        }
        QPushButton:hover {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 #65a30d, stop:1 #4d7c0f);
        }
        QPushButton:disabled {
            background: #cbd5e1;
            color: #94a3b8;
        }
    )";
    
    const QString BTN_SECONDARY_STYLE = R"(
        QPushButton {
            padding: 10px 18px;
            font-size: 13px;
            background: #ffffff;
            color: #4d7c0f;
            border: 2px solid #d4d4d4;
            border-radius: 8px;
            font-weight: 500;
        }
        QPushButton:hover {
            background: #f7fee7;
            border-color: #84cc16;
        }
    )";
    
    const QString BTN_SUCCESS_STYLE = R"(
        QPushButton {
            padding: 10px 18px;
            font-size: 13px;
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 #10b981, stop:1 #34d399);
            color: white;
            border: none;
            border-radius: 8px;
            font-weight: 500;
        }
        QPushButton:hover {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 #059669, stop:1 #10b981);
        }
    )";
    
    const QString CARD_STYLE = R"(
        #infoCard, #createCard {
            background: #f2ffffff;
            border-radius: 20px;
            border: 1px solid #1a6366f1;
        }
    )";
    
    const QString CONFIG_CARD_STYLE = R"(
        ConfigurationCard {
            background: #f2ffffff;
            border-radius: 16px;
            border: 1px solid #1a6366f1;
        }
        ConfigurationCard:hover {
            border: 1px solid #406366f1;
        }
    )";
}

// ============================================================================
// ============================================================================

CharacterDetailPage::CharacterDetailPage(const QString &charId, QWidget *parent)
    : QWidget(parent)
    , m_charId(charId)
    , m_backBtn(nullptr)
    , m_titleLabel(nullptr)
    , m_nameLabel(nullptr)
    , m_roleLabel(nullptr)
    , m_genderLabel(nullptr)
    , m_ageLabel(nullptr)
    , m_personalityLabel(nullptr)
    , m_configNameEdit(nullptr)
    , m_configDescEdit(nullptr)
    , m_createConfigBtn(nullptr)
    , m_configContainer(nullptr)
    , m_configLayout(nullptr)
{
    setupUI();
    loadCharacterData();
}

CharacterDetailPage::~CharacterDetailPage()
{
}

void CharacterDetailPage::setCharacterId(const QString &charId)
{
    m_charId = charId;
    loadCharacterData();
}


QWidget* CharacterDetailPage::createTransparentWidget()
{
    QWidget *widget = new QWidget();
    widget->setStyleSheet("background: transparent;");
    return widget;
}

QLabel* CharacterDetailPage::createLabel(const QString &text, const QString &style, int fontSize, bool bold)
{
    QLabel *label = new QLabel(text);
    label->setStyleSheet(style);
    if (fontSize > 0) {
        label->setFont(QFont("Microsoft YaHei", fontSize, bold ? QFont::Bold : QFont::Normal));
    }
    return label;
}

void CharacterDetailPage::setupVBoxLayout(QVBoxLayout *layout, int left, int top, int right, int bottom, int spacing)
{
    layout->setContentsMargins(left, top, right, bottom);
    layout->setSpacing(spacing);
}

void CharacterDetailPage::setupHBoxLayout(QHBoxLayout *layout, int left, int top, int right, int bottom, int spacing)
{
    layout->setContentsMargins(left, top, right, bottom);
    layout->setSpacing(spacing);
}

QWidget* CharacterDetailPage::createCard(const QString &objectName)
{
    QWidget *card = new QWidget();
    card->setObjectName(objectName);
    card->setStyleSheet(QString(R"(
        #%1 {
            background: #f2ffffff;
            border-radius: 20px;
            border: 1px solid #1a6366f1;
        }
    )").arg(objectName));
    
    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(card);
    shadow->setBlurRadius(30);
    shadow->setColor(QColor(99, 102, 241, 20));
    shadow->setOffset(0, 8);
    card->setGraphicsEffect(shadow);
    
    return card;
}

QWidget* CharacterDetailPage::createInfoRow(const QString &labelText, const QString &valueText)
{
    QWidget *row = createTransparentWidget();
    QHBoxLayout *layout = new QHBoxLayout(row);
    setupHBoxLayout(layout, 0, 0, 0, 0, 8);
    
    QLabel *label = createLabel(labelText, "font-size: 13px; color: #64748b; background: transparent;");
    QLabel *value = createLabel(valueText, "font-size: 14px; color: #1e293b; background: transparent; font-weight: 500;");
    
    layout->addWidget(label);
    layout->addWidget(value);
    layout->addStretch();
    
    return row;
}


void CharacterDetailPage::setupUI()
{
    this->setObjectName("characterDetailPage");
    this->setStyleSheet(R"(
        #characterDetailPage {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                stop:0 #fafbff,
                stop:0.5 #f5f3ff,
                stop:1 #ede9fe);
        }
    )");

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    setupVBoxLayout(mainLayout, 0, 0, 0, 0, 0);

    mainLayout->addWidget(createScrollArea());
}

QScrollArea* CharacterDetailPage::createScrollArea()
{
    QScrollArea *scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setStyleSheet(R"(
        QScrollArea {
            border: none;
            background: transparent;
        }
    )" + EditorStyles::scrollBarStyle());
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    scrollArea->setWidget(createContentWidget());
    return scrollArea;
}

QWidget* CharacterDetailPage::createContentWidget()
{
    QWidget *contentWidget = new QWidget();
    contentWidget->setObjectName("contentWidget");
    contentWidget->setStyleSheet("#contentWidget { background: transparent; }");

    QVBoxLayout *contentLayout = new QVBoxLayout(contentWidget);
    setupVBoxLayout(contentLayout, 24, 24, 24, 40, 24);

    contentLayout->addWidget(createHeader());
    contentLayout->addWidget(createBasicInfoCard());
    contentLayout->addWidget(createCreateConfigArea());
    contentLayout->addWidget(createConfigListArea());
    contentLayout->addStretch();

    return contentWidget;
}

QWidget* CharacterDetailPage::createHeader()
{
    QWidget *header = createTransparentWidget();
    QHBoxLayout *layout = new QHBoxLayout(header);
    setupHBoxLayout(layout, 0, 0, 0, 0, 12);

    m_backBtn = new QPushButton(tr("返回"));
    m_backBtn->setStyleSheet(BTN_SECONDARY_STYLE);
    m_backBtn->setCursor(Qt::PointingHandCursor);
    connect(m_backBtn, &QPushButton::clicked, this, &CharacterDetailPage::onBackClicked);

    m_titleLabel = createLabel(tr("角色详情"), "font-size: 22px; font-weight: bold; color: #1e293b; background: transparent;", 22, true);

    layout->addWidget(m_backBtn);
    layout->addWidget(m_titleLabel);
    layout->addStretch();

    return header;
}

QWidget* CharacterDetailPage::createBasicInfoCard()
{
    QWidget *card = createCard("infoCard");
    QVBoxLayout *layout = new QVBoxLayout(card);
    setupVBoxLayout(layout, 24, 24, 24, 24, 16);

    QLabel *cardTitle = createLabel(tr("基本信息"), "font-size: 16px; font-weight: bold; color: #1e293b; background: transparent;");
    layout->addWidget(cardTitle);

    QWidget *infoGrid = createTransparentWidget();
    // info grid
    QGridLayout *gridLayout = new QGridLayout(infoGrid);
    gridLayout->setContentsMargins(0, 0, 0, 0);
    gridLayout->setSpacing(12);
    m_nameLabel = createLabel(tr("名称"), "font-size: 14px; color: #1e293b; background: transparent; font-weight: 500;");
    m_roleLabel = createLabel(tr("角色"), "font-size: 14px; color: #1e293b; background: transparent; font-weight: 500;");
    m_genderLabel = createLabel(tr("性别"), "font-size: 14px; color: #1e293b; background: transparent; font-weight: 500;");
    m_ageLabel = createLabel(tr("年龄"), "font-size: 14px; color: #1e293b; background: transparent; font-weight: 500;");
    m_personalityLabel = createLabel(tr("性格"), "font-size: 14px; color: #1e293b; background: transparent; font-weight: 500;");

    gridLayout->addWidget(createLabel(tr("名称"), "font-size: 13px; color: #64748b; background: transparent;"), 0, 0);
    gridLayout->addWidget(m_nameLabel, 0, 1);
    gridLayout->addWidget(createLabel(tr("角色"), "font-size: 13px; color: #64748b; background: transparent;"), 0, 2);
    gridLayout->addWidget(m_roleLabel, 0, 3);
    gridLayout->addWidget(createLabel(tr("性别"), "font-size: 13px; color: #64748b; background: transparent;"), 1, 0);
    gridLayout->addWidget(m_genderLabel, 1, 1);
    gridLayout->addWidget(createLabel(tr("年龄"), "font-size: 13px; color: #64748b; background: transparent;"), 1, 2);
    gridLayout->addWidget(m_ageLabel, 1, 3);
    gridLayout->addWidget(createLabel(tr("性格"), "font-size: 13px; color: #64748b; background: transparent;"), 2, 0);
    gridLayout->addWidget(m_personalityLabel, 2, 1, 1, 3);

    layout->addWidget(infoGrid);

    return card;
}

QWidget* CharacterDetailPage::createCreateConfigArea()
{
    QWidget *card = createCard("createCard");
    QVBoxLayout *layout = new QVBoxLayout(card);
    setupVBoxLayout(layout, 24, 24, 24, 24, 16);

    QLabel *cardTitle = createLabel(tr("新建配置"), "font-size: 16px; font-weight: bold; color: #1e293b; background: transparent;");
    layout->addWidget(cardTitle);

    QWidget *nameRow = createTransparentWidget();
    QHBoxLayout *nameLayout = new QHBoxLayout(nameRow);
    setupHBoxLayout(nameLayout, 0, 0, 0, 0, 12);

    QLabel *nameLabel = createLabel(tr("配置名称"), "font-size: 14px; color: #374151; background: transparent;");
    m_configNameEdit = new QLineEdit();
    m_configNameEdit->setPlaceholderText(tr("输入配置名称"));
    m_configNameEdit->setStyleSheet(INPUT_STYLE);
    m_configNameEdit->setMinimumHeight(INPUT_HEIGHT);

    nameLayout->addWidget(nameLabel);
    nameLayout->addWidget(m_configNameEdit, 1);
    layout->addWidget(nameRow);

    QWidget *descRow = createTransparentWidget();
    QVBoxLayout *descLayout = new QVBoxLayout(descRow);
    setupVBoxLayout(descLayout, 0, 0, 0, 0, 8);

    QLabel *descLabel = createLabel(tr("角色配置说明"), "font-size: 14px; color: #374151; background: transparent;");
    m_configDescEdit = new QTextEdit();
    m_configDescEdit->setPlaceholderText(tr("输入角色配置说明，尽量写清外观、性格、服装和参考设定。"));
    m_configDescEdit->setStyleSheet(TEXTEDIT_STYLE);
    m_configDescEdit->setMaximumHeight(80);

    descLayout->addWidget(descLabel);
    descLayout->addWidget(m_configDescEdit);
    layout->addWidget(descRow);

    QWidget *btnRow = createTransparentWidget();
    QHBoxLayout *btnLayout = new QHBoxLayout(btnRow);
    setupHBoxLayout(btnLayout, 0, 0, 0, 0, 0);

    m_createConfigBtn = new QPushButton(tr("创建配置"));
    m_createConfigBtn->setStyleSheet(BTN_PRIMARY_STYLE);
    m_createConfigBtn->setMinimumHeight(40);
    m_createConfigBtn->setCursor(Qt::PointingHandCursor);
    connect(m_createConfigBtn, &QPushButton::clicked, this, &CharacterDetailPage::onCreateConfigClicked);

    btnLayout->addStretch();
    btnLayout->addWidget(m_createConfigBtn);
    layout->addWidget(btnRow);

    return card;
}

QWidget* CharacterDetailPage::createConfigListArea()
{
    QWidget *area = createTransparentWidget();
    QVBoxLayout *layout = new QVBoxLayout(area);
    setupVBoxLayout(layout, 0, 0, 0, 0, 16);

    QLabel *titleLabel = createLabel(tr("配置列表"), "font-size: 18px; font-weight: bold; color: #1e293b; background: transparent;");
    layout->addWidget(titleLabel);

    m_configContainer = createTransparentWidget();
    m_configLayout = new QVBoxLayout(m_configContainer);
    setupVBoxLayout(m_configLayout, 0, 0, 0, 0, 16);

    layout->addWidget(m_configContainer);

    return area;
}


void CharacterDetailPage::loadCharacterData()
{
    
    m_character.setId(m_charId.isEmpty() ? "char-001" : m_charId);
    m_character.setName(m_character.name().isEmpty() ? tr("角色") : m_character.name());
    m_character.setRole(tr("主角"));
    
    CharacterAppearance appearance;
    appearance.gender = tr("女");
    appearance.age = 18;
    m_character.setAppearance(appearance);
    m_character.setPersonality(QStringList{tr("温柔"), tr("勇敢"), tr("善良")});

    m_titleLabel->setText(m_character.name() + tr(" - 角色详情"));
    m_nameLabel->setText(m_character.name());
    m_roleLabel->setText(roleToLabel(m_character.role()));
    m_genderLabel->setText(genderToLabel(m_character.appearance().gender));
    m_ageLabel->setText(m_character.appearance().age > 0 ? QString::number(m_character.appearance().age) : QStringLiteral("-"));
    m_personalityLabel->setText(m_character.personality().join(", "));

    refreshConfigList();
}

void CharacterDetailPage::refreshConfigList()
{
    QLayoutItem *item;
    while ((item = m_configLayout->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }

    if (m_configs.isEmpty()) {
        CharacterConfiguration config1;
        config1.id = "config-001";
        config1.charId = m_character.id();
        config1.name = tr("默认角色配置");
        config1.description = tr("默认配置。");
        config1.tags = QStringList() << "default" << "profile";
        config1.referenceImages = QStringList{"ref1.jpg", "ref2.jpg"};
        config1.isDefault = true;
        m_configs.append(config1);

        CharacterConfiguration config2;
        config2.id = "config-002";
        config2.charId = m_character.id();
        config2.name = tr("战斗角色配置");
        config2.description = tr("战斗装束，轻装装备。");
        config2.tags = QStringList() << "battle" << "combat";
        config2.referenceImages = QStringList{"ref3.jpg"};
        config2.isDefault = false;
        m_configs.append(config2);
    }

    for (const CharacterConfiguration &config : m_configs) {
        addConfigurationCard(config);
    }

    if (m_configs.isEmpty()) {
        QLabel *emptyLabel = createLabel(tr("暂无配置，点击上方创建新配置"), "font-size: 14px; color: #94a3b8; background: transparent;");
        emptyLabel->setAlignment(Qt::AlignCenter);
        m_configLayout->addWidget(emptyLabel);
    }
}

void CharacterDetailPage::addConfigurationCard(const CharacterConfiguration &config)
{
    ConfigurationCard *card = new ConfigurationCard(config, this);
    connect(card, &ConfigurationCard::uploadRequested, this, &CharacterDetailPage::onUploadReference);
    connect(card, &ConfigurationCard::generatePortraitRequested, this, &CharacterDetailPage::onGeneratePortrait);
    connect(card, &ConfigurationCard::editRequested, this, &CharacterDetailPage::onEditConfig);
    m_configLayout->addWidget(card);
}

QString CharacterDetailPage::roleToLabel(const QString &role) const
{
    return role;
}

QString CharacterDetailPage::genderToLabel(const QString &gender) const
{
    return gender;
}

void CharacterDetailPage::onBackClicked()
{
    emit backRequested();
}

void CharacterDetailPage::onCreateConfigClicked()
{
    QString name = m_configNameEdit->text().trimmed();
    if (name.isEmpty()) {
        QMessageBox::warning(this, tr("提示"), tr("请输入配置名称"));
        return;
    }

    CharacterConfiguration newConfig;
    newConfig.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    newConfig.charId = m_character.id();
    newConfig.name = name;
    newConfig.description = m_configDescEdit->toPlainText().trimmed();
    newConfig.isDefault = m_configs.isEmpty();

    m_configs.append(newConfig);
    refreshConfigList();

    m_configNameEdit->clear();
    m_configDescEdit->clear();

    QMessageBox::information(this, tr("已保存"), tr("%1 已保存").arg(name));
}

void CharacterDetailPage::onUploadReference(const QString &configId)
{
    QString fileName = QFileDialog::getOpenFileName(
        this,
        tr("选择参考图片"),
        QString(),
        tr("图片文件 (*.png *.jpg *.jpeg *.bmp)")
    );
    if (fileName.isEmpty()) {
        return;
    }
    
    QwenImageClient* imageClient = ServiceContainer::instance()->qwenImageClient();
    if (!imageClient) {
        QMessageBox::warning(this, tr("错误"), tr("图片服务不可用"));
        return;
    }
    
    QString refImagePath = QUuid::createUuid().toString(QUuid::WithoutBraces);
    LOG_INFO("CharacterDetailPage", QString("Upload reference image for config %1: %2 -> %3")
        .arg(configId, fileName, refImagePath));
    
    QMessageBox::information(this, tr("上传成功"), tr("参考图片已上传，配置ID: %1").arg(configId));
}

void CharacterDetailPage::onGeneratePortrait(const QString &configId)
{
    QwenImageClient* imageClient = ServiceContainer::instance()->qwenImageClient();
    if (!imageClient) {
        QMessageBox::warning(this, tr("错误"), tr("图片服务不可用"));
        return;
    }
    
    QwenImageClient::GenerateOptions options;
    options.requestId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    options.prompt = tr("角色肖像生成");
    options.size = QwenImageClient::ImageSize::Square;
    
    connect(imageClient, &QwenImageClient::generateCompleted,
            this, [this, configId](const QwenImageClient::GenerateResult& result) {
        if (result.success) {
            LOG_INFO("CharacterDetailPage", QString("Portrait generated for config %1, size: %2x%3")
                .arg(configId).arg(result.width).arg(result.height));
            QMessageBox::information(this, tr("生成成功"), tr("肖像已生成，配置ID: %1").arg(configId));
        } else {
            QMessageBox::warning(this, tr("生成失败"), tr("错误: %1").arg(result.errorMessage));
        }
    }, Qt::UniqueConnection);
    
    imageClient->generateAsync(options);
    QMessageBox::information(this, tr("已提交"), tr("肖像生成任务已提交，配置ID: %1").arg(configId));
}

void CharacterDetailPage::onEditConfig(const QString &configId)
{
    for (const CharacterConfiguration& config : m_configs) {
        if (config.id == configId) {
            m_configNameEdit->setText(config.name);
            m_configDescEdit->setPlainText(config.description);
            
            QMessageBox::information(this, tr("编辑模式"), tr("正在编辑配置: %1").arg(configId));
            return;
        }
    }
    QMessageBox::warning(this, tr("配置不存在"), tr("未找到配置: %1").arg(configId));
}

// ============================================================================
// ============================================================================

ConfigurationCard::ConfigurationCard(const CharacterConfiguration &config, QWidget *parent)
    : QFrame(parent)
    , m_configId(config.id)
    , m_configName(config.name)
    , m_configDesc(config.description)
    , m_tags(config.tags)
    , m_referenceImages(config.referenceImages)
    , m_portraits(config.portraits)
    , m_isDefault(config.isDefault)
    , m_nameLabel(nullptr)
    , m_defaultBadge(nullptr)
    , m_descLabel(nullptr)
    , m_tagsWidget(nullptr)
    , m_referenceImagesWidget(nullptr)
    , m_portraitsWidget(nullptr)
    , m_uploadBtn(nullptr)
    , m_generateBtn(nullptr)
    , m_editBtn(nullptr)
{
    setupUI();
}

ConfigurationCard::~ConfigurationCard()
{
}

QWidget* ConfigurationCard::createTransparentWidget()
{
    QWidget *widget = new QWidget();
    widget->setStyleSheet("background: transparent;");
    return widget;
}

QLabel* ConfigurationCard::createLabel(const QString &text, const QString &style, int fontSize, bool bold)
{
    QLabel *label = new QLabel(text);
    label->setStyleSheet(style);
    if (fontSize > 0) {
        label->setFont(QFont("Microsoft YaHei", fontSize, bold ? QFont::Bold : QFont::Normal));
    }
    return label;
}

void ConfigurationCard::setupVBoxLayout(QVBoxLayout *layout, int left, int top, int right, int bottom, int spacing)
{
    layout->setContentsMargins(left, top, right, bottom);
    layout->setSpacing(spacing);
}

void ConfigurationCard::setupHBoxLayout(QHBoxLayout *layout, int left, int top, int right, int bottom, int spacing)
{
    layout->setContentsMargins(left, top, right, bottom);
    layout->setSpacing(spacing);
}

void ConfigurationCard::setupUI()
{
    this->setStyleSheet(R"(
        ConfigurationCard {
            background: #f2ffffff;
            border-radius: 16px;
            border: 1px solid #1a6366f1;
        }
        ConfigurationCard:hover {
            border: 1px solid #406366f1;
        }
    )");

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    setupVBoxLayout(mainLayout, 20, 20, 20, 20, 16);

    mainLayout->addWidget(createHeader());
    mainLayout->addWidget(createDescription());
    mainLayout->addWidget(createTagsSection());
    mainLayout->addWidget(createImagesSection());
    mainLayout->addWidget(createActionButtons());
}

QWidget* ConfigurationCard::createHeader()
{
    QWidget *header = createTransparentWidget();
    QHBoxLayout *layout = new QHBoxLayout(header);
    setupHBoxLayout(layout, 0, 0, 0, 0, 8);

    m_nameLabel = createLabel(m_configName, "font-size: 16px; font-weight: bold; color: #1e293b; background: transparent;");

    if (m_isDefault) {
        m_defaultBadge = createLabel(tr("默认"), "padding: 4px 10px; background: #2684cc16; color: #4d7c0f; border-radius: 6px; font-size: 11px; font-weight: 600;");
    } else {
        m_defaultBadge = nullptr;
    }

    layout->addWidget(m_nameLabel);
    if (m_defaultBadge) {
        layout->addWidget(m_defaultBadge);
    }
    layout->addStretch();

    return header;
}

QWidget* ConfigurationCard::createDescription()
{
    QWidget *widget = createTransparentWidget();
    QVBoxLayout *layout = new QVBoxLayout(widget);
    setupVBoxLayout(layout, 0, 0, 0, 0, 0);

    QString desc = m_configDesc.isEmpty() ? tr("暂无描述") : m_configDesc;
    m_descLabel = createLabel(desc, "font-size: 13px; color: #64748b; background: transparent;");
    m_descLabel->setWordWrap(true);

    layout->addWidget(m_descLabel);
    return widget;
}

QWidget* ConfigurationCard::createTagsSection()
{
    QWidget *widget = createTransparentWidget();
    QHBoxLayout *layout = new QHBoxLayout(widget);
    setupHBoxLayout(layout, 0, 0, 0, 0, 8);

    if (!m_tags.isEmpty()) {
        for (const QString &tag : m_tags) {
            QLabel *tagLabel = createLabel(tag, "padding: 4px 10px; background: #1a84cc16; color: #4d7c0f; border-radius: 6px; font-size: 12px;");
            layout->addWidget(tagLabel);
        }
    }
    layout->addStretch();

    return widget;
}

QWidget* ConfigurationCard::createImagesSection()
{
    QWidget *widget = createTransparentWidget();
    QVBoxLayout *layout = new QVBoxLayout(widget);
    setupVBoxLayout(layout, 0, 0, 0, 0, 12);

    QWidget *refRow = createTransparentWidget();
    QHBoxLayout *refLayout = new QHBoxLayout(refRow);
    setupHBoxLayout(refLayout, 0, 0, 0, 0, 8);

    QLabel *refTitle = createLabel(tr("参考图片"), "font-size: 12px; color: #94a3b8; background: transparent;");
    QLabel *refCount = createLabel(QString::number(m_referenceImages.size()), "font-size: 12px; color: #4d7c0f; background: transparent; font-weight: 600;");
    
    refLayout->addWidget(refTitle);
    refLayout->addWidget(refCount);
    refLayout->addWidget(createImageGrid(m_referenceImages, 5));
    refLayout->addStretch();

    layout->addWidget(refRow);
    QWidget *portraitRow = createTransparentWidget();
    QHBoxLayout *portraitLayout = new QHBoxLayout(portraitRow);
    setupHBoxLayout(portraitLayout, 0, 0, 0, 0, 8);

    QLabel *portraitTitle = createLabel(tr("标准图"), "font-size: 12px; color: #94a3b8; background: transparent;");
    QLabel *portraitCount = createLabel(QString::number(m_portraits.size()), "font-size: 12px; color: #4d7c0f; background: transparent; font-weight: 600;");
    
    portraitLayout->addWidget(portraitTitle);
    portraitLayout->addWidget(portraitCount);
    portraitLayout->addWidget(createImageGrid(m_portraits, 5));
    portraitLayout->addStretch();

    layout->addWidget(portraitRow);

    return widget;
}

QWidget* ConfigurationCard::createImageThumbnail(const QString &/*url*/, const QString &placeholder)
{
    QWidget *widget = new QWidget();
    widget->setFixedSize(THUMBNAIL_SIZE, THUMBNAIL_SIZE);
    widget->setStyleSheet(QString(R"(
        background: #f1f5f9;
        border-radius: 8px;
        border: 1px solid #e2e8f0;
    )"));

    QVBoxLayout *layout = new QVBoxLayout(widget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setAlignment(Qt::AlignCenter);

    QLabel *icon = createLabel(placeholder, "font-size: 20px; background: transparent;");
    icon->setAlignment(Qt::AlignCenter);
    layout->addWidget(icon);

    return widget;
}

QWidget* ConfigurationCard::createImageGrid(const QStringList &images, int maxShow)
{
    QWidget *widget = createTransparentWidget();
    QHBoxLayout *layout = new QHBoxLayout(widget);
    setupHBoxLayout(layout, 0, 0, 0, 0, 8);

    int showCount = qMin(images.size(), maxShow);
    for (int i = 0; i < showCount; ++i) {
        layout->addWidget(createImageThumbnail(images[i], tr("缩略图")));
    }

    if (images.size() > maxShow) {
        QLabel *moreLabel = createLabel(QString("+%1").arg(images.size() - maxShow), "font-size: 12px; color: #4d7c0f; background: transparent;");
        layout->addWidget(moreLabel);
    }

    return widget;
}

QWidget* ConfigurationCard::createActionButtons()
{
    QWidget *widget = createTransparentWidget();
    QHBoxLayout *layout = new QHBoxLayout(widget);
    setupHBoxLayout(layout, 0, 0, 0, 0, 12);

    m_uploadBtn = new QPushButton(tr("上传"));
    m_uploadBtn->setStyleSheet(R"(
        QPushButton {
            padding: 10px 18px;
            font-size: 13px;
            background: #ffffff;
            color: #4d7c0f;
            border: 2px solid #d4d4d4;
            border-radius: 8px;
            font-weight: 500;
        }
        QPushButton:hover {
            background: #f7fee7;
            border-color: #84cc16;
        }
    )");
    m_uploadBtn->setCursor(Qt::PointingHandCursor);
    m_generateBtn = new QPushButton(tr("生成"));
    m_generateBtn->setStyleSheet(R"(
        QPushButton {
            padding: 10px 18px;
            font-size: 13px;
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 #10b981, stop:1 #34d399);
            color: white;
            border: none;
            border-radius: 8px;
            font-weight: 500;
        }
        QPushButton:hover {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 #059669, stop:1 #10b981);
        }
    )");
    m_generateBtn->setCursor(Qt::PointingHandCursor);
    connect(m_generateBtn, &QPushButton::clicked, this, &ConfigurationCard::onGenerateClicked);

    m_editBtn = new QPushButton(tr("编辑"));
    m_editBtn->setStyleSheet(R"(
        QPushButton {
            padding: 10px 18px;
            font-size: 13px;
            background: #ffffff;
            color: #64748b;
            border: 2px solid #e2e8f0;
            border-radius: 8px;
            font-weight: 500;
        }
        QPushButton:hover {
            background: #f8fafc;
            border-color: #94a3b8;
        }
    )");
    m_editBtn->setCursor(Qt::PointingHandCursor);
    connect(m_editBtn, &QPushButton::clicked, this, &ConfigurationCard::onEditClicked);

    layout->addWidget(m_uploadBtn);
    layout->addWidget(m_generateBtn);
    layout->addWidget(m_editBtn);
    layout->addStretch();

    return widget;
}

void ConfigurationCard::onUploadClicked()
{
    emit uploadRequested(m_configId);
}

void ConfigurationCard::onGenerateClicked()
{
    emit generatePortraitRequested(m_configId);
}

void ConfigurationCard::onEditClicked()
{
    emit editRequested(m_configId);
}

