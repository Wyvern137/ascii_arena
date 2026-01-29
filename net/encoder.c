/*
 * encoder.c - Реализация кодирования/декодирования сообщений
 */

#include "encoder.h"
#include <string.h>

/* Версия протокола */
#define PROTOCOL_VERSION "1.0.0"

/* === Вспомогательные функции === */

/* Запись заголовка пакета */
static int write_header(uint8_t *buffer, uint8_t msg_type, uint16_t data_len) {
    buffer[0] = msg_type;
    buffer[1] = (uint8_t)(data_len & 0xFF);
    buffer[2] = (uint8_t)((data_len >> 8) & 0xFF);
    return PACKET_HEADER_SIZE;
}

/* === Функции кодирования (клиент → сервер) === */

int encode_version(uint8_t *buffer, const char *version) {
    size_t len = strlen(version);
    if (len > 32) len = 32;
    
    int offset = write_header(buffer, CLIENT_MSG_VERSION, (uint16_t)(len + 1));
    buffer[offset++] = (uint8_t)len;
    memcpy(buffer + offset, version, len);
    return offset + (int)len;
}

int encode_subscribe_info(uint8_t *buffer) {
    return write_header(buffer, CLIENT_MSG_SUBSCRIBE_INFO, 0);
}

int encode_login(uint8_t *buffer, char symbol) {
    int offset = write_header(buffer, CLIENT_MSG_LOGIN, 1);
    buffer[offset++] = (uint8_t)symbol;
    return offset;
}

int encode_logout(uint8_t *buffer) {
    return write_header(buffer, CLIENT_MSG_LOGOUT, 0);
}

int encode_connect_udp(uint8_t *buffer, int32_t token) {
    int offset = write_header(buffer, CLIENT_MSG_CONNECT_UDP, 4);
    memcpy(buffer + offset, &token, 4);
    return offset + 4;
}

int encode_trust_udp(uint8_t *buffer) {
    return write_header(buffer, CLIENT_MSG_TRUST_UDP, 0);
}

int encode_move_player(uint8_t *buffer, Direction dir) {
    int offset = write_header(buffer, CLIENT_MSG_MOVE_PLAYER, 1);
    buffer[offset++] = (uint8_t)dir;
    return offset;
}

int encode_cast_skill(uint8_t *buffer, Direction dir, uint8_t spell_type) {
    int offset = write_header(buffer, CLIENT_MSG_CAST_SKILL, 2);
    buffer[offset++] = (uint8_t)dir;
    buffer[offset++] = spell_type;
    return offset;
}

/* === Функции кодирования (сервер → клиент) === */

int encode_version_response(uint8_t *buffer, const char *version, int compatible) {
    size_t len = strlen(version);
    if (len > 32) len = 32;
    
    int offset = write_header(buffer, SERVER_MSG_VERSION, (uint16_t)(len + 2));
    buffer[offset++] = (uint8_t)len;
    memcpy(buffer + offset, version, len);
    offset += (int)len;
    buffer[offset++] = (uint8_t)compatible;
    return offset;
}

int encode_static_info(uint8_t *buffer, int udp_port, int map_size, int winner_points, int max_players) {
    int offset = write_header(buffer, SERVER_MSG_STATIC_INFO, 8);
    uint16_t port = (uint16_t)udp_port;
    uint16_t size = (uint16_t)map_size;
    uint16_t points = (uint16_t)winner_points;
    uint16_t players = (uint16_t)max_players;
    
    memcpy(buffer + offset, &port, 2); offset += 2;
    memcpy(buffer + offset, &size, 2); offset += 2;
    memcpy(buffer + offset, &points, 2); offset += 2;
    memcpy(buffer + offset, &players, 2); offset += 2;
    return offset;
}

int encode_dynamic_info(uint8_t *buffer, char *symbols, int count) {
    int offset = write_header(buffer, SERVER_MSG_DYNAMIC_INFO, (uint16_t)(count + 1));
    buffer[offset++] = (uint8_t)count;
    for (int i = 0; i < count; i++) {
        buffer[offset++] = (uint8_t)symbols[i];
    }
    return offset;
}

