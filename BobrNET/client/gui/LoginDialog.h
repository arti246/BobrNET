#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>

class LoginDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LoginDialog(QWidget* parent = nullptr);
    QString getLogin() const { return m_loginEdit->text(); }
    QString getPassword() const { return m_passwordEdit->text(); }
    QString getBirthday() const { return m_birthdayEdit->text(); }
    bool isRegisterMode() const { return m_isRegister; }

private slots:
    void onLoginClicked();
    void onRegisterClicked();

private:
    QLineEdit* m_loginEdit;
    QLineEdit* m_passwordEdit;
    QLineEdit* m_birthdayEdit;
    QPushButton* m_loginBtn;
    QPushButton* m_registerBtn;
    bool m_isRegister;
};