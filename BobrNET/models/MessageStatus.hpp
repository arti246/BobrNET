#pragma once

enum class MessageStatus {
    SENT,       // отправлено (сохранено на сервере)
    DELIVERED,  // доставлено клиенту
    READ        // прочитано (опционально)
};