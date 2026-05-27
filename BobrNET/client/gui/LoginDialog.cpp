#include "LoginDialog.h"
#include <qmainwindow.h>
#include <QVBoxLayout>
#include <QHBoxLayout>

LoginDialog::LoginDialog(QWidget* parent) : QDialog(parent), m_isRegister(false)
{
    setWindowTitle("BobrNET - Авторизация");
    setMinimumSize(350, 300);
    setWindowIcon(QIcon(":/icon.ico"));

    // Главный горизонтальный layout
    QHBoxLayout* mainLayout = new QHBoxLayout(this);
    mainLayout->setSpacing(30);

    // Левый layout
    QVBoxLayout* leftLayout = new QVBoxLayout;
    leftLayout->addWidget(new QLabel("Логин:"));
    m_loginEdit = new QLineEdit;
    leftLayout->addWidget(m_loginEdit);

    leftLayout->addWidget(new QLabel("Пароль:"));
    m_passwordEdit = new QLineEdit;
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    leftLayout->addWidget(m_passwordEdit);

    leftLayout->addWidget(new QLabel("Дата рождения (YYYY-MM-DD):"));
    m_birthdayEdit = new QLineEdit;
    m_birthdayEdit->setPlaceholderText("YYYY-MM-DD");
    leftLayout->addWidget(m_birthdayEdit);

    // Кнопки
    m_loginBtn = new QPushButton("Авторизация");
    m_registerBtn = new QPushButton("Регистрация");

    QHBoxLayout* btnLayout = new QHBoxLayout;
    btnLayout->addWidget(m_loginBtn);
    btnLayout->addWidget(m_registerBtn);
    leftLayout->addLayout(btnLayout);

    // Правый layout
    QVBoxLayout* rightLayout = new QVBoxLayout;
    QLabel* label_img = new QLabel;
    QPixmap pixmap(":/bobr_han.jpg");
    label_img->setPixmap(pixmap);
    label_img->setScaledContents(true);
    label_img->setFixedSize(230, 300);
    rightLayout->addStretch();
    rightLayout->addWidget(label_img);
    rightLayout->addStretch();

    // Оба layout в главный
    mainLayout->addLayout(leftLayout);
    mainLayout->addLayout(rightLayout);

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