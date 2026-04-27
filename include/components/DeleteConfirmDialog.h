#ifndef DELETECONFIRMDIALOG_H
#define DELETECONFIRMDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

class DeleteConfirmDialog : public QDialog
{
    Q_OBJECT

public:
    static bool showConfirm(QWidget *parent, const QString &title, const QString &message,
                            const QString &warningText = QString(), const QString &confirmText = QString());

    explicit DeleteConfirmDialog(QWidget *parent = nullptr);
    ~DeleteConfirmDialog() override = default;

    void setTitle(const QString &title);
    void setMessage(const QString &message);
    void setWarningText(const QString &warningText);
    void setConfirmText(const QString &confirmText);
    void updateFixedSize();

private slots:
    void onConfirmClicked();
    void onCancelClicked();

private:
    void setupUI();
    void createHeader(QVBoxLayout *parentLayout);
    void createMessageArea(QVBoxLayout *parentLayout);
    void createWarningArea(QVBoxLayout *parentLayout);
    void createButtonArea(QVBoxLayout *parentLayout);
    void animateClose();

    QLabel *m_titleLabel = nullptr;
    QLabel *m_messageLabel = nullptr;
    QLabel *m_warningLabel = nullptr;
    QPushButton *m_confirmBtn = nullptr;
    QPushButton *m_cancelBtn = nullptr;
    bool m_confirmed = false;
};

#endif // DELETECONFIRMDIALOG_H
