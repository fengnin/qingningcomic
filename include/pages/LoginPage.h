#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QSvgWidget>
#include <QFrame>
#include <QHBoxLayout>

class LoginPage : public QWidget
{
    Q_OBJECT

public:
    explicit LoginPage(QWidget* parent = nullptr);

signals:
    void loginSuccess(const QString& userId, const QString& username);

private slots:
    void onLoginClicked();

private:
    void setupUi();
    void setupLeftPanel(QHBoxLayout* root);
    void setupRightPanel(QHBoxLayout* root);
    void setupConnections();
    void setupStyles();

    QString findAssetPath(const QString& filename) const;
    void    showError(const QString& message);
    void    addFormField(QVBoxLayout* form, const QString& labelText,
                         QWidget* parent, QWidget* fieldWidget, int bottomSpacing);

    QLabel*      m_illustration   = nullptr;
    QLineEdit*   m_usernameEdit   = nullptr;
    QLineEdit*   m_passwordEdit   = nullptr;
    QPushButton* m_loginButton    = nullptr;
    QLabel*      m_errorLabel     = nullptr;
};
