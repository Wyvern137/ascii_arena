/*
 * game.h - Игровая логика
 * Управляет игроками, аренами и подсчётом очков
 */

#ifndef GAME_H
#define GAME_H

#include "arena.h"
#include "player.h"
#include "character.h"

/* Состояние игры */
typedef enum {
    GAME_STATE_WAITING,     /* Ожидание игроков */
    GAME_STATE_PLAYING,     /* Игра идёт */
    GAME_STATE_FINISHED     /* Игра завершена */
} GameState;

/* Игра */
typedef struct {
    int map_size;               /* Размер карты */
    int winner_points;          /* Очки для победы */
    int arena_number;           /* Номер текущей арены */
    GameState state;            /* Состояние игры */
    Arena *arena;               /* Текущая арена (NULL если нет) */
    Player players[MAX_PLAYERS]; /* Массив игроков */
    int player_count;           /* Количество игроков */
    int max_players;            /* Максимальное количество игроков */
} Game;

/* Создание игры */
Game* game_create(int map_size, int winner_points, int max_players);

/* Добавление игрока в игру, возвращает индекс игрока или -1 */
int game_add_player(Game *game, char symbol);

/* Удаление игрока из игры */
void game_remove_player(Game *game, int player_index);

/* Получение игрока по символу */
Player* game_get_player_by_symbol(Game *game, char symbol);

/* Получение индекса игрока по символу */
int game_get_player_index(Game *game, char symbol);

/* Создание новой арены */
void game_create_arena(Game *game);

/* Шаг игры (обновление арены, подсчёт очков) */
void game_step(Game *game, float delta_time);

/* Проверка наличия победителя */
int game_has_winner(Game *game);

/* Получение символа победителя (0 если нет) */
char game_get_winner(Game *game);

/* Проверка, готова ли игра к началу */
int game_is_ready(Game *game);

/* Начало игры */
void game_start(Game *game);

/* Обработка смерти сущности */
void game_handle_entity_death(Game *game, int entity_id, int killer_entity_id);

/* Освобождение памяти игры */
void game_destroy(Game *game);

#endif /* GAME_H */

