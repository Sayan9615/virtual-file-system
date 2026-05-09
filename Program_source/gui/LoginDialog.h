#pragma once
#include <QDialog>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include "../services/IAuthService.h"
#include "../exceptions/AuthException.h"
#include "../exceptions/AppException.h"

class LoginDialog : public QDialog {
    Q_OBJECT

public:
    explicit LoginDialog(IAuthService& auth, QWidget* parent = nullptr);

    // returneaza username-ul dupa login reusit
    QString getUsername() const;

private slots:
    void onLogin();
    void onRegister();

private:
    // ── Helper ───────────────────────────────────────
    void resetStyles();
    void showError(const QString& message);
    void showSuccess(const QString& message);

    // ── Date ─────────────────────────────────────────
    IAuthService& m_auth;
    QString      m_loggedUsername;

    // ── UI ───────────────────────────────────────────
    QLineEdit* m_usernameEdit;
    QLineEdit* m_passwordEdit;
    QLabel*    m_errorLabel;
    QPushButton* m_loginBtn;
    QPushButton* m_registerBtn;
};