#include <QApplication>
#include <QMessageBox>
#include "MainWindow.h"
#include "QtNetworkAdapter.h"
#include "LoginDialog.h"


int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setWindowIcon(QIcon("icon.ico"));

    QtNetworkAdapter adapter;
    QString login;

    while (true) {
        LoginDialog dialog(nullptr);

        if (dialog.exec() != QDialog::Accepted) {
            return 0;
        }

        if (!adapter.connectToServer("127.0.0.1", 8888)) {
            QMessageBox::critical(nullptr, "Ошибка",
                "Не удается подключиться к серверу. Убедитесь, что сервер запущен");
            continue;
        }

        bool success;
        if (dialog.isRegisterMode()) {
            success = adapter.registerUser(
                dialog.getLogin(),
                dialog.getPassword(),
                dialog.getBirthday()
            );
        }
        else {
            success = adapter.login(
                dialog.getLogin(),
                dialog.getPassword()
            );
        }

        if (success) {
            login = dialog.getLogin();
            break;
        }
        else {
            QMessageBox::critical(nullptr, "Ошибка",
                "Я не знаю такого пользователя! Проверь ещё раз, какой логин и пароль ты вводишь!");
            adapter.disconnect();
        }
    }

    MainWindow window;
    window.setAdapter(&adapter);  // передаём уже подключённый адаптер
    window.setCurrentUser(login);
    window.show();

    return app.exec();
}