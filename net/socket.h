/*
 * socket.h - Обёртки над POSIX сокетами
 * Упрощённый интерфейс для работы с TCP и UDP
 */

#ifndef SOCKET_H
#define SOCKET_H

#include <stddef.h>
#include <stdint.h>
#include <netinet/in.h>

/* Структура сокета */
typedef struct {
    int fd;                     /* File descriptor */
    int is_tcp;                 /* 1 = TCP, 0 = UDP */
    struct sockaddr_in addr;    /* Адрес (для UDP или подключения) */
} Socket;

/* Создание TCP сокета */
Socket socket_tcp_create(void);

/* Создание UDP сокета */
Socket socket_udp_create(void);

/* Привязка сокета к порту */
int socket_bind(Socket *sock, int port);

/* Проверка доступности порта (0 = свободен, -1 = занят) */
int socket_check_port_available(int port, int is_tcp);

/* Привязка сокета с автоматическим выбором свободного порта
 * Возвращает 0 при успехе, -1 при ошибке
 * Обновляет *port на фактически использованный порт */
int socket_bind_with_fallback(Socket *sock, int *port, int max_attempts);

/* Прослушивание подключений (TCP) */
int socket_listen(Socket *sock, int backlog);

/* Принятие подключения (TCP) */
Socket socket_accept(Socket *listener);

/* Подключение к серверу (TCP) с таймаутом */
int socket_connect_with_timeout(Socket *sock, const char *host, int port, int timeout_ms);

/* Подключение к серверу (TCP) - обёртка с таймаутом по умолчанию (5 сек) */
int socket_connect(Socket *sock, const char *host, int port);

/* Установка неблокирующего режима */
int socket_set_nonblocking(Socket *sock);

/* Отправка данных (TCP) */
int socket_send(Socket *sock, const void *data, size_t len);

/* Отправка данных с гарантией полной отправки (TCP) */
int socket_send_all(Socket *sock, const void *data, size_t len);

/* Получение данных (TCP) */
int socket_recv(Socket *sock, void *buffer, size_t len);

/* Отправка данных (UDP) */
int socket_sendto(Socket *sock, const void *data, size_t len, struct sockaddr_in *dest);

/* Получение данных (UDP) */
int socket_recvfrom(Socket *sock, void *buffer, size_t len, struct sockaddr_in *src);

/* Мультиплексирование сокетов с помощью select() */
int socket_select(Socket *sockets[], int count, int timeout_ms, int *ready_flags);

/* Проверка, есть ли данные для чтения */
int socket_has_data(Socket *sock, int timeout_ms);

/* Проверка, готов ли сокет к записи */
int socket_is_writable(Socket *sock, int timeout_ms);

/* Закрытие сокета */
void socket_close(Socket *sock);

/* Проверка валидности сокета */
int socket_is_valid(Socket *sock);

/* Получение порта сокета */
int socket_get_port(Socket *sock);

#endif /* SOCKET_H */

