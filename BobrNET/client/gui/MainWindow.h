#pragma once

#include <QMainWindow>
#include <QListWidget>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <memory>

class QtNetworkAdapter;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();
    void setCurrentUser(const QString& login);
    void setAdapter(QtNetworkAdapter* adapter);

private slots:
    void onSendMessage();
    void onChatSelected(QListWidgetItem* item);
    void onMessageReceived(const QString& msg);

private:
    void setupUI();
    void updateChatList(const QString& msg);
    void appendMessage(const QString& msg);
    void updateCurrentChat(const QString& msg);

    std::unique_ptr<QtNetworkAdapter> m_adapter;

    QListWidget* m_chatList;
    QTextEdit* m_chatHistory;
    QLineEdit* m_inputLine;
    QPushButton* m_sendBtn;

    int m_currentChatId;
    QString m_currentChatName;
};