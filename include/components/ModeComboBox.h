#ifndef MODECOMBOBOX_H
#define MODECOMBOBOX_H

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QMenu>
#include <QHBoxLayout>

// 自定义下拉框，带三角形箭头
class ModeComboBox : public QWidget
{
    Q_OBJECT

public:
    explicit ModeComboBox(QWidget *parent = nullptr);
    
    void addItem(const QString &text);
    void setCurrentIndex(int index);
    int currentIndex() const { return m_currentIndex; }
    QString currentText() const;
    int findText(const QString &text) const;
    void setFixedHeight(int height);

signals:
    void currentIndexChanged(int index);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void onDropDownClicked();

private:
    QLineEdit *m_lineEdit;
    QPushButton *m_dropBtn;
    QMenu *m_menu;
    QStringList m_items;
    int m_currentIndex;
};

#endif // MODECOMBOBOX_H
