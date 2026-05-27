#include <QApplication>
#include <QMessageBox>
#include "MainWindow.h"
#include "QtNetworkAdapter.h"
#include "LoginDialog.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setWindowIcon(QIcon(":/icon.ico"));

    QtNetworkAdapter* adapter = new QtNetworkAdapter();
    QString login;
    bool authenticated = false;

    while (!authenticated) {
        LoginDialog dialog(nullptr);
        if (dialog.exec() != QDialog::Accepted) {
            delete adapter;
            return 0;
        }

        if (!adapter->connectToServer("127.0.0.1", 8888)) {
            QMessageBox::critical(nullptr, "Ошибка",
                "Не удается подключиться к серверу. Убедитесь, что сервер запущен");
            continue;
        }

        bool success;
        if (dialog.isRegisterMode()) {
            success = adapter->registerUser(
                dialog.getLogin(),
                dialog.getPassword(),
                dialog.getBirthday()
            );
        }
        else {
            success = adapter->login(
                dialog.getLogin(),
                dialog.getPassword()
            );
        }

        if (success) {
            login = dialog.getLogin();
            authenticated = true;
        }
        else {
            QMessageBox::critical(nullptr, "Ошибка",
                "Неверный логин или пароль!");
            adapter->disconnect();
        }
    }

    MainWindow window;
    window.setAdapter(adapter);  // передаём указатель
    window.setCurrentUser(login);
    window.show();

    return app.exec();
}