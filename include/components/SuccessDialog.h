#ifndef SUCCESSDIALOG_H
#define SUCCESSDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

class SuccessDialog : public QDialog
{
    Q_OBJECT

public:
    enum Type {
        Success,
        Warning,
        Error,
        Info
    };

    struct ColorScheme {
        QString primary;
        QString hover;
        QString pressed;
    };

    static void showSuccess(QWidget *parent, const QString &title, const QString &message);
    static void showWarning(QWidget *parent, const QString &title, const QString &message);
    static void showError(QWidget *parent, const QString &title, const QString &message);
    static void showInfo(QWidget *parent, const QString &title, const QString &message);

    explicit SuccessDialog(QWidget *parent = nullptr);
    ~SuccessDialog() = default;

    void setType(Type type);
    void setTitle(const QString &title);
    void setMessage(const QString &message);
    void setDetails(const QStringList &details);

protected:
    void showEvent(QShowEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void onCloseClicked();

private:
    void setupUI();
    void createHeader(QVBoxLayout *parentLayout);
    void createContent(QVBoxLayout *parentLayout);
    void createFooter(QVBoxLayout *parentLayout);
    void applyStyle();
    QString getIconText() const;
    ColorScheme getColorScheme() const;
    void animateFadeIn();
    void animateFadeOut();
    static void showDialog(QWidget *parent, Type type, const QString &title, const QString &message);

    Type m_type = Success;
    QLabel *m_iconLabel = nullptr;
    QLabel *m_titleLabel = nullptr;
    QLabel *m_messageLabel = nullptr;
    QWidget *m_detailsWidget = nullptr;
    QVBoxLayout *m_detailsLayout = nullptr;
    QPushButton *m_closeBtn = nullptr;
    QWidget *m_container = nullptr;
};

#endif // SUCCESSDIALOG_H
