#include "LoginDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QPushButton>
#include <QMessageBox>

LoginDialog::LoginDialog(AuthService& auth, QWidget* parent)
    : QDialog(parent), m_auth(auth) {
    setWindowTitle("ATMosFILE - Login");
    setFixedSize(350, 200);

    auto* layout = new QVBoxLayout(this);

    // Form
    auto* form = new QFormLayout();
    m_usernameEdit = new QLineEdit();
    m_passwordEdit = new QLineEdit();
    m_passwordEdit->setEchoMode(QLineEdit::Password);

    form->addRow("Username:", m_usernameEdit);
    form->addRow("Parola:",   m_passwordEdit);
    layout->addLayout(form);

    // Error label
    m_errorLabel = new QLabel("");
    m_errorLabel->setStyleSheet("color: red;");
    m_errorLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_errorLabel);

    // Butoane
    auto* btnLayout = new QHBoxLayout();
    auto* loginBtn    = new QPushButton("Login");
    auto* registerBtn = new QPushButton("Inregistrare");

    loginBtn->setDefault(true);
    btnLayout->addWidget(loginBtn);
    btnLayout->addWidget(registerBtn);
    layout->addLayout(btnLayout);

    connect(loginBtn,    &QPushButton::clicked, this, &LoginDialog::onLogin);
    connect(registerBtn, &QPushButton::clicked, this, &LoginDialog::onRegister);
    connect(m_passwordEdit, &QLineEdit::returnPressed, this, &LoginDialog::onLogin);
}

void LoginDialog::onLogin() {
    QString user = m_usernameEdit->text().trimmed();
    QString pass = m_passwordEdit->text();

    if (user.isEmpty() || pass.isEmpty()) {
        m_errorLabel->setText("Completeaza toate campurile!");
        return;
    }

    if (m_auth.login(user.toStdString(), pass.toStdString())) {
        m_loggedUsername = user;
        accept(); // inchide dialogul cu succes
    } else {
        m_errorLabel->setText("Username sau parola incorecta!");
        m_passwordEdit->clear();
    }
}

void LoginDialog::onRegister() {
    QString user = m_usernameEdit->text().trimmed();
    QString pass = m_passwordEdit->text();

    if (user.isEmpty() || pass.isEmpty()) {
        m_errorLabel->setText("Completeaza toate campurile!");
        return;
    }

    if (m_auth.registerUser(user.toStdString(), pass.toStdString())) {
        m_errorLabel->setStyleSheet("color: green;");
        m_errorLabel->setText("Cont creat! Te poti loga acum.");
    } else {
        m_errorLabel->setText("Username deja existent sau invalid!");
    }
}

QString LoginDialog::getUsername() const {
    return m_loggedUsername;
}