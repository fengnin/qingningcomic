#ifndef STORYBOARDITEM_H
#define STORYBOARDITEM_H

#include "components/EditorCardBase.h"
#include <QComboBox>

class StoryboardItem : public EditorCardBase
{
    Q_OBJECT

public:
    explicit StoryboardItem(int panelNumber, const QString &panelId,
                           const QString &scene, 
                           const QString &shotType, const QString &cameraAngle,
                           const QString &characters, const QString &dialogue,
                           const QString &visualPrompt = QString(),
                           const QString &visualPromptEn = QString(), QWidget *parent = nullptr);
    
    void setScene(const QString &scene);
    void setShotType(const QString &shotType);
    void setCameraAngle(const QString &cameraAngle);
    void setCharacters(const QString &characters);
    void setDialogue(const QString &dialogue);
    void setVisualPrompt(const QString &prompt);
    void setVisualPromptEn(const QString &prompt);
    int panelNumber() const { return m_panelNumber; }
    QString panelId() const { return m_panelId; }

signals:
    void editClicked(int panelNumber);
    void dataChanged(const QString &panelId, int panelNumber, const QString &scene, 
                     const QString &shotType, const QString &cameraAngle,
                     const QString &characters, const QString &dialogue,
                     const QString &visualPrompt, const QString &visualPromptEn);

private slots:
    void onEditBtnClicked();
    void onSaveClicked();
    void onCancelClicked();

private:
    void setupUI();
    void setupEditorCard() override;
    void syncDataToEditor() override;
    
    QWidget* createHeaderRow();
    QWidget* createInfoRow(const QString &title, QLabel *&contentLabel, const QString &content, bool wordWrap = false);
    QWidget* createComboBoxGroup(const QString &title1, QComboBox *&combo1, const QStringList &items1,
                                  const QString &title2, QComboBox *&combo2, const QStringList &items2);
    QWidget* createComboBoxWithArrow(QComboBox *&combo, const QStringList &items);
    
    QLabel *m_panelNumberLabel;
    QLabel *m_sceneLabel;
    QLabel *m_shotTypeLabel;
    QLabel *m_charactersLabel;
    QLabel *m_dialogueLabel;
    QPushButton *m_editBtn;
    
    QString m_panelId;
    int m_panelNumber;
    QString m_scene;
    QString m_shotType;
    QString m_cameraAngle;
    QString m_characters;
    QString m_dialogue;
    QString m_visualPrompt;
    QString m_visualPromptEn;
    
    QTextEdit *m_sceneEdit;
    QComboBox *m_shotTypeCombo;
    QComboBox *m_cameraCombo;
    QTextEdit *m_charactersEdit;
    QTextEdit *m_dialogueEdit;
    QTextEdit *m_imagenPromptEdit;
    QPushButton *m_saveBtn;
    QPushButton *m_cancelBtn;
};

#endif // STORYBOARDITEM_H
