#include "LoginDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>

LoginDialog::LoginDialog(QWidget* parent) : QDialog(parent), m_isRegister(false)
{
    setWindowTitle("Messenger - Login");
    setMinimumSize(300, 200);

    QVBoxLayout* layout = new QVBoxLayout(this);

    layout->addWidget(new QLabel("Login:"));
    m_loginEdit = new QLineEdit;
    layout->addWidget(m_loginEdit);

    layout->addWidget(new QLabel("Password:"));
    m_passwordEdit = new QLineEdit;
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    layout->addWidget(m_passwordEdit);

    m_loginBtn = new QPushButton("Login");
    m_registerBtn = new QPushButton("Register");

    QHBoxLayout* btnLayout = new QHBoxLayout;
    btnLayout->addWidget(m_loginBtn);
    btnLayout->addWidget(m_registerBtn);
    layout->addLayout(btnLayout);

    connect(m_loginBtn, &QPushButton::clicked, this, &LoginDialog::onLoginClicked);
    connect(m_registerBtn, &QPushButton::clicked, this, &LoginDialog::onRegisterClicked);
}

void LoginDialog::onLoginClicked()
{
    m_isRegister = false;
    accept();
}

void LoginDialog::onRegisterClicked()
{
    m_isRegister = true;
    accept();
}