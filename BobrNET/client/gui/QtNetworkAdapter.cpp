#include "QtNetworkAdapter.h"

QtNetworkAdapter::QtNetworkAdapter(QObject* parent) : QObject(parent)
{
    m_client.setMessageCallback([this](const std::string& msg) {
        QMetaObject::invokeMethod(this, [this, msg]() {
            emit messageReceived(QString::fromStdString(msg));
            });
        });
}

QtNetworkAdapter::~QtNetworkAdapter()
{
    m_client.disconnect();
}

bool QtNetworkAdapter::connectToServer(const QString& host, int port)
{
    return m_client.connectToServer(host.toStdString(), port);
}

void QtNetworkAdapter::disconnect()
{
    m_client.disconnect();
}

void QtNetworkAdapter::sendCommand(const QString& cmd)
{
    m_client.sendCommand(cmd.toStdString());
}

bool QtNetworkAdapter::login(const QString& login, const QString& password)
{
    return m_client.login(login.toStdString(), password.toStdString());
}

bool QtNetworkAdapter::registerUser(const QString& login, const QString& password, const QString& birthday)
{
    return m_client.registerUser(login.toStdString(), password.toStdString(), birthday.toStdString());
}

bool QtNetworkAdapter::isConnected() const
{
    return m_client.isConnected();
}

QString QtNetworkAdapter::getCurrentUser() const
{
    return QString::fromStdString(m_client.getCurrentUser());
}

void QtNetworkAdapter::onMessageReceived(const std::string& msg) {
    // Преобразуем std::string в QString и отправляем сигнал дальше в GUI
    emit messageReceived(QString::fromStdString(msg));
}