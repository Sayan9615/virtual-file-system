#pragma once
#include <QDialog>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include "../services/AuthService.h"

class LoginDialog : public QDialog {
    Q_OBJECT

public:
    explicit LoginDialog(AuthService& auth, QWidget* parent = nullptr);
    QString getUsername() const;

private slots:
    void onLogin();
    void onRegister();

private:
    AuthService& m_auth;

    QLineEdit* m_usernameEdit;
    QLineEdit* m_passwordEdit;
    QLabel*    m_errorLabel;
    QString    m_loggedUsername;
};