#include "LoginDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QMessageBox>

LoginDialog::LoginDialog(IAuthService& auth, QWidget* parent)
    : QDialog(parent), m_auth(auth)
{
    setWindowTitle("ATMosFS — Autentificare");
    setFixedSize(380, 280);
    setModal(true);

    // ── Layout principal ─────────────────────────────
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(30, 20, 30, 20);
    mainLayout->setSpacing(15);

    // ── Titlu ────────────────────────────────────────
    auto* titleLabel = new QLabel("ATMosFS", this);
    QFont titleFont;
    titleFont.setPointSize(18);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    titleLabel->setAlignment(Qt::AlignCenter);

    // ── Form ─────────────────────────────────────────
    auto* formLayout = new QFormLayout();

    m_usernameEdit = new QLineEdit(this);
    m_usernameEdit->setPlaceholderText("minim 3 caractere");
    m_usernameEdit->setMinimumHeight(32);

    m_passwordEdit = new QLineEdit(this);
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    m_passwordEdit->setPlaceholderText("parola ta");
    m_passwordEdit->setMinimumHeight(32);

    formLayout->addRow("Username:", m_usernameEdit);
    formLayout->addRow("Parolă:",   m_passwordEdit);

    // ── Label eroare ──────────────────────────────────
    m_errorLabel = new QLabel(this);
    m_errorLabel->setStyleSheet("color: red; font-size: 12px;");
    m_errorLabel->setAlignment(Qt::AlignCenter);
    m_errorLabel->hide();

    // ── Butoane ───────────────────────────────────────
    auto* btnLayout = new QHBoxLayout();

    auto* loginBtn    = new QPushButton("Log In",      this);
    auto* registerBtn = new QPushButton("Înregistrare", this);

    loginBtn->setMinimumHeight(35);
    registerBtn->setMinimumHeight(35);

    loginBtn->setStyleSheet(
        "QPushButton { background-color: #0078D4; color: white; "
        "border-radius: 4px; font-weight: bold; }"
        "QPushButton:hover { background-color: #006CBE; }");

    registerBtn->setStyleSheet(
        "QPushButton { background-color: #E0E0E0; color: black; "
        "border-radius: 4px; }"
        "QPushButton:hover { background-color: #D0D0D0; }");

    btnLayout->addWidget(registerBtn);
    btnLayout->addWidget(loginBtn);

    // ── Asamblare ─────────────────────────────────────
    mainLayout->addWidget(titleLabel);
    mainLayout->addLayout(formLayout);
    mainLayout->addWidget(m_errorLabel);
    mainLayout->addLayout(btnLayout);

    // ── Conexiuni ─────────────────────────────────────
    connect(loginBtn,    &QPushButton::clicked, this, &LoginDialog::onLogin);
    connect(registerBtn, &QPushButton::clicked, this, &LoginDialog::onRegister);

    // Enter triggereaza login
    connect(m_passwordEdit, &QLineEdit::returnPressed, this, &LoginDialog::onLogin);
}

QString LoginDialog::getUsername() const {
    return m_loggedUsername;
}

void LoginDialog::onLogin() {
    QString username = m_usernameEdit->text().trimmed();
    QString password = m_passwordEdit->text();

    if (username.isEmpty() || password.isEmpty()) {
        showError("Completează toate câmpurile!");
        return;
    }

    try {
        // FIX: al tau are login() nu loginUser()
        if (!m_auth.login(username.toStdString(), password.toStdString())) {
            showError("Username sau parolă incorectă!");
            m_passwordEdit->clear();
            return;
        }

        m_loggedUsername = username;
        m_errorLabel->hide();
        resetStyles();
        accept();

    } catch (const std::exception& e) {
        showError(QString::fromStdString(e.what()));
        m_passwordEdit->clear();
    }
}

void LoginDialog::onRegister() {
    QString username = m_usernameEdit->text().trimmed();
    QString password = m_passwordEdit->text();

    if (username.length() < 3) {
        showError("Username-ul trebuie să aibă minim 3 caractere!");
        m_usernameEdit->setStyleSheet("border: 1px solid red;");
        return;
    }

    if (password.isEmpty()) {
        showError("Parola nu poate fi goală!");
        m_passwordEdit->setStyleSheet("border: 1px solid red;");
        return;
    }

    try {
        // FIX: al tau returneaza bool
        if (m_auth.registerUser(username.toStdString(), password.toStdString())) {
            showSuccess("Cont creat cu succes! Acum te poți autentifica.");
            resetStyles();
            m_passwordEdit->clear();
        } else {
            showError("Eroare la înregistrare — username deja folosit!");
            m_usernameEdit->setStyleSheet("border: 1px solid red;");
        }
    } catch (const std::exception& e) {
        showError(QString::fromStdString(e.what()));
    }
}

void LoginDialog::resetStyles() {
    m_usernameEdit->setStyleSheet("");
    m_passwordEdit->setStyleSheet("");
}

void LoginDialog::showError(const QString& message) {
    m_errorLabel->setStyleSheet("color: red; font-size: 12px;");
    m_errorLabel->setText(message);
    m_errorLabel->show();
}

void LoginDialog::showSuccess(const QString& message) {
    m_errorLabel->setStyleSheet("color: green; font-size: 12px;");
    m_errorLabel->setText(message);
    m_errorLabel->show();
}