/*
 * encoder.h - Кодирование и декодирование сообщений
 * Сериализация структур в бинарный формат для передачи по сети
 */

#ifndef ENCODER_H
#define ENCODER_H

#include <stdint.h>
#include <stddef.h>
#include "protocol.h"
#include "../core/arena.h"
#include "../core/game.h"

/* === Функции кодирования (клиент → сервер) === */

/* Кодирование версии */
int encode_version(uint8_t *buffer, const char *version);

/* Кодирование подписки на информацию */
int encode_subscribe_info(uint8_t *buffer);

/* Кодирование входа */
int encode_login(uint8_t *buffer, char symbol);

/* Кодирование выхода */
int encode_logout(uint8_t *buffer);

/* Кодирование UDP handshake */
int encode_connect_udp(uint8_t *buffer, int32_t token);

/* Кодирование подтверждения UDP */
int encode_trust_udp(uint8_t *buffer);

/* Кодирование движения игрока */
int encode_move_player(uint8_t *buffer, Direction dir);

/* Кодирование применения способности (spell_type: 1 = базовая, 2 = усиленная) */
int encode_cast_skill(uint8_t *buffer, Direction dir, uint8_t spell_type);

/* === Функции кодирования (сервер → клиент) === */

/* Кодирование ответа версии */
int encode_version_response(uint8_t *buffer, const char *version, int compatible);

/* Кодирование статической информации */
int encode_static_info(uint8_t *buffer, int udp_port, int map_size, int winner_points, int max_players);

/* Кодирование динамической информации */
int encode_dynamic_info(uint8_t *buffer, char *symbols, int count);

/* Кодирование статуса входа */
int encode_login_status(uint8_t *buffer, char symbol, LoginStatus status, int32_t token);

/* Кодирование подтверждения UDP */
int encode_udp_connected(uint8_t *buffer);

/* Кодирование начала игры */
int encode_start_game(uint8_t *buffer, int winner_points);

/* Кодирование конца игры */
int encode_finish_game(uint8_t *buffer, char winner);

/* Кодирование ожидания арены */
int encode_wait_arena(uint8_t *buffer, int seconds);

/* Кодирование начала арены */
int encode_start_arena(uint8_t *buffer, Arena *arena, Game *game);

/* Кодирование кадра состояния */
int encode_game_step(uint8_t *buffer, Arena *arena, Game *game);

/* Кодирование игрового события */
int encode_game_event(uint8_t *buffer, char symbol, int points);

/* === Функции декодирования === */

/* Декодирование заголовка пакета */
int decode_packet_header(const uint8_t *buffer, PacketHeader *header);

/* Декодирование версии */
int decode_version(const uint8_t *buffer, char *version, size_t max_len);

/* Декодирование входа */
int decode_login(const uint8_t *buffer, char *symbol);

/* Декодирование UDP handshake */
int decode_connect_udp(const uint8_t *buffer, int32_t *token);

/* Декодирование движения игрока */
int decode_move_player(const uint8_t *buffer, Direction *dir);

/* Декодирование применения способности (spell_type: 1 = базовая, 2 = усиленная) */
int decode_cast_skill(const uint8_t *buffer, Direction *dir, uint8_t *spell_type);

/* Декодирование статуса входа */
int decode_login_status(const uint8_t *buffer, char *symbol, LoginStatus *status, int32_t *token);

/* Декодирование статической информации */
int decode_static_info(const uint8_t *buffer, int *udp_port, int *map_size, int *winner_points, int *max_players);

/* Декодирование кадра состояния */
int decode_game_step(const uint8_t *buffer, size_t len, EntityData *entities, int *entity_count,
                     SpellData *spells, int *spell_count, PlayerData *players, int *player_count);

#endif /* ENCODER_H */

