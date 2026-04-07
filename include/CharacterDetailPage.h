#ifndef CHARACTERDETAILPAGE_H
#define CHARACTERDETAILPAGE_H

#include <QWidget>
#include <QLabel>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFrame>
#include <QListWidget>
#include "Character.h"

// 前向声明
class ConfigurationCard;

// 角色详情页面：显示角色基本信息和配置列表
class CharacterDetailPage : public QWidget
{
    Q_OBJECT

public:
    explicit CharacterDetailPage(const QString &charId, QWidget *parent = nullptr);
    ~CharacterDetailPage();
    
    // 设置角色 ID 并重新加载数据
    void setCharacterId(const QString &charId);

signals:
    void backRequested();

private slots:
    void onBackClicked();
    void onCreateConfigClicked();
    void onUploadReference(const QString &configId);
    void onGeneratePortrait(const QString &configId);
    void onEditConfig(const QString &configId);

private:
    // ========== UI 初始化 ==========
    void setupUI();
    QScrollArea* createScrollArea();
    QWidget* createContentWidget();
    QWidget* createHeader();
    QWidget* createBasicInfoCard();
    QWidget* createCreateConfigArea();
    QWidget* createConfigListArea();
    
    // ========== 辅助方法 ==========
    QWidget* createTransparentWidget();
    QLabel* createLabel(const QString &text, const QString &style, int fontSize = -1, bool bold = false);
    void setupVBoxLayout(QVBoxLayout *layout, int left, int top, int right, int bottom, int spacing = 0);
    void setupHBoxLayout(QHBoxLayout *layout, int left, int top, int right, int bottom, int spacing = 0);
    QWidget* createCard(const QString &objectName);
    QWidget* createInfoRow(const QString &labelText, const QString &valueText);
    
    // ========== 业务逻辑 ==========
    void loadCharacterData();
    void refreshConfigList();
    void addConfigurationCard(const CharacterConfiguration &config);
    QString roleToLabel(const QString &role) const;
    QString genderToLabel(const QString &gender) const;

    // ========== 成员变量 ==========
    QString m_charId;
    Character m_character;
    QList<CharacterConfiguration> m_configs;
    
    // 头部区域
    QPushButton *m_backBtn;
    QLabel *m_titleLabel;
    
    // 基本信息区域
    QLabel *m_nameLabel;
    QLabel *m_roleLabel;
    QLabel *m_genderLabel;
    QLabel *m_ageLabel;
    QLabel *m_personalityLabel;
    
    // 创建配置区域
    QLineEdit *m_configNameEdit;
    QTextEdit *m_configDescEdit;
    QPushButton *m_createConfigBtn;
    
    // 配置列表区域
    QWidget *m_configContainer;
    QVBoxLayout *m_configLayout;
};

// 配置卡片组件：显示单个角色配置
class ConfigurationCard : public QFrame
{
    Q_OBJECT

public:
    explicit ConfigurationCard(const CharacterConfiguration &config, QWidget *parent = nullptr);
    ~ConfigurationCard();

    QString configId() const { return m_configId; }

signals:
    void uploadRequested(const QString &configId);
    void generatePortraitRequested(const QString &configId);
    void editRequested(const QString &configId);

private slots:
    void onUploadClicked();
    void onGenerateClicked();
    void onEditClicked();

private:
    // ========== UI 初始化 ==========
    void setupUI();
    QWidget* createHeader();
    QWidget* createDescription();
    QWidget* createTagsSection();
    QWidget* createImagesSection();
    QWidget* createActionButtons();
    
    // ========== 辅助方法 ==========
    QWidget* createTransparentWidget();
    QLabel* createLabel(const QString &text, const QString &style, int fontSize = -1, bool bold = false);
    void setupVBoxLayout(QVBoxLayout *layout, int left, int top, int right, int bottom, int spacing = 0);
    void setupHBoxLayout(QHBoxLayout *layout, int left, int top, int right, int bottom, int spacing = 0);
    QWidget* createImageThumbnail(const QString &url, const QString &placeholder);
    QWidget* createImageGrid(const QStringList &images, int maxShow);

    // ========== 成员变量 ==========
    QString m_configId;
    QString m_configName;
    QString m_configDesc;
    QStringList m_tags;
    QStringList m_referenceImages;
    QStringList m_portraits;
    bool m_isDefault;
    
    // UI 组件
    QLabel *m_nameLabel;
    QLabel *m_defaultBadge;
    QLabel *m_descLabel;
    QWidget *m_tagsWidget;
    QWidget *m_referenceImagesWidget;
    QWidget *m_portraitsWidget;
    QPushButton *m_uploadBtn;
    QPushButton *m_generateBtn;
    QPushButton *m_editBtn;
};

#endif // CHARACTERDETAILPAGE_H
