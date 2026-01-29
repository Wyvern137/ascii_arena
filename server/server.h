/*
 * server.h - Серверная логика
 * Главный цикл сервера, обработка подключений и сообщений
 */

#ifndef SERVER_H
#define SERVER_H

#include "session.h"
#include "../core/game.h"
#include "../net/socket.h"

/* Сервер */
typedef struct {
    Socket tcp_listener;    /* TCP listener */
    Socket udp_socket;      /* UDP сокет */
    RoomSession room;       /* Комната с сессиями */
    Game *game;             /* Игра */
    int tcp_port;           /* TCP порт */
    int udp_port;           /* UDP порт */
    int running;            /* Флаг работы */
} Server;

/* Создание сервера */
Server* server_create(int tcp_port, int udp_port, int max_players, int map_size, int winner_points);

/* Главный цикл сервера */
void server_run(Server *server);

/* Обработка нового подключения */
void server_handle_connection(Server *server);

/* Обработка сообщения от клиента */
void server_handle_message(Server *server, Session *session, uint8_t *data, int len);

/* Обработка UDP сообщения */
void server_handle_udp(Server *server, uint8_t *data, int len, struct sockaddr_in *src);

/* Рассылка GameStep всем клиентам */
void server_broadcast_game_step(Server *server);

/* Рассылка сообщения всем клиентам */
void server_broadcast(Server *server, uint8_t *data, int len);

/* Остановка сервера */
void server_stop(Server *server);

/* Освобождение ресурсов */
void server_destroy(Server *server);

#endif /* SERVER_H */

