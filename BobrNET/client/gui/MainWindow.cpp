#include "MainWindow.h"
#include "QtNetworkAdapter.h"
#include "LoginDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QMessageBox>
#include <QPushButton>
#include <QDateTime>
#include <QRegularExpression>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), m_currentChatId(-1)
{
    setupUI();

    setStyleSheet(R"(
    QMainWindow {
        background-color: #2b2b2b;
    }
    QListWidget {
        background-color: #3c3c3c;
        color: #ffffff;
        border: none;
        font-size: 12px;
    }
    QListWidget::item:selected {
        background-color: #4a4a4a;
    }
    QTextEdit {
        background-color: #252526;
        color: #d4d4d4;
        border: none;
        font-size: 12px;
    }
    QLineEdit {
        background-color: #3c3c3c;
        color: #ffffff;
        border: 1px solid #555;
        padding: 5px;
        font-size: 12px;
    }
    QPushButton {
        background-color: #0e639c;
        color: white;
        border: none;
        padding: 5px;
        font-size: 12px;
    }
    QPushButton:hover {
        background-color: #1177bb;
    }
)");

    m_adapter = std::make_unique<QtNetworkAdapter>();

    connect(m_adapter.get(), &QtNetworkAdapter::messageReceived,
        this, &MainWindow::onMessageReceived);

    showLoginDialog();
}

MainWindow::~MainWindow()
{
    // Отключаем все соединения перед удалением
    if (m_adapter) {
        m_adapter->disconnect();
    }

    // Очищаем указатели
    m_chatList = nullptr;
    m_chatHistory = nullptr;
    m_inputLine = nullptr;
    m_sendBtn = nullptr;
}

void MainWindow::setupUI()
{
    setWindowTitle("BobrNET");
    setMinimumSize(800, 600);

    // Создаём виджеты
    m_chatList = new QListWidget(this);
    m_chatList->setMaximumWidth(250);

    // Стили для QListWidget
    m_chatList->setStyleSheet(R"(
    QListWidget {
        background-color: #2b2b2b;
        border: none;
        outline: none;
    }
    QListWidget::item {
        background-color: #3c3c3c;
        color: #ffffff;
        padding: 12px;
        border-bottom: 1px solid #4a4a4a;
        font-size: 13px;
    }
    QListWidget::item:hover {
        background-color: #4a6a8a;
    }
    QListWidget::item:selected {
        background-color: #0e639c;
        color: white;
    }
)");

    m_chatHistory = new QTextEdit(this);
    m_chatHistory->setReadOnly(true);
    m_chatHistory->setFontFamily("Consolas");
    m_chatHistory->setFontPointSize(10);

    m_inputLine = new QLineEdit(this);
    m_inputLine->setPlaceholderText("Type your message...");
    m_sendBtn = new QPushButton("Send", this);
    m_sendBtn->setFixedWidth(80);

    // Layout для чата
    QVBoxLayout* chatLayout = new QVBoxLayout;
    chatLayout->addWidget(m_chatHistory);

    QHBoxLayout* inputLayout = new QHBoxLayout;
    inputLayout->addWidget(m_inputLine);
    inputLayout->addWidget(m_sendBtn);
    chatLayout->addLayout(inputLayout);

    QWidget* chatWidget = new QWidget;
    chatWidget->setLayout(chatLayout);

    // Splitter для разделения списка чатов и области чата
    QSplitter* splitter = new QSplitter(Qt::Horizontal);
    splitter->addWidget(m_chatList);
    splitter->addWidget(chatWidget);
    splitter->setSizes({ 250, 550 });

    setCentralWidget(splitter);

    // Подключаем сигналы
    connect(m_sendBtn, &QPushButton::clicked, this, &MainWindow::onSendMessage);
    connect(m_inputLine, &QLineEdit::returnPressed, this, &MainWindow::onSendMessage);
    connect(m_chatList, &QListWidget::itemClicked, this, &MainWindow::onChatSelected);
}

void MainWindow::showLoginDialog()
{
    LoginDialog* dialog = new LoginDialog(this);
    if (dialog->exec() != QDialog::Accepted) {
        delete dialog;
        close();
        return;
    }

    // Сохраняем данные до удаления dialog
    QString login = dialog->getLogin();
    QString password = dialog->getPassword();
    QString birthday = dialog->getBirthday();
    bool isRegister = dialog->isRegisterMode();

    delete dialog;  // удаляем после получения данных

    if (!m_adapter->connectToServer("127.0.0.1", 8888)) {
        QMessageBox::critical(this, "Error", "Cannot connect to server");
        close();
        return;
    }

    bool success;
    if (isRegister) {
        success = m_adapter->registerUser(login, password, birthday);
    }
    else {
        success = m_adapter->login(login, password);
    }

    if (!success) {
        QMessageBox::critical(this, "Error", "Authentication failed");
        close();
        return;
    }

    setWindowTitle("BobrNET - " + login);
    m_adapter->sendCommand("/chats");
}

void MainWindow::onSendMessage()
{
    QString text = m_inputLine->text().trimmed();
    if (text.isEmpty()) return;

    // Локально добавляем своё сообщение в историю (сразу)
    if (m_currentChatId != -1) {
        // Получаем текущее время
        QString time = QDateTime::currentDateTime().toString("dd.MM.yyyy HH:mm");
        appendMessage(QString("[%1] Me: %2").arg(time, text));
    }

    // Отправляем на сервер
    if (m_currentChatId == -1) {
        m_adapter->sendCommand(text);
    }
    else {
        m_adapter->sendCommand(text);
    }

    m_inputLine->clear();
}

void MainWindow::onChatSelected(QListWidgetItem* item)
{
    if (!item) return;

    QString text = item->text();

    // Формат: "  [1] Username"
    QRegularExpression re("\\[([0-9]+)\\]");
    QRegularExpressionMatch match = re.match(text);

    if (match.hasMatch()) {
        int chatId = match.captured(1).toInt();
        m_currentChatId = chatId;
        m_adapter->sendCommand("/open " + QString::number(chatId));

        // Опционально: подсветка выбранного чата
        m_chatList->setCurrentItem(item);
    }
}

void MainWindow::onMessageReceived(const QString& msg)
{
    // Список чатов
        if (msg == "=== Your chats ===") {
            m_chatList->clear();
        }
    // Элемент списка чатов
        else if (msg.startsWith("  [")) {
            m_chatList->addItem(msg);
        }
    // Открытие чата
        else if (msg.startsWith("=== Chat:")) {
            m_chatHistory->clear();
            updateCurrentChat(msg);
        }
    // История сообщений
        else if (msg.startsWith("[") && msg.contains("] ")) {
            m_chatHistory->append(msg);
        }
    // Системные сообщения
        else {
            m_chatHistory->append(msg);
        }
}

void MainWindow::appendMessage(const QString& msg)
{
    m_chatHistory->append(msg);
}

void MainWindow::updateChatList(const QString& msg)
{
    if (msg == "=== Your chats ===") {
        m_chatList->clear();
        m_chatHistory->clear();
    }
    else if (msg.startsWith("  [")) {
        m_chatList->addItem(msg);
    }
}

void MainWindow::updateCurrentChat(const QString& msg)
{
    if (msg.startsWith("=== Chat:")) {
        // Очищаем историю перед загрузкой нового чата
        m_chatHistory->clear();

        // Сохраняем имя собеседника
        int start = 10;
        int end = msg.indexOf(" ===", start);
        if (end > start) {
            m_currentChatName = msg.mid(start, end - start);
        }
    }
}