int encode_login_status(uint8_t *buffer, char symbol, LoginStatus status, int32_t token) {
    int offset = write_header(buffer, SERVER_MSG_LOGIN_STATUS, 6);
    buffer[offset++] = (uint8_t)symbol;
    buffer[offset++] = (uint8_t)status;
    memcpy(buffer + offset, &token, 4);
    return offset + 4;
}

int encode_udp_connected(uint8_t *buffer) {
    return write_header(buffer, SERVER_MSG_UDP_CONNECTED, 0);
}

int encode_start_game(uint8_t *buffer, int winner_points) {
    int offset = write_header(buffer, SERVER_MSG_START_GAME, 2);
    uint16_t points = (uint16_t)winner_points;
    memcpy(buffer + offset, &points, 2);
    return offset + 2;
}

int encode_finish_game(uint8_t *buffer, char winner) {
    int offset = write_header(buffer, SERVER_MSG_FINISH_GAME, 1);
    buffer[offset++] = (uint8_t)winner;
    return offset;
}

int encode_wait_arena(uint8_t *buffer, int seconds) {
    int offset = write_header(buffer, SERVER_MSG_WAIT_ARENA, 2);
    uint16_t secs = (uint16_t)seconds;
    memcpy(buffer + offset, &secs, 2);
    return offset + 2;
}

int encode_start_arena(uint8_t *buffer, Arena *arena, Game *game) {
    /* Формат: arena_number(2) + player_count(1) + map_size(2) + entity_ids... + map_data... */
    int offset = PACKET_HEADER_SIZE;
    
    uint16_t arena_num = (uint16_t)game->arena_number;
    memcpy(buffer + offset, &arena_num, 2); offset += 2;
    
    buffer[offset++] = (uint8_t)game->player_count;
    
    uint16_t map_size = (uint16_t)arena->map.size;
    memcpy(buffer + offset, &map_size, 2); offset += 2;
    
    /* Entity IDs для каждого игрока */
    for (int i = 0; i < game->player_count; i++) {
        int32_t eid = game->players[i].entity_id;
        memcpy(buffer + offset, &eid, 4); offset += 4;
    }
    
    /* Данные карты */
    int map_data_size = arena->map.size * arena->map.size;
    for (int i = 0; i < map_data_size; i++) {
        buffer[offset++] = (uint8_t)arena->map.ground[i];
    }
    
    /* Записываем заголовок */
    write_header(buffer, SERVER_MSG_START_ARENA, (uint16_t)(offset - PACKET_HEADER_SIZE));
    return offset;
}

int encode_game_step(uint8_t *buffer, Arena *arena, Game *game) {
    int offset = PACKET_HEADER_SIZE;
    
    /* Количество сущностей и заклинаний */
    uint8_t entity_count = (uint8_t)arena->entity_count;
    uint8_t spell_count = (uint8_t)arena->spell_count;
    uint8_t player_count = (uint8_t)game->player_count;
    
    buffer[offset++] = entity_count;
    buffer[offset++] = spell_count;
    buffer[offset++] = player_count;
    
    /* Данные сущностей */
    for (int i = 0; i < arena->entity_count; i++) {
        Entity *e = &arena->entities[i];
        EntityData data;
        data.id = e->id;
        data.symbol = e->symbol;
        data.pos_x = (int16_t)e->position.x;
        data.pos_y = (int16_t)e->position.y;
        data.health = (uint8_t)e->health;
        data.energy = (uint8_t)e->energy;
        data.direction = (uint8_t)e->direction;
        data.spell_type = (uint8_t)e->spell_type;
        memcpy(buffer + offset, &data, ENTITY_DATA_SIZE);
        offset += ENTITY_DATA_SIZE;
    }
    
    /* Данные заклинаний */
    for (int i = 0; i < arena->spell_count; i++) {
        Spell *s = &arena->spells[i];
        SpellData data;
        data.id = s->id;
        data.pos_x = (int16_t)s->position.x;
        data.pos_y = (int16_t)s->position.y;
        data.direction = (uint8_t)s->direction;
        data.spell_type = (uint8_t)s->spell_type;
        memcpy(buffer + offset, &data, SPELL_DATA_SIZE);
        offset += SPELL_DATA_SIZE;
    }
    
    /* Данные игроков */
    for (int i = 0; i < game->player_count; i++) {
        Player *p = &game->players[i];
        PlayerData data;
        data.symbol = p->symbol;
        data.points = (uint16_t)p->points;
        memcpy(buffer + offset, &data, PLAYER_DATA_SIZE);
        offset += PLAYER_DATA_SIZE;
    }
    
    /* Записываем заголовок */
    write_header(buffer, SERVER_MSG_GAME_STEP, (uint16_t)(offset - PACKET_HEADER_SIZE));
    return offset;
}

