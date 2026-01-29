/*
 * game.c - Реализация игровой логики
 */

#include "game.h"
#include <stdlib.h>
#include <string.h>

/* Константы для размещения игроков */
#define SPAWN_OFFSET 2

/* Создание игры */
Game* game_create(int map_size, int winner_points, int max_players) {
    Game *game = (Game *)malloc(sizeof(Game));
    if (!game) return NULL;
    
    game->map_size = map_size;
    game->winner_points = winner_points;
    game->max_players = max_players;
    game->arena_number = 0;
    game->state = GAME_STATE_WAITING;
    game->arena = NULL;
    game->player_count = 0;
    
    /* Инициализируем массив игроков */
    for (int i = 0; i < MAX_PLAYERS; i++) {
        game->players[i] = player_create('\0');
    }
    
    return game;
}

/* Добавление игрока в игру */
int game_add_player(Game *game, char symbol) {
    if (game->player_count >= game->max_players) {
        return -1;
    }
    
    /* Проверяем, не занят ли символ */
    if (game_get_player_by_symbol(game, symbol) != NULL) {
        return -1;
    }
    
    int index = game->player_count;
    game->players[index] = player_create(symbol);
    game->players[index].connected = 1;
    game->player_count++;
    
    return index;
}

/* Удаление игрока из игры */
void game_remove_player(Game *game, int player_index) {
    if (player_index < 0 || player_index >= game->player_count) {
        return;
    }
    
    /* Сдвигаем остальных игроков */
    for (int i = player_index; i < game->player_count - 1; i++) {
        game->players[i] = game->players[i + 1];
    }
    game->player_count--;
}

/* Получение игрока по символу */
Player* game_get_player_by_symbol(Game *game, char symbol) {
    for (int i = 0; i < game->player_count; i++) {
        if (game->players[i].symbol == symbol) {
            return &game->players[i];
        }
    }
    return NULL;
}

/* Получение индекса игрока по символу */
int game_get_player_index(Game *game, char symbol) {
    for (int i = 0; i < game->player_count; i++) {
        if (game->players[i].symbol == symbol) {
            return i;
        }
    }
    return -1;
}

/* Генерация случайной позиции спавна на полу карты */
static Vec2 get_random_floor_position(Map *map, Vec2 *occupied, int occupied_count) {
    int attempts = 100;  /* Максимум попыток */
    
    while (attempts-- > 0) {
        /* Случайная позиция внутри карты (не на стенах) */
        int x = 1 + rand() % (map->size - 2);
        int y = 1 + rand() % (map->size - 2);
        Vec2 pos = vec2_create(x, y);
        
        /* Проверяем, что это пол */
        if (map_get_terrain(map, pos) != TERRAIN_FLOOR) {
            continue;
        }
        
        /* Проверяем, что позиция не занята */
        int is_occupied = 0;
        for (int i = 0; i < occupied_count; i++) {
            if (vec2_equals(pos, occupied[i])) {
                is_occupied = 1;
                break;
            }
        }
        
        if (!is_occupied) {
            return pos;
        }
    }
    
    /* Fallback: возвращаем центр */
    return vec2_create(map->size / 2, map->size / 2);
}

/* Создание новой арены */
void game_create_arena(Game *game) {
    /* Освобождаем старую арену */
    if (game->arena) {
        arena_destroy(game->arena);
        free(game->arena);
    }
    
    /* Создаём новую арену */
    game->arena = (Arena *)malloc(sizeof(Arena));
    *game->arena = arena_create(game->map_size);
    game->arena_number++;
    
    /* Массив занятых позиций для спавна */
    Vec2 occupied[MAX_PLAYERS];
    int occupied_count = 0;
    
    /* Добавляем сущности для всех игроков на случайных позициях */
    for (int i = 0; i < game->player_count; i++) {
        Player *player = &game->players[i];
        if (!player->connected) continue;
        
        Vec2 spawn = get_random_floor_position(&game->arena->map, occupied, occupied_count);
        occupied[occupied_count++] = spawn;
        
        int entity_id = arena_add_entity(game->arena, player->symbol, spawn, 100, 100);
        player->entity_id = entity_id;
    }
}

/* Подсчёт живых игроков на арене */
static int game_count_living_players(Game *game) {
    int count = 0;
    for (int i = 0; i < game->player_count; i++) {
        if (game->players[i].entity_id >= 0) {
            Entity *e = arena_get_entity(game->arena, game->players[i].entity_id);
            if (e && e->alive) {
                count++;
            }
        }
    }
    return count;
}

/* Шаг игры */
void game_step(Game *game, float delta_time) {
    if (game->state != GAME_STATE_PLAYING || !game->arena) {
        return;
    }
    
    /* Подсчитываем живых до обновления */
    int living_before = game_count_living_players(game);
    
    /* Обновляем арену */
    arena_update(game->arena, delta_time);
    
    /* Обновляем entity_id для мёртвых игроков */
    for (int i = 0; i < game->player_count; i++) {
        if (game->players[i].entity_id >= 0) {
            Entity *e = arena_get_entity(game->arena, game->players[i].entity_id);
            if (e && !e->alive) {
                game->players[i].entity_id = -1;
            }
        }
    }
    
    /* Подсчитываем живых после обновления */
    int living_after = game_count_living_players(game);
    int deleted = living_before - living_after;
    
    /* Выжившие получают очки за каждого убитого */
    if (deleted > 0) {
        for (int i = 0; i < game->player_count; i++) {
            if (game->players[i].entity_id >= 0) {
                Entity *e = arena_get_entity(game->arena, game->players[i].entity_id);
                if (e && e->alive) {
                    player_add_points(&game->players[i], deleted);
                }
            }
        }
    }
    
    /* Проверяем окончание арены (остался 1 или 0 живых) */
    if (living_after <= 1 && game->player_count > 1) {
        /* Проверяем победителя игры */
        if (game_has_winner(game)) {
            game->state = GAME_STATE_FINISHED;
        } else {
            /* Создаём новую арену */
            game_create_arena(game);
        }
    }
}

/* Проверка наличия победителя */
int game_has_winner(Game *game) {
    for (int i = 0; i < game->player_count; i++) {
        if (game->players[i].points >= game->winner_points) {
            return 1;
        }
    }
    return 0;
}

/* Получение символа победителя */
char game_get_winner(Game *game) {
    for (int i = 0; i < game->player_count; i++) {
        if (game->players[i].points >= game->winner_points) {
            return game->players[i].symbol;
        }
    }
    return '\0';
}

/* Проверка, готова ли игра к началу */
int game_is_ready(Game *game) {
    return game->player_count >= 2 && game->player_count <= game->max_players;
}

/* Начало игры */
void game_start(Game *game) {
    if (!game_is_ready(game)) {
        return;
    }
    
    game->state = GAME_STATE_PLAYING;
    game_create_arena(game);
}

/* Обработка смерти сущности */
void game_handle_entity_death(Game *game, int entity_id, int killer_entity_id) {
    /* Находим игрока-убийцу и начисляем очко */
    for (int i = 0; i < game->player_count; i++) {
        if (game->players[i].entity_id == killer_entity_id) {
            player_add_points(&game->players[i], 1);
            break;
        }
    }
    
    /* Помечаем игрока как мёртвого */
    for (int i = 0; i < game->player_count; i++) {
        if (game->players[i].entity_id == entity_id) {
            game->players[i].entity_id = -1;
            break;
        }
    }
}

/* Освобождение памяти игры */
void game_destroy(Game *game) {
    if (game->arena) {
        arena_destroy(game->arena);
        free(game->arena);
        game->arena = NULL;
    }
    free(game);
}

