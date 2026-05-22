#pragma once

#include <QObject>
#include "../NetworkClient.h"

class QtNetworkAdapter : public QObject
{
    Q_OBJECT

public:
    explicit QtNetworkAdapter(QObject* parent = nullptr);
    ~QtNetworkAdapter();

    bool connectToServer(const QString& host, int port);
    void disconnect();
    void sendCommand(const QString& cmd);

    bool login(const QString& login, const QString& password);
    bool registerUser(const QString& login, const QString& password, const QString& birthday);

    bool isConnected() const;
    QString getCurrentUser() const;

signals:
    void messageReceived(const QString& msg);

private slots:
    void onMessageReceived(const std::string& msg);

private:
    NetworkClient m_client;
};