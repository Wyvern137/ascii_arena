/*
 * state.h - Состояние клиента
 * Управление состоянием клиентского приложения
 */

#ifndef CLIENT_STATE_H
#define CLIENT_STATE_H

#include "../core/game.h"
#include "../net/socket.h"
#include "../net/protocol.h"

/* Состояния клиента */
typedef enum {
    CLIENT_STATE_MENU,          /* Меню подключения */
    CLIENT_STATE_CONNECTING,    /* Подключение к серверу */
    CLIENT_STATE_WAITING,       /* Ожидание игроков */
    CLIENT_STATE_PLAYING,       /* Игра */
    CLIENT_STATE_GAME_OVER,     /* Конец игры */
    CLIENT_STATE_DISCONNECTED   /* Отключён */
} ClientStateType;

/* Размер буфера для TCP потока */
#define TCP_BUFFER_SIZE (MAX_PACKET_SIZE * 2)

/* Состояние клиента */
typedef struct {
    ClientStateType state;      /* Текущее состояние */
    
    /* Настройки подключения */
    char host[256];             /* Адрес сервера */
    int tcp_port;               /* TCP порт */
    int udp_port;               /* UDP порт */
    char player_symbol;         /* Символ игрока */
    int32_t session_token;      /* Токен сессии */
    
    /* Сетевые соединения */
    Socket tcp_socket;          /* TCP сокет */
    Socket udp_socket;          /* UDP сокет */
    int udp_connected;          /* Флаг UDP соединения */
    
    /* Буфер для TCP потока (обработка частичных пакетов) */
    uint8_t tcp_buffer[TCP_BUFFER_SIZE];
    size_t tcp_buffer_len;      /* Текущая длина данных в буфере */
    
    /* Информация о сервере */
    int map_size;               /* Размер карты */
    int winner_points;          /* Очки для победы */
    int max_players;            /* Максимум игроков */
    
    /* Игровые данные */
    char players[MAX_PLAYERS];  /* Символы игроков */
    int player_count;           /* Количество игроков */
    int wait_seconds;           /* Секунды до начала */
    
    /* Данные кадра */
    EntityData entities[MAX_ENTITIES];
    int entity_count;
    SpellData spells[MAX_SPELLS];
    int spell_count;
    PlayerData player_data[MAX_PLAYERS];
    int player_data_count;
    
    /* Результат игры */
    char winner;                /* Победитель */
    
    /* Выбранный тип заклинания (1 = базовая, 2 = усиленная) */
    uint8_t selected_spell_type;
    
    /* Последнее нажатое направление (для индикатора) */
    int last_direction;         /* 0=up, 1=right, 2=down, 3=left */
} ClientState;

/* Создание состояния клиента */
ClientState client_state_create(void);

/* Установка состояния */
void client_state_set(ClientState *state, ClientStateType new_state);

/* Сброс состояния */
void client_state_reset(ClientState *state);

/* Освобождение ресурсов */
void client_state_destroy(ClientState *state);

#endif /* CLIENT_STATE_H */

