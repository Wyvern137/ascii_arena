/*
 * socket.c - Реализация обёрток над POSIX сокетами
 */

#include "socket.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <netdb.h>

/* Создание TCP сокета */
Socket socket_tcp_create(void) {
    Socket sock;
    sock.fd = socket(AF_INET, SOCK_STREAM, 0);
    sock.is_tcp = 1;
    memset(&sock.addr, 0, sizeof(sock.addr));
    
    if (sock.fd >= 0) {
        /* Разрешаем переиспользование адреса */
        int opt = 1;
        setsockopt(sock.fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        
        /* SO_REUSEPORT позволяет нескольким сокетам привязываться к одному порту */
#ifdef SO_REUSEPORT
        setsockopt(sock.fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));
#endif
    }
    
    return sock;
}

/* Создание UDP сокета */
Socket socket_udp_create(void) {
    Socket sock;
    sock.fd = socket(AF_INET, SOCK_DGRAM, 0);
    sock.is_tcp = 0;
    memset(&sock.addr, 0, sizeof(sock.addr));
    
    if (sock.fd >= 0) {
        int opt = 1;
        setsockopt(sock.fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        
        /* SO_REUSEPORT позволяет нескольким сокетам привязываться к одному порту */
#ifdef SO_REUSEPORT
        setsockopt(sock.fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));
#endif
    }
    
    return sock;
}

/* Привязка сокета к порту */
int socket_bind(Socket *sock, int port) {
    sock->addr.sin_family = AF_INET;
    sock->addr.sin_addr.s_addr = INADDR_ANY;
    sock->addr.sin_port = htons((uint16_t)port);
    
    return bind(sock->fd, (struct sockaddr *)&sock->addr, sizeof(sock->addr));
}

/* Проверка доступности порта (0 = свободен, -1 = занят) */
int socket_check_port_available(int port, int is_tcp) {
    int sock_fd;
    
    if (is_tcp) {
        sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    } else {
        sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    }
    
    if (sock_fd < 0) {
        return -1;
    }
    
    /* Устанавливаем SO_REUSEADDR для проверки */
    int opt = 1;
    setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#ifdef SO_REUSEPORT
    setsockopt(sock_fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));
#endif
    
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons((uint16_t)port);
    
    int result = bind(sock_fd, (struct sockaddr *)&addr, sizeof(addr));
    close(sock_fd);
    
    return result;
}

/* Привязка сокета с автоматическим выбором свободного порта
 * Возвращает 0 при успехе, -1 при ошибке
 * Обновляет *port на фактически использованный порт */
int socket_bind_with_fallback(Socket *sock, int *port, int max_attempts) {
    int original_port = *port;
    
    for (int attempt = 0; attempt < max_attempts; attempt++) {
        int try_port = original_port + attempt;
        
        /* Проверяем, не вышли ли за пределы допустимых портов */
        if (try_port > 65535) {
            break;
        }
        
        sock->addr.sin_family = AF_INET;
        sock->addr.sin_addr.s_addr = INADDR_ANY;
        sock->addr.sin_port = htons((uint16_t)try_port);
        
        if (bind(sock->fd, (struct sockaddr *)&sock->addr, sizeof(sock->addr)) == 0) {
            *port = try_port;
            if (attempt > 0) {
                fprintf(stderr, "Порт %d занят, использован порт %d\n", original_port, try_port);
            }
            return 0;
        }
    }
    
    return -1;
}

/* Прослушивание подключений (TCP) */
int socket_listen(Socket *sock, int backlog) {
    return listen(sock->fd, backlog);
}

/* Принятие подключения (TCP) */
Socket socket_accept(Socket *listener) {
    Socket client;
    socklen_t addr_len = sizeof(client.addr);
    
    client.fd = accept(listener->fd, (struct sockaddr *)&client.addr, &addr_len);
    client.is_tcp = 1;
    
    return client;
}

