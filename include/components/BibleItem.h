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

#include "Bible.h"

// 圣经条目组件，用于显示角色或场景信息
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

signals:
    void editClicked(const QString &id, BibleType type);  // 改为传递 ID
    void dataChanged(const QString &id, const QStringList &details);  // 改为传递 ID
    void imageClicked(const QString &imagePath);
    void uploadClicked(const QString &id, BibleType type);  // 改为传递 ID
    void deleteImageClicked(const QString &id, BibleType type);  // 改为传递 ID

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
    
    void saveCharacterData();
    void saveSceneData();
    void populateEditorData();
    void populateCharacterEditorData();
    void populateSceneEditorData();
    QString extractValue(const QString &detail, const QString &key);
    
    // 通用字段填充方法
    void populateTextField(const QString &detail, const QString &key, QLineEdit *edit);
    void populateTextField(const QString &detail, const QString &key, QTextEdit *edit);
    
    // UI 辅助方法
    QWidget* createInputField(const QString &label, QLineEdit *&edit, const QString &placeholder);
    QWidget* createInputField(const QString &label, QTextEdit *&edit, const QString &placeholder, int height = 80);
    QWidget* createButtonRow();
    QWidget* createEditorTitleRow(const QString &title);
    QWidget* createEditorOverlay();
    QFrame* createEditorCardBase(const QString &title, int height);
    QFrame* createSeparator();
    QScrollArea* createEditorScrollArea(QWidget *&contentWidget);
    
    // 基础组件
    QLabel *m_nameLabel;
    QWidget *m_detailsWidget;
    QLabel *m_imageLabel;
    QPushButton *m_editBtn;
    
    // 数据
    QString m_name;
    QString m_itemId;  // 稳定 ID（用于数据库查询）
    QStringList m_details;
    bool m_isExpanded;
    BibleType m_bibleType;
    QString m_currentImagePath;
    QString m_loadingImageId;
    
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
    
    QPushButton *m_saveBtn;
    QPushButton *m_cancelBtn;
};

#endif // BIBLEITEM_H
