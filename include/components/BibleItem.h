#ifndef BIBLEITEM_H
#define BIBLEITEM_H

#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QScrollArea>
#include <QSize>

#include "Bible.h"
#include "Character.h"
#include "Scene.h"
#include "ModeComboBox.h"

class QVBoxLayout;

namespace BibleItemConstants {
    constexpr int LABEL_WIDTH = 128;
    constexpr int LABEL_HEIGHT = 180;
    constexpr int LOAD_SCALE = 3;  // 提高缩放比例，使缩略图更清晰
    const QSize LABEL_SIZE(LABEL_WIDTH, LABEL_HEIGHT);
    const QSize LOAD_SIZE(LABEL_WIDTH * LOAD_SCALE, LABEL_HEIGHT * LOAD_SCALE);
}

class BibleItem : public QFrame
{
    Q_OBJECT

public:
    explicit BibleItem(const QString &name, const QStringList &details, 
                       BibleType type = BibleType::Character, QWidget *parent = nullptr);
    
    void setImage(const QString &imageUrl);
    void setDetails(const QStringList &details);
    void setItemId(const QString &id);  // 设置稳定 ID
    void expandEditor();
    void collapseEditor();
    
    QString getName() const { return m_name; }
    QString getItemId() const { return m_itemId; }  // 获取稳定 ID
    BibleType getType() const { return m_bibleType; }
    QStringList getDetails() const { return m_details; }

    void setCharacterData(const Character& character);
    void setSceneData(const Scene& scene);
    Character getCharacterData() const { return m_characterData; }
    Scene getSceneData() const { return m_sceneData; }
    bool hasCharacterData() const { return m_hasCharacterData; }
    bool hasSceneData() const { return m_hasSceneData; }

signals:
    void editClicked(const QString &id, BibleType type);
    void dataChanged(const QString &id, const QStringList &details);
    void characterDataChanged(const QString &id, const Character &character);
    void sceneDataChanged(const QString &id, const Scene &scene);
    void imageClicked(const QString &imagePath);
    void deleteRequested(const QString &id, BibleType type);
    void uploadClicked(const QString &id, BibleType type);
    void deleteImageClicked(const QString &id, BibleType type);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void onEditBtnClicked();
    void onSaveClicked();
    void onCancelClicked();
    void onAsyncImageLoaded(const QString& id, const QString& cacheKey, const QPixmap& pixmap);

private:
    void setupUI();
    void createEditorCard();
    void createCharacterEditorCard();
    void createSceneEditorCard();
    void showEditorCard();
    void hideEditorCard();
    void updateEditorCardPosition();
    void displayProcessedImage(const QPixmap& pixmap);
    static QPixmap trimWhiteBorders(const QPixmap& pixmap, int threshold = 240, int margin = 4);
    
    void saveCharacterData();
    void saveSceneData();
    void populateEditorData();
    void populateCharacterEditorData();
    void populateCharacterEditorFromData(const Character& character);
    void populateSceneEditorData();
    void populateSceneEditorFromData(const Scene& scene);
    QString extractValue(const QString &detail, const QString &key);
    
    // 通用字段填充方法
    void populateTextField(const QString &detail, const QString &key, QLineEdit *edit);
    void populateTextField(const QString &detail, const QString &key, QTextEdit *edit);
    
    // UI 辅助方法
    QWidget* createInputField(const QString &label, QLineEdit *&edit, const QString &placeholder);
    QWidget* createInputField(const QString &label, QTextEdit *&edit, const QString &placeholder, int height = 80);
    QWidget* createComboBoxField(const QString &label, ModeComboBox *&combo, const QStringList &items);
    QWidget* createButtonRow();
    QWidget* createEditorTitleRow(const QString &title);
    QWidget* createEditorOverlay();
    QFrame* createEditorCardBase(const QString &title, int height);
    QFrame* createSeparator();
    QScrollArea* createEditorScrollArea(QWidget *&contentWidget);
    
    // 编辑器卡片初始化结构（用于提取公共代码）
    struct EditorCardContext {
        QWidget* scrollContent;
        QVBoxLayout* cardLayout;
        QScrollArea* scrollArea;
        QVBoxLayout* cardMainLayout;
    };
    EditorCardContext initEditorCard(const QString& title, int height);
    void finishEditorCard(EditorCardContext& ctx);
    
    // 基础组件
    QLabel *m_nameLabel;
    QWidget *m_detailsWidget;
    QLabel *m_imageLabel;
    QPushButton *m_editBtn;
    QPushButton *m_deleteBtn;
    
    // 数据
    QString m_name;
    QString m_itemId;  // 稳定 ID（用于数据库查询）
    QStringList m_details;
    bool m_isExpanded;
    BibleType m_bibleType;
    QString m_currentImagePath;
    QString m_loadingImageId;
    
    // 原始数据对象（直接从数据源回填编辑器，避免从显示文本反解析）
    Character m_characterData;
    Scene m_sceneData;
    bool m_hasCharacterData = false;
    bool m_hasSceneData = false;
    
    // 浮动编辑卡片层
    QWidget *m_overlayWidget;
    QFrame *m_editorCard;
    
    // 角色编辑表单组件
    QComboBox *m_genderCombo;
    QSpinBox *m_ageSpin;
    QLineEdit *m_eyeColorEdit;
    QLineEdit *m_bodyTypeEdit;
    QLineEdit *m_hairColorEdit;
    QLineEdit *m_hairStyleEdit;
    QLineEdit *m_clothingEdit;
    QLineEdit *m_featuresEdit;
    QLineEdit *m_personalityEdit;
    
    // 场景编辑表单组件
    QLineEdit *m_sceneNameEdit;
    QTextEdit *m_sceneDescEdit;
    QLineEdit *m_buildingEdit;
    QLineEdit *m_colorEdit;
    QLineEdit *m_landmarkEdit;
    QLineEdit *m_layoutEdit;
    QLineEdit *m_atmosphereEdit;
    ModeComboBox *m_typeCombo;
    ModeComboBox *m_settingCombo;
    ModeComboBox *m_timeOfDayCombo;
    ModeComboBox *m_weatherCombo;
    QLineEdit *m_narrativeRoleEdit;
    
    QPushButton *m_saveBtn;
    QPushButton *m_cancelBtn;
};

#endif // BIBLEITEM_H