/* Подключение к серверу (TCP) с таймаутом */
int socket_connect_with_timeout(Socket *sock, const char *host, int port, int timeout_ms) {
    struct addrinfo hints, *result, *rp;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;      /* IPv4 или IPv6 */
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_NUMERICSERV;
    
    char port_str[16];
    snprintf(port_str, sizeof(port_str), "%d", port);
    
    int ret = getaddrinfo(host, port_str, &hints, &result);
    if (ret != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret));
        return -1;
    }
    
    /* Попытка подключения к каждому адресу */
    for (rp = result; rp != NULL; rp = rp->ai_next) {
        /* Закрываем предыдущий сокет если был */
        if (sock->fd >= 0) {
            close(sock->fd);
        }
        
        sock->fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sock->fd < 0) {
            continue;
        }
        
        /* Разрешаем переиспользование адреса */
        int opt = 1;
        setsockopt(sock->fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        
        /* Устанавливаем неблокирующий режим для подключения с таймаутом */
        int flags = fcntl(sock->fd, F_GETFL, 0);
        if (flags >= 0) {
            fcntl(sock->fd, F_SETFL, flags | O_NONBLOCK);
        }
        
        int connect_result = connect(sock->fd, rp->ai_addr, rp->ai_addrlen);
        
        if (connect_result == 0) {
            /* Немедленное подключение (редко) */
            if (rp->ai_family == AF_INET) {
                memcpy(&sock->addr, rp->ai_addr, sizeof(struct sockaddr_in));
            }
            /* Оставляем неблокирующий режим (будет установлен в client_app_connect) */
            sock->is_tcp = 1;
            freeaddrinfo(result);
            return 0;
        }
        
        if (errno == EINPROGRESS) {
            /* Ожидаем завершения подключения с таймаутом */
            fd_set write_fds;
            FD_ZERO(&write_fds);
            FD_SET(sock->fd, &write_fds);
            
            struct timeval tv;
            tv.tv_sec = timeout_ms / 1000;
            tv.tv_usec = (timeout_ms % 1000) * 1000;
            
            int select_result = select(sock->fd + 1, NULL, &write_fds, NULL, &tv);
            
            if (select_result > 0) {
                /* Проверяем, успешно ли подключение */
                int error = 0;
                socklen_t len = sizeof(error);
                if (getsockopt(sock->fd, SOL_SOCKET, SO_ERROR, &error, &len) == 0 && error == 0) {
                    /* Успешное подключение */
                    if (rp->ai_family == AF_INET) {
                        memcpy(&sock->addr, rp->ai_addr, sizeof(struct sockaddr_in));
                    }
                    /* Оставляем неблокирующий режим (будет установлен в client_app_connect) */
                    sock->is_tcp = 1;
                    freeaddrinfo(result);
                    return 0;
                }
            }
        }
        
        /* Не удалось подключиться к этому адресу, пробуем следующий */
        close(sock->fd);
        sock->fd = -1;
    }
    
    freeaddrinfo(result);
    return -1;
}

/* Подключение к серверу (TCP) - обёртка с таймаутом по умолчанию */
int socket_connect(Socket *sock, const char *host, int port) {
    return socket_connect_with_timeout(sock, host, port, 5000);  /* 5 секунд таймаут */
}

/* Установка неблокирующего режима */
int socket_set_nonblocking(Socket *sock) {
    int flags = fcntl(sock->fd, F_GETFL, 0);
    if (flags < 0) return -1;
    return fcntl(sock->fd, F_SETFL, flags | O_NONBLOCK);
}

/* Отправка данных (TCP) */
int socket_send(Socket *sock, const void *data, size_t len) {
    return (int)send(sock->fd, data, len, 0);
}

