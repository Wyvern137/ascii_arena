/*
 * session.h - Управление сессиями игроков
 * Хранение информации о подключённых клиентах
 */

#ifndef SESSION_H
#define SESSION_H

#include "../net/socket.h"
#include "../net/protocol.h"
#include "../core/player.h"

/* Размер буфера для TCP потока */
#define SESSION_TCP_BUFFER_SIZE (MAX_PACKET_SIZE * 2)

/* Сессия игрока */
typedef struct {
    int token;              /* Уникальный токен сессии */
    char symbol;            /* Символ игрока */
    Socket tcp_socket;      /* TCP соединение */
    struct sockaddr_in udp_addr;  /* UDP адрес клиента */
    int udp_connected;      /* Флаг UDP соединения */
    int active;             /* Флаг активности */
    
    /* Буфер для TCP потока (обработка частичных пакетов) */
    uint8_t tcp_buffer[SESSION_TCP_BUFFER_SIZE];
    size_t tcp_buffer_len;  /* Текущая длина данных в буфере */
} Session;

/* Комната с сессиями */
typedef struct {
    Session sessions[MAX_PLAYERS];  /* Массив сессий */
    int session_count;              /* Количество активных сессий */
    int max_players;                /* Максимум игроков */
    int next_token;                 /* Следующий токен */
} RoomSession;

/* Создание комнаты */
RoomSession room_session_create(int max_players);

/* Добавление сессии, возвращает токен или -1 */
int room_session_add(RoomSession *room, char symbol, Socket tcp_socket);

/* Поиск сессии по токену */
Session* room_session_find_by_token(RoomSession *room, int token);

/* Поиск сессии по символу */
Session* room_session_find_by_symbol(RoomSession *room, char symbol);

/* Поиск сессии по TCP сокету */
Session* room_session_find_by_socket(RoomSession *room, int socket_fd);

/* Удаление сессии */
void room_session_remove(RoomSession *room, int token);

/* Проверка заполненности */
int room_session_is_full(RoomSession *room);

/* Получение списка символов игроков */
int room_session_get_symbols(RoomSession *room, char *symbols);

/* Освобождение ресурсов */
void room_session_destroy(RoomSession *room);

#endif /* SESSION_H */

