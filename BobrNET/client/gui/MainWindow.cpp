#include "MainWindow.h"

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent)
{
    setWindowTitle("Messenger");
    setMinimumSize(600, 400);

    // Создаём виджеты
    m_chatHistory = new QTextEdit(this);
    m_chatHistory->setReadOnly(true);
    m_inputLine = new QLineEdit(this);
    m_sendBtn = new QPushButton("Send", this);

    // Раскладка
    QVBoxLayout* mainLayout = new QVBoxLayout;
    mainLayout->addWidget(m_chatHistory);

    QHBoxLayout* inputLayout = new QHBoxLayout;
    inputLayout->addWidget(m_inputLine);
    inputLayout->addWidget(m_sendBtn);
    mainLayout->addLayout(inputLayout);

    QWidget* central = new QWidget(this);
    central->setLayout(mainLayout);
    setCentralWidget(central);

    // Сигналы
    connect(m_sendBtn, &QPushButton::clicked, this, &MainWindow::onSend);
    connect(m_inputLine, &QLineEdit::returnPressed, this, &MainWindow::onSend);
}

MainWindow::~MainWindow() = default;

void MainWindow::onSend()
{
    QString text = m_inputLine->text().trimmed();
    if (text.isEmpty()) return;

    m_chatHistory->append("You: " + text);
    m_inputLine->clear();
}