/* Отправка данных с гарантией полной отправки (TCP) */
int socket_send_all(Socket *sock, const void *data, size_t len) {
    const uint8_t *buf = (const uint8_t *)data;
    size_t total_sent = 0;
    
    while (total_sent < len) {
        ssize_t sent = send(sock->fd, buf + total_sent, len - total_sent, 0);
        if (sent < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                /* В неблокирующем режиме сокет не готов */
                /* Ждём готовности сокета к записи */
                fd_set write_fds;
                FD_ZERO(&write_fds);
                FD_SET(sock->fd, &write_fds);
                
                struct timeval tv;
                tv.tv_sec = 5;  /* 5 секунд таймаут */
                tv.tv_usec = 0;
                
                int select_result = select(sock->fd + 1, NULL, &write_fds, NULL, &tv);
                if (select_result <= 0) {
                    /* Таймаут или ошибка */
                    return (total_sent > 0) ? (int)total_sent : -1;
                }
                /* Сокет готов, повторяем попытку */
                continue;
            }
            return -1;  /* Ошибка */
        }
        if (sent == 0) {
            /* Соединение закрыто */
            return -1;
        }
        total_sent += (size_t)sent;
    }
    
    return (int)total_sent;
}

/* Получение данных (TCP) */
int socket_recv(Socket *sock, void *buffer, size_t len) {
    return (int)recv(sock->fd, buffer, len, 0);
}

/* Отправка данных (UDP) */
int socket_sendto(Socket *sock, const void *data, size_t len, struct sockaddr_in *dest) {
    return (int)sendto(sock->fd, data, len, 0, (struct sockaddr *)dest, sizeof(*dest));
}

/* Получение данных (UDP) */
int socket_recvfrom(Socket *sock, void *buffer, size_t len, struct sockaddr_in *src) {
    socklen_t addr_len = sizeof(*src);
    return (int)recvfrom(sock->fd, buffer, len, 0, (struct sockaddr *)src, &addr_len);
}

/* Мультиплексирование сокетов с помощью select() */
int socket_select(Socket *sockets[], int count, int timeout_ms, int *ready_flags) {
    fd_set read_fds;
    FD_ZERO(&read_fds);
    
    int max_fd = 0;
    for (int i = 0; i < count; i++) {
        if (sockets[i] && sockets[i]->fd >= 0) {
            FD_SET(sockets[i]->fd, &read_fds);
            if (sockets[i]->fd > max_fd) {
                max_fd = sockets[i]->fd;
            }
        }
        if (ready_flags) ready_flags[i] = 0;
    }
    
    struct timeval tv;
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    
    int result = select(max_fd + 1, &read_fds, NULL, NULL, &tv);
    
    if (result > 0 && ready_flags) {
        for (int i = 0; i < count; i++) {
            if (sockets[i] && sockets[i]->fd >= 0 && FD_ISSET(sockets[i]->fd, &read_fds)) {
                ready_flags[i] = 1;
            }
        }
    }
    
    return result;
}

/* Проверка, есть ли данные для чтения */
int socket_has_data(Socket *sock, int timeout_ms) {
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(sock->fd, &read_fds);
    
    struct timeval tv;
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    
    return select(sock->fd + 1, &read_fds, NULL, NULL, &tv) > 0;
}

/* Проверка, готов ли сокет к записи */
int socket_is_writable(Socket *sock, int timeout_ms) {
    fd_set write_fds;
    FD_ZERO(&write_fds);
    FD_SET(sock->fd, &write_fds);
    
    struct timeval tv;
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    
    return select(sock->fd + 1, NULL, &write_fds, NULL, &tv) > 0;
}

/* Закрытие сокета */
void socket_close(Socket *sock) {
    if (sock->fd >= 0) {
        close(sock->fd);
        sock->fd = -1;
    }
}

/* Проверка валидности сокета */
int socket_is_valid(Socket *sock) {
    return sock && sock->fd >= 0;
}

/* Получение порта сокета */
int socket_get_port(Socket *sock) {
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    if (getsockname(sock->fd, (struct sockaddr *)&addr, &len) == 0) {
        return ntohs(addr.sin_port);
    }
    return -1;
}

