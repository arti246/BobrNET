#pragma once

namespace MessageStatus {
    const int SENT = 0;      // Отправлено (сохранено в БД)
    const int DELIVERED = 1; // Доставлено клиенту
    const int READ = 2;      // Прочитано пользователем
};