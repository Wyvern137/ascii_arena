/*
 * session.c - Реализация управления сессиями
 */

#include "session.h"
#include <string.h>

/* Создание комнаты */
RoomSession room_session_create(int max_players) {
    RoomSession room;
    memset(&room, 0, sizeof(room));
    room.max_players = max_players;
    room.session_count = 0;
    room.next_token = 1;
    
    for (int i = 0; i < MAX_PLAYERS; i++) {
        room.sessions[i].active = 0;
        room.sessions[i].tcp_socket.fd = -1;
        room.sessions[i].tcp_buffer_len = 0;
    }
    
    return room;
}

/* Добавление сессии */
int room_session_add(RoomSession *room, char symbol, Socket tcp_socket) {
    if (room_session_is_full(room)) {
        return -1;
    }
    
    /* Проверяем, не занят ли символ */
    if (room_session_find_by_symbol(room, symbol) != NULL) {
        return -1;
    }
    
    /* Ищем свободный слот */
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (!room->sessions[i].active) {
            Session *s = &room->sessions[i];
            s->token = room->next_token++;
            s->symbol = symbol;
            s->tcp_socket = tcp_socket;
            s->udp_connected = 0;
            s->active = 1;
            s->tcp_buffer_len = 0;  /* Инициализация TCP буфера */
            memset(&s->udp_addr, 0, sizeof(s->udp_addr));
            
            room->session_count++;
            return s->token;
        }
    }
    
    return -1;
}

/* Поиск сессии по токену */
Session* room_session_find_by_token(RoomSession *room, int token) {
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (room->sessions[i].active && room->sessions[i].token == token) {
            return &room->sessions[i];
        }
    }
    return NULL;
}

/* Поиск сессии по символу */
Session* room_session_find_by_symbol(RoomSession *room, char symbol) {
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (room->sessions[i].active && room->sessions[i].symbol == symbol) {
            return &room->sessions[i];
        }
    }
    return NULL;
}

/* Поиск сессии по TCP сокету */
Session* room_session_find_by_socket(RoomSession *room, int socket_fd) {
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (room->sessions[i].active && room->sessions[i].tcp_socket.fd == socket_fd) {
            return &room->sessions[i];
        }
    }
    return NULL;
}

/* Удаление сессии */
void room_session_remove(RoomSession *room, int token) {
    Session *s = room_session_find_by_token(room, token);
    if (s) {
        socket_close(&s->tcp_socket);
        s->active = 0;
        s->tcp_buffer_len = 0;  /* Очистка TCP буфера */
        room->session_count--;
    }
}

/* Проверка заполненности */
int room_session_is_full(RoomSession *room) {
    return room->session_count >= room->max_players;
}

/* Получение списка символов игроков */
int room_session_get_symbols(RoomSession *room, char *symbols) {
    int count = 0;
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (room->sessions[i].active) {
            symbols[count++] = room->sessions[i].symbol;
        }
    }
    return count;
}

/* Освобождение ресурсов */
void room_session_destroy(RoomSession *room) {
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (room->sessions[i].active) {
            socket_close(&room->sessions[i].tcp_socket);
            room->sessions[i].active = 0;
        }
    }
    room->session_count = 0;
}

