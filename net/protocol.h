/*
 * protocol.h - Сетевой протокол
 * Определяет типы сообщений между клиентом и сервером
 */

#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>
#include "../core/direction.h"
#include "../core/vec2.h"

/* Типы сообщений от клиента к серверу */
typedef enum {
    CLIENT_MSG_VERSION = 0,         /* Проверка версии */
    CLIENT_MSG_SUBSCRIBE_INFO = 1,  /* Подписка на информацию сервера */
    CLIENT_MSG_LOGIN = 2,           /* Вход с символом */
    CLIENT_MSG_LOGOUT = 3,          /* Выход */
    CLIENT_MSG_CONNECT_UDP = 4,     /* UDP handshake */
    CLIENT_MSG_TRUST_UDP = 5,       /* Подтверждение UDP */
    CLIENT_MSG_MOVE_PLAYER = 6,     /* Движение игрока */
    CLIENT_MSG_CAST_SKILL = 7       /* Применение способности */
} ClientMessageType;

/* Типы сообщений от сервера к клиенту */
typedef enum {
    SERVER_MSG_VERSION = 0,         /* Ответ версии */
    SERVER_MSG_STATIC_INFO = 1,     /* Статическая информация */
    SERVER_MSG_DYNAMIC_INFO = 2,    /* Динамическая информация (игроки) */
    SERVER_MSG_LOGIN_STATUS = 3,    /* Результат входа */
    SERVER_MSG_UDP_CONNECTED = 4,   /* Подтверждение UDP */
    SERVER_MSG_START_GAME = 5,      /* Начало игры */
    SERVER_MSG_FINISH_GAME = 6,     /* Конец игры */
    SERVER_MSG_WAIT_ARENA = 7,      /* Ожидание арены */
    SERVER_MSG_START_ARENA = 8,     /* Начало арены */
    SERVER_MSG_GAME_STEP = 9,       /* Кадр состояния */
    SERVER_MSG_GAME_EVENT = 10      /* Игровое событие */
} ServerMessageType;

/* Статус входа */
typedef enum {
    LOGIN_OK = 0,
    LOGIN_INVALID_CHAR = 1,
    LOGIN_ALREADY_USED = 2,
    LOGIN_ROOM_FULL = 3
} LoginStatus;

/* Заголовок пакета */
typedef struct __attribute__((packed)) {
    uint8_t message_type;   /* Тип сообщения */
    uint16_t data_length;   /* Длина данных после заголовка */
} PacketHeader;

/* Данные сущности для сериализации */
typedef struct __attribute__((packed)) {
    int32_t id;             /* ID сущности */
    char symbol;            /* Символ персонажа */
    int16_t pos_x;          /* Позиция X */
    int16_t pos_y;          /* Позиция Y */
    uint8_t health;         /* Здоровье (0-100) */
    uint8_t energy;         /* Энергия (0-100) */
    uint8_t direction;      /* Направление */
    uint8_t spell_type;     /* Тип заклинания (1 = базовая, 2 = усиленная) */
} EntityData;

/* Данные заклинания для сериализации */
typedef struct __attribute__((packed)) {
    int32_t id;             /* ID заклинания */
    int16_t pos_x;          /* Позиция X */
    int16_t pos_y;          /* Позиция Y */
    uint8_t direction;      /* Направление */
    uint8_t spell_type;     /* Тип заклинания (1 = базовая, 2 = усиленная) */
} SpellData;

/* Данные игрока для сериализации */
typedef struct __attribute__((packed)) {
    char symbol;            /* Символ игрока */
    uint16_t points;        /* Очки */
} PlayerData;

/* Размеры пакетов */
#define PACKET_HEADER_SIZE sizeof(PacketHeader)
#define ENTITY_DATA_SIZE sizeof(EntityData)
#define SPELL_DATA_SIZE sizeof(SpellData)
#define PLAYER_DATA_SIZE sizeof(PlayerData)

/* Максимальный размер пакета */
#define MAX_PACKET_SIZE 4096

#endif /* PROTOCOL_H */

