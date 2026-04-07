#ifndef CONFIRMDIALOG_H
#define CONFIRMDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

class ConfirmDialog : public QDialog
{
    Q_OBJECT

public:
    static bool showConfirm(QWidget *parent, const QString &title, const QString &message);

    explicit ConfirmDialog(QWidget *parent = nullptr);
    ~ConfirmDialog();

    void setTitle(const QString &title);
    void setMessage(const QString &message);
    void adjustSize();

protected:
    void showEvent(QShowEvent *event) override;

private slots:
    void onConfirmClicked();
    void onCancelClicked();

private:
    void setupUI();
    void createHeader(QVBoxLayout *parentLayout);
    void createMessageArea(QVBoxLayout *parentLayout);
    void createButtonArea(QVBoxLayout *parentLayout);
    void animateClose();

    QLabel *m_iconLabel = nullptr;
    QLabel *m_titleLabel = nullptr;
    QLabel *m_messageLabel = nullptr;
    QPushButton *m_confirmBtn = nullptr;
    QPushButton *m_cancelBtn = nullptr;
    QWidget *m_container = nullptr;
    bool m_confirmed = false;
};

#endif
