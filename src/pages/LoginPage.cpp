#include "pages/LoginPage.h"
#include "data/DatabaseManager.h"
#include "utils/UserSession.h"
#include "utils/Logger.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QCryptographicHash>
#include <QSvgWidget>
#include <QVariant>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QPixmap>
#include <QSizePolicy>

LoginPage::LoginPage(QWidget* parent)
    : QWidget(parent)
{
    setupUi();
    setupStyles();
}

// ── 工具方法 ──────────────────────────────────────────────

QString LoginPage::findAssetPath(const QString& filename) const
{
    const QString appDir = QCoreApplication::applicationDirPath();
    const QStringList candidates = {
        appDir + "/images/" + filename,
        appDir + "/../resources/images/" + filename,
        QDir::currentPath() + "/resources/images/" + filename,
    };
    for (const QString& p : candidates) {
        if (QFileInfo::exists(p))
            return p;
    }
    return {};
}

void LoginPage::showError(const QString& message)
{
    m_errorLabel->setText(message);
    m_errorLabel->show();
}

void LoginPage::addFormField(QVBoxLayout* form, const QString& labelText,
                              QWidget* parent, QWidget* fieldWidget, int bottomSpacing)
{
    QLabel* label = new QLabel(labelText, parent);
    label->setObjectName("fieldLabel");
    form->addWidget(label);
    form->addSpacing(8);
    form->addWidget(fieldWidget);
    form->addSpacing(bottomSpacing);
}

// ── UI 构建 ───────────────────────────────────────────────

void LoginPage::setupUi()
{
    setFixedSize(1600, 960);

    QHBoxLayout* root = new QHBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    setupLeftPanel(root);
    setupRightPanel(root);
    setupConnections();
}

void LoginPage::setupLeftPanel(QHBoxLayout* root)
{
    QWidget* leftPanel = new QWidget(this);
    leftPanel->setObjectName("leftPanel");
    leftPanel->setFixedWidth(674);
    root->addWidget(leftPanel);

    QVBoxLayout* form = new QVBoxLayout(leftPanel);
    form->setContentsMargins(97, 0, 97, 0);
    form->setSpacing(0);

    QLabel* welcomeLabel = new QLabel("欢迎使用 青柠漫画创作平台", leftPanel);
    welcomeLabel->setObjectName("subtitleLabel");
    form->addStretch(2);
    form->addWidget(welcomeLabel);
    form->addSpacing(36);

    m_usernameEdit = new QLineEdit(leftPanel);
    m_usernameEdit->setObjectName("inputField");
    m_usernameEdit->setPlaceholderText("请输入用户名");
    m_usernameEdit->setFixedHeight(78);
    addFormField(form, "用户名", leftPanel, m_usernameEdit, 24);

    m_passwordEdit = new QLineEdit(leftPanel);
    m_passwordEdit->setObjectName("inputField");
    m_passwordEdit->setPlaceholderText("请输入密码");
    m_passwordEdit->setFixedHeight(78);
    addFormField(form, "密码", leftPanel, m_passwordEdit, 28);

    m_errorLabel = new QLabel(leftPanel);
    m_errorLabel->setObjectName("errorLabel");
    m_errorLabel->setAlignment(Qt::AlignCenter);
    m_errorLabel->setFixedHeight(24);
    m_errorLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_errorLabel->hide();
    form->addWidget(m_errorLabel);

    m_loginButton = new QPushButton("登 录", leftPanel);
    m_loginButton->setObjectName("loginBtn");
    m_loginButton->setFixedHeight(80);
    m_loginButton->setCursor(Qt::PointingHandCursor);
    form->addWidget(m_loginButton);
    form->addStretch(3);

    QFrame* divider = new QFrame(this);
    divider->setObjectName("panelDivider");
    divider->setFrameShape(QFrame::VLine);
    divider->setGeometry(674, 0, 1, 960);
}

