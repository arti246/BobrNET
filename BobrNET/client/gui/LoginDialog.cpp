#include "LoginDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>

LoginDialog::LoginDialog(QWidget* parent) : QDialog(parent), m_isRegister(false)
{
    setWindowTitle("BobrNET - Login");
    setMinimumSize(350, 300);
    setWindowIcon(QIcon(":/icon.ico"));

    QVBoxLayout* layout = new QVBoxLayout(this);

    // Поле Логин
    layout->addWidget(new QLabel("Login:"));
    m_loginEdit = new QLineEdit;
    layout->addWidget(m_loginEdit);

    // Поле Пароль
    layout->addWidget(new QLabel("Password:"));
    m_passwordEdit = new QLineEdit;
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    layout->addWidget(m_passwordEdit);

    // Поле Дата рождения (только для регистрации, но показываем всегда)
    layout->addWidget(new QLabel("Birthday (YYYY-MM-DD):"));
    m_birthdayEdit = new QLineEdit;
    m_birthdayEdit->setPlaceholderText("YYYY-MM-DD");
    layout->addWidget(m_birthdayEdit);

    // Кнопки
    m_loginBtn = new QPushButton("Login");
    m_registerBtn = new QPushButton("Register");

    QHBoxLayout* btnLayout = new QHBoxLayout;
    btnLayout->addWidget(m_loginBtn);
    btnLayout->addWidget(m_registerBtn);
    layout->addLayout(btnLayout);

    // Подключаем сигналы
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