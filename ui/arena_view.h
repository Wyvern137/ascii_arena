/*
 * arena_view.h - Отображение арены
 * Структуры и функции для отрисовки игровой арены
 */

#ifndef ARENA_VIEW_H
#define ARENA_VIEW_H

#include <stdint.h>

/* Максимальное количество игроков */
#define MAX_ARENA_PLAYERS 8

/* Максимальное количество сущностей */
#define MAX_ARENA_ENTITIES 32

/* Максимальное количество заклинаний */
#define MAX_ARENA_SPELLS 64

/* Длина луча заклинания (5 кадров) */
#define SPELL_TRAIL_LENGTH 5

/* Размеры панели игрока */
#define PLAYER_PANEL_WIDTH 27
#define PLAYER_PANEL_HEIGHT 5

/* Данные сущности на арене */
typedef struct {
    int id;
    char symbol;
    int pos_x;
    int pos_y;
    int health;
    int max_health;
    int energy;
    int max_energy;
    int direction;      /* 0=up, 1=right, 2=down, 3=left */
    int spell_type;     /* 1=базовая, 2=усиленная */
    int is_player;
    int64_t damage_time; /* Время получения урона (для анимации) */
} ArenaEntity;

/* Данные заклинания */
typedef struct {
    int id;
    int pos_x;
    int pos_y;
    int prev_pos_x;     /* Предыдущая позиция X для интерполяции */
    int prev_pos_y;     /* Предыдущая позиция Y для интерполяции */
    int direction;      /* Направление движения (0=up, 1=down, 2=left, 3=right) */
    int spell_type;     /* Тип заклинания (1=базовая, 2=усиленная) */
    float interp_timer; /* Таймер интерполяции (0.0-1.0) */
    int active;
} ArenaSpell;

/* Данные игрока */
typedef struct {
    int id;
    char symbol;
    int points;
    int entity_id;      /* ID сущности или -1 если мёртв */
    int is_current_user;
} ArenaPlayer;

/* Состояние арены */
typedef struct {
    /* Информация об арене */
    int arena_number;
    int winner_points;
    int map_size;
    
    /* Игроки */
    ArenaPlayer players[MAX_ARENA_PLAYERS];
    int player_count;
    
    /* Сущности */
    ArenaEntity entities[MAX_ARENA_ENTITIES];
    int entity_count;
    
    /* Заклинания */
    ArenaSpell spells[MAX_ARENA_SPELLS];
    int spell_count;
    
    /* Текущий игрок */
    int current_player_id;
    int current_direction;
    int local_direction;    /* Направление на основе нажатых клавиш (для индикатора) */
    
    /* Состояние игры */
    int game_finished;
    char winner_symbol;
    
    /* Таймер до следующей арены */
    int next_arena_countdown;   /* -1 если не активен */
    char arena_winner_symbol;   /* Символ выжившего в арене */
} ArenaView;

/* Создание view арены */
ArenaView arena_view_create(int map_size, int winner_points);

/* Обновление арены (вызывается каждый кадр) */
void arena_view_update(ArenaView *view);

/* Обновление интерполяции заклинаний (вызывается каждый кадр с delta_time) */
void arena_view_update_interpolation(ArenaView *view, float delta_time);

/* Отрисовка арены */
void arena_view_render(ArenaView *view, int screen_width, int screen_height);

/* Установка номера арены */
void arena_view_set_arena_number(ArenaView *view, int number);

/* Добавление/обновление игрока */
void arena_view_set_player(ArenaView *view, int id, char symbol, int points, int entity_id, int is_current);

/* Очистка списка игроков */
void arena_view_clear_players(ArenaView *view);

/* Добавление/обновление сущности */
void arena_view_set_entity(ArenaView *view, int id, char symbol, int pos_x, int pos_y, 
                           int health, int max_health, int energy, int max_energy, 
                           int direction, int spell_type, int is_player);

/* Очистка списка сущностей */
void arena_view_clear_entities(ArenaView *view);

/* Добавление заклинания */
void arena_view_set_spell(ArenaView *view, int id, int pos_x, int pos_y, int direction, int spell_type, int active);

/* Очистка списка заклинаний */
void arena_view_clear_spells(ArenaView *view);

/* Установка текущего игрока */
void arena_view_set_current_player(ArenaView *view, int player_id, int direction);

/* Установка локального направления (на основе нажатых клавиш) */
void arena_view_set_local_direction(ArenaView *view, int direction);

/* Установка состояния окончания игры */
void arena_view_set_game_finished(ArenaView *view, char winner_symbol);

/* Установка таймера следующей арены */
void arena_view_set_next_arena(ArenaView *view, int countdown, char winner_symbol);

/* Нанесение урона сущности (для анимации) */
void arena_view_damage_entity(ArenaView *view, int entity_id);

/* Получение размеров арены для layout */
void arena_view_get_dimension(ArenaView *view, int *width, int *height);

#endif /* ARENA_VIEW_H */

