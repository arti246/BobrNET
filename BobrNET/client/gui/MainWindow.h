#pragma once

#include <QMainWindow>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    void onSend();

private:
    QTextEdit* m_chatHistory;
    QLineEdit* m_inputLine;
    QPushButton* m_sendBtn;
};