int encode_game_event(uint8_t *buffer, char symbol, int points) {
    int offset = write_header(buffer, SERVER_MSG_GAME_EVENT, 3);
    buffer[offset++] = (uint8_t)symbol;
    uint16_t pts = (uint16_t)points;
    memcpy(buffer + offset, &pts, 2);
    return offset + 2;
}

/* === Функции декодирования === */

int decode_packet_header(const uint8_t *buffer, PacketHeader *header) {
    header->message_type = buffer[0];
    header->data_length = (uint16_t)(buffer[1] | (buffer[2] << 8));
    return PACKET_HEADER_SIZE;
}

int decode_version(const uint8_t *buffer, char *version, size_t max_len) {
    uint8_t len = buffer[0];
    if (len >= max_len) len = (uint8_t)(max_len - 1);
    memcpy(version, buffer + 1, len);
    version[len] = '\0';
    return 1 + len;
}

int decode_login(const uint8_t *buffer, char *symbol) {
    *symbol = (char)buffer[0];
    return 1;
}

int decode_connect_udp(const uint8_t *buffer, int32_t *token) {
    memcpy(token, buffer, 4);
    return 4;
}

int decode_move_player(const uint8_t *buffer, Direction *dir) {
    *dir = (Direction)buffer[0];
    return 1;
}

int decode_cast_skill(const uint8_t *buffer, Direction *dir, uint8_t *spell_type) {
    *dir = (Direction)buffer[0];
    *spell_type = buffer[1];
    return 2;
}

int decode_login_status(const uint8_t *buffer, char *symbol, LoginStatus *status, int32_t *token) {
    *symbol = (char)buffer[0];
    *status = (LoginStatus)buffer[1];
    memcpy(token, buffer + 2, 4);
    return 6;
}

int decode_static_info(const uint8_t *buffer, int *udp_port, int *map_size, int *winner_points, int *max_players) {
    uint16_t port, size, points, players;
    memcpy(&port, buffer, 2);
    memcpy(&size, buffer + 2, 2);
    memcpy(&points, buffer + 4, 2);
    memcpy(&players, buffer + 6, 2);
    *udp_port = port;
    *map_size = size;
    *winner_points = points;
    *max_players = players;
    return 8;
}

int decode_game_step(const uint8_t *buffer, size_t len, EntityData *entities, int *entity_count,
                     SpellData *spells, int *spell_count, PlayerData *players, int *player_count) {
    int offset = 0;
    
    *entity_count = buffer[offset++];
    *spell_count = buffer[offset++];
    *player_count = buffer[offset++];
    
    /* Читаем сущности */
    for (int i = 0; i < *entity_count && (size_t)offset + ENTITY_DATA_SIZE <= len; i++) {
        memcpy(&entities[i], buffer + offset, ENTITY_DATA_SIZE);
        offset += ENTITY_DATA_SIZE;
    }
    
    /* Читаем заклинания */
    for (int i = 0; i < *spell_count && (size_t)offset + SPELL_DATA_SIZE <= len; i++) {
        memcpy(&spells[i], buffer + offset, SPELL_DATA_SIZE);
        offset += SPELL_DATA_SIZE;
    }
    
    /* Читаем игроков */
    for (int i = 0; i < *player_count && (size_t)offset + PLAYER_DATA_SIZE <= len; i++) {
        memcpy(&players[i], buffer + offset, PLAYER_DATA_SIZE);
        offset += PLAYER_DATA_SIZE;
    }
    
    return offset;
}

