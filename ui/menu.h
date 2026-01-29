/*
 * menu.h - Меню игры
 * Структуры и функции для отрисовки и обработки меню
 */

#ifndef MENU_H
#define MENU_H

#include <stdint.h>

/* Максимальная длина адреса сервера */
#define MAX_SERVER_ADDR_LEN 64

/* Максимальное количество игроков в waiting room */
#define MAX_WAITING_ROOM_PLAYERS 8

/* Размеры waiting room */
#define WAITING_ROOM_WIDTH 18
#define WAITING_ROOM_HEIGHT 5

/* Состояние игрока в waiting room */
typedef struct {
    char symbol;        /* Символ игрока */
    int pos_x;          /* Позиция X (в символах, не в пикселях) */
    int pos_y;          /* Позиция Y */
    int direction;      /* Направление: 0=up, 1=right, 2=down, 3=left */
    int64_t last_move;  /* Время последнего движения (мс) */
} WaitingRoomPlayer;

/* Состояние waiting room */
typedef struct {
    WaitingRoomPlayer players[MAX_WAITING_ROOM_PLAYERS];
    int player_count;
    int width;
    int height;
} WaitingRoom;

/* Поле ввода текста */
typedef struct {
    char content[MAX_SERVER_ADDR_LEN];
    int cursor_pos;
    int has_focus;
} InputText;

/* Поле ввода символа (A-Z) */
typedef struct {
    char content;       /* '\0' если не выбран */
    int has_focus;
} InputChar;

/* Статус подключения */
typedef enum {
    CONNECTION_NOT_CONNECTED,
    CONNECTION_CONNECTING,
    CONNECTION_CONNECTED,
    CONNECTION_NOT_FOUND,
    CONNECTION_LOST
} ConnectionStatus;

/* Статус логина */
typedef enum {
    LOGIN_NOT_LOGGED,
    LOGIN_LOGGED,
    LOGIN_INVALID_NAME,
    LOGIN_NAME_TAKEN,
    LOGIN_PLAYER_LIMIT
} LoginStatusType;

/* Информация о версии */
typedef struct {
    char version[32];
    int compatibility;  /* 0=None, 1=NotExact, 2=Fully */
} VersionInfo;

/* Информация о сервере */
typedef struct {
    int has_info;           /* Есть ли информация */
    int udp_port;
    int map_size;
    int winner_points;
    int max_players;
    int current_players;
    int udp_confirmed;      /* -1=checking, 0=no, 1=yes */
} ServerInfo;

/* Состояние меню */
typedef struct {
    /* Поля ввода */
    InputText server_addr_input;
    InputChar character_input;
    
    /* Waiting room */
    WaitingRoom waiting_room;
    
    /* Состояние подключения */
    ConnectionStatus connection_status;
    LoginStatusType login_status;
    
    /* Информация о сервере */
    VersionInfo version_info;
    int has_version_info;
    ServerInfo server_info;
    
    /* Список игроков */
    char logged_players[MAX_WAITING_ROOM_PLAYERS];
    int logged_players_count;
    
    /* Таймер до начала игры */
    int start_countdown;    /* -1 если не запущен */
} Menu;

/* Создание меню */
Menu menu_create(const char *default_addr, char default_char);

/* Обновление меню (вызывается каждый кадр) */
void menu_update(Menu *menu);

/* Обработка ввода клавиши */
int menu_handle_key(Menu *menu, int key);

/* Отрисовка меню */
void menu_render(Menu *menu, int screen_width, int screen_height);

/* Установка состояния подключения */
void menu_set_connection_status(Menu *menu, ConnectionStatus status);

/* Установка статуса логина */
void menu_set_login_status(Menu *menu, LoginStatusType status);

/* Установка информации о версии */
void menu_set_version_info(Menu *menu, const char *version, int compatibility);

/* Установка информации о сервере */
void menu_set_server_info(Menu *menu, int udp_port, int map_size, int winner_points, int max_players);

/* Установка списка игроков */
void menu_set_players(Menu *menu, const char *players, int count);

/* Установка таймера */
void menu_set_countdown(Menu *menu, int seconds);

/* Установка подтверждения UDP */
void menu_set_udp_confirmed(Menu *menu, int confirmed);

/* Получение адреса сервера */
const char* menu_get_server_addr(Menu *menu);

/* Получение символа игрока */
char menu_get_character(Menu *menu);

/* Проверка, залогинен ли пользователь */
int menu_is_logged(Menu *menu);

/* Проверка, подключен ли к серверу */
int menu_is_connected(Menu *menu);

/* Проверка, совместима ли версия */
int menu_has_compatible_version(Menu *menu);

/* Проверка, заполнен ли сервер */
int menu_is_server_full(Menu *menu);

#endif /* MENU_H */