void LoginPage::setupRightPanel(QHBoxLayout* root)
{
    QWidget* rightPanel = new QWidget(this);
    rightPanel->setObjectName("rightPanel");
    root->addWidget(rightPanel, 1);

    QVBoxLayout* rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(0);

    constexpr int IMG_W = 708, IMG_H = 598;

    QWidget* imgContainer = new QWidget(rightPanel);
    imgContainer->setFixedSize(IMG_W, IMG_H);

    m_illustration = new QLabel(imgContainer);
    m_illustration->setGeometry(0, 0, IMG_W, IMG_H);

    const QString pngPath = findAssetPath("Image.png");
    if (!pngPath.isEmpty()) {
        QPixmap pix(pngPath);
        m_illustration->setPixmap(pix.scaled(IMG_W, IMG_H, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        m_illustration->setAlignment(Qt::AlignCenter);
    }

    const QString svgPath = findAssetPath("Image.svg");
    QSvgWidget* svgOverlay = new QSvgWidget(svgPath, imgContainer);
    svgOverlay->setGeometry(0, 0, IMG_W, IMG_H);
    svgOverlay->setAttribute(Qt::WA_TranslucentBackground);
    svgOverlay->setStyleSheet("background: transparent;");

    rightLayout->addStretch();
    rightLayout->addWidget(imgContainer, 0, Qt::AlignHCenter);
    rightLayout->addStretch();
}

void LoginPage::setupConnections()
{
    connect(m_loginButton,  &QPushButton::clicked,     this, &LoginPage::onLoginClicked);
    connect(m_usernameEdit, &QLineEdit::returnPressed, this, &LoginPage::onLoginClicked);
    connect(m_passwordEdit, &QLineEdit::returnPressed, this, &LoginPage::onLoginClicked);
}

// ── 样式 ──────────────────────────────────────────────────

void LoginPage::setupStyles()
{
    setStyleSheet(R"(
        LoginPage, QWidget#leftPanel {
            background-color: white;
        }

        QWidget#rightPanel {
            background-color: #EEF2FF;
        }

        QLabel#subtitleLabel {
            font-size: 22px;
            font-weight: 700;
            color: #444B59;
        }
        QLabel#fieldLabel {
            font-size: 15px;
            font-weight: 600;
            color: #444B59;
        }

        QLineEdit#inputField, QLineEdit#passwordField {
            font-size: 16px;
            color: #444B59;
        }
        QLineEdit#inputField::placeholder, QLineEdit#passwordField::placeholder {
            color: #B8C8EE;
        }

        QLineEdit#inputField {
            background-color: white;
            border: 2px solid #789ADE;
            border-radius: 39px;
            padding: 0 24px;
        }
        QLineEdit#inputField:focus {
            border-color: #5578C8;
        }

        QFrame#panelDivider {
            color: #E0E8F8;
        }

        QLabel#errorLabel {
            font-size: 13px;
            color: #E05252;
            padding: 0 0 8px 0;
        }

        QPushButton#loginBtn {
            background-color: #8699DA;
            color: white;
            border: none;
            border-radius: 40px;
            font-size: 18px;
            font-weight: 600;
        }
        QPushButton#loginBtn:hover {
            background-color: #7088CC;
        }
        QPushButton#loginBtn:pressed {
            background-color: #5C76BB;
        }
    )");
}

// ── 槽函数 ────────────────────────────────────────────────

void LoginPage::onLoginClicked()
{
    m_errorLabel->hide();

    const QString username = m_usernameEdit->text().trimmed();
    const QString password = m_passwordEdit->text();

    if (username.isEmpty() || password.isEmpty()) {
        showError("请输入用户名和密码。");
        return;
    }

    const QString passwordHash = QString(
        QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha256).toHex());

    const QList<QVariantMap> rows = DatabaseManager::instance()->executeQuery(
        "SELECT id, username, is_online FROM users WHERE username = ? AND password_hash = ? LIMIT 1",
        QVariantList() << username << passwordHash
    );

    if (rows.isEmpty()) {
        showError("用户名或密码错误。");
        LOG_WARNING("LoginPage", QString("Login failed for user: %1").arg(username));
        return;
    }

    const QVariantMap& user = rows.first();
    const QString userId = user.value("id").toString();
    const QString uname  = user.value("username").toString();

    if (user.value("is_online").toInt() == 1) {
        showError("该账号已在其他设备登录。");
        LOG_WARNING("LoginPage", QString("Login rejected, already online: %1").arg(uname));
        return;
    }

    DatabaseManager::instance()->executeUpdate(
        "UPDATE users SET is_online = 1 WHERE id = ?",
        QVariantList() << userId
    );

    UserSession::instance()->login(userId, uname);
    LOG_INFO("LoginPage", QString("Login success: %1 (%2)").arg(uname).arg(userId));

    emit loginSuccess(userId, uname);
}
