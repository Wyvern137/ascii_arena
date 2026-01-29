/*
 * renderer.h - Рендеринг игры
 * Отрисовка арены, меню и статуса
 */

#ifndef RENDERER_H
#define RENDERER_H

#include "../core/arena.h"
#include "../core/game.h"

/* Структура рендерера */
typedef struct {
    int screen_width;   /* Ширина экрана */
    int screen_height;  /* Высота экрана */
} Renderer;

/* Создание рендерера */
Renderer renderer_create(void);

/* Очистка экрана */
void renderer_clear(void);

/* Отрисовка арены */
void renderer_draw_arena(Renderer *renderer, Arena *arena, int offset_x, int offset_y);

/* Отрисовка меню */
void renderer_draw_menu(Renderer *renderer, const char *title, const char **options, int option_count, int selected);

/* Отрисовка экрана ожидания */
void renderer_draw_waiting(Renderer *renderer, int seconds_left, char *players, int player_count, int max_players);

/* Отрисовка статуса игроков (очки, здоровье) */
void renderer_draw_status(Renderer *renderer, Game *game, int offset_x, int offset_y);

/* Отрисовка экрана подключения */
void renderer_draw_connect(Renderer *renderer, const char *host, int port, char selected_char);

/* Отрисовка экрана окончания игры */
void renderer_draw_game_over(Renderer *renderer, char winner, int winner_points);

/* Отрисовка сообщения */
void renderer_draw_message(Renderer *renderer, const char *message);

/* Обновление экрана */
void renderer_refresh(void);

/* Освобождение ресурсов рендерера */
void renderer_destroy(Renderer *renderer);

#endif /* RENDERER_H */

