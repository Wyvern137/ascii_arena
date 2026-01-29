/*
 * renderer.c - Реализация рендеринга
 * Двойная ширина, белые игроки, панели с прогресс-барами
 */

#include "renderer.h"
#include "terminal.h"
#include "widgets.h"
#include <ncurses.h>
#include <string.h>
#include <stdio.h>

/* Константы панели игрока */
#define PANEL_WIDTH 22
#define PANEL_HEIGHT 5
#define PROGRESS_BAR_WIDTH 10

/* Создание рендерера */
Renderer renderer_create(void) {
    Renderer r;
    terminal_get_size(&r.screen_width, &r.screen_height);
    return r;
}

/* Очистка экрана - используем erase() вместо clear() для уменьшения мерцания */
void renderer_clear(void) {
    erase();
}

/* Отрисовка прогресс-бара */
static void draw_progress_bar(int x, int y, int width, int current, int max, int color) {
    if (max <= 0) max = 1;
    int filled = (current * width) / max;
    if (filled > width) filled = width;
    
    attron(COLOR_PAIR(color) | A_BOLD);
    for (int i = 0; i < filled; i++) {
        mvaddch(y, x + i, '=');
    }
    attroff(A_BOLD);
    
    attron(COLOR_PAIR(color));
    for (int i = filled; i < width; i++) {
        mvaddch(y, x + i, '-');
    }
    attroff(COLOR_PAIR(color));
}

/* Отрисовка панели игрока */
static void draw_player_panel(int x, int y, char symbol, int points, int health, int max_health, 
                              int energy, int max_energy, int is_current, int is_alive) {
    int panel_color = is_current ? COLOR_PANEL_ACTIVE : COLOR_PANEL_INACTIVE;
    
    /* Рамка панели */
    attron(COLOR_PAIR(panel_color));
    widget_draw_box(x, y, PANEL_WIDTH, PANEL_HEIGHT);
    attroff(COLOR_PAIR(panel_color));
    
    /* Рамка для символа игрока */
    attron(COLOR_PAIR(panel_color));
    mvaddch(y + 1, x + 1, ACS_ULCORNER);
    mvaddch(y + 1, x + 4, ACS_URCORNER);
    mvaddch(y + 3, x + 1, ACS_LLCORNER);
    mvaddch(y + 3, x + 4, ACS_LRCORNER);
    mvaddch(y + 1, x + 2, ACS_HLINE);
    mvaddch(y + 1, x + 3, ACS_HLINE);
    mvaddch(y + 3, x + 2, ACS_HLINE);
    mvaddch(y + 3, x + 3, ACS_HLINE);
    mvaddch(y + 2, x + 1, ACS_VLINE);
    mvaddch(y + 2, x + 4, ACS_VLINE);
    attroff(COLOR_PAIR(panel_color));
    
    /* Символ игрока (белый с BOLD, или серый если мёртв) */
    if (is_alive) {
        attron(COLOR_PAIR(COLOR_PLAYER_WHITE) | A_BOLD);
    } else {
        attron(COLOR_PAIR(COLOR_PANEL_INACTIVE));
    }
    mvaddch(y + 2, x + 2, symbol);
    mvaddch(y + 2, x + 3, ' ');
    attroff(A_BOLD | A_COLOR);
    
    /* Очки */
    char buf[16];
    snprintf(buf, sizeof(buf), "%d", points);
    attron(COLOR_PAIR(COLOR_TEXT_WHITE));
    mvprintw(y + 1, x + 6, "Score: %s", buf);
    attroff(COLOR_PAIR(COLOR_TEXT_WHITE));
    
    /* Прогресс-бар здоровья */
    draw_progress_bar(x + 6, y + 2, PROGRESS_BAR_WIDTH, health, max_health, COLOR_HEALTH);
    
    /* Прогресс-бар энергии */
    draw_progress_bar(x + 6, y + 3, PROGRESS_BAR_WIDTH, energy, max_energy, COLOR_ENERGY);
}

/* Отрисовка арены (с двойной шириной) */
void renderer_draw_arena(Renderer *renderer, Arena *arena, int offset_x, int offset_y) {
    (void)renderer;
    
    /* Отрисовка карты с двойной шириной */
    for (int y = 0; y < arena->map.size; y++) {
        for (int x = 0; x < arena->map.size; x++) {
            Vec2 pos = vec2_create(x, y);
            Terrain terrain = map_get_terrain(&arena->map, pos);
            int screen_x = offset_x + x * 2;  /* Двойная ширина */
            int screen_y = offset_y + y;
            
            if (terrain == TERRAIN_WALL) {
                attron(COLOR_PAIR(COLOR_WALL) | A_BOLD);
                mvaddch(screen_y, screen_x, '#');
                mvaddch(screen_y, screen_x + 1, '#');
                attroff(COLOR_PAIR(COLOR_WALL) | A_BOLD);
            } else {
                attron(COLOR_PAIR(COLOR_FLOOR));
                mvaddch(screen_y, screen_x, '.');
                mvaddch(screen_y, screen_x + 1, ' ');
                attroff(COLOR_PAIR(COLOR_FLOOR));
            }
        }
    }
    
    /* Отрисовка заклинаний (символ 'o', оранжевый/жёлтый цвет) */
    for (int i = 0; i < arena->spell_count; i++) {
        Spell *spell = &arena->spells[i];
        if (!spell->destroyed) {
            int screen_x = offset_x + spell->position.x * 2;
            int screen_y = offset_y + spell->position.y;
            
            attron(COLOR_PAIR(COLOR_SPELL) | A_BOLD);
            mvaddch(screen_y, screen_x, 'o');
            mvaddch(screen_y, screen_x + 1, ' ');
            attroff(COLOR_PAIR(COLOR_SPELL) | A_BOLD);
        }
    }
    
    /* Отрисовка сущностей (все белые с BOLD, красные при уроне) */
    for (int i = 0; i < arena->entity_count; i++) {
        Entity *entity = &arena->entities[i];
        if (entity->alive) {
            int screen_x = offset_x + entity->position.x * 2;
            int screen_y = offset_y + entity->position.y;
            
            /* Цвет: красный при уроне, иначе белый */
            int is_damaged = entity->damage_timer > 0;
            int color = terminal_get_player_color(is_damaged);
            
            attron(COLOR_PAIR(color) | A_BOLD);
            mvaddch(screen_y, screen_x, entity->symbol);
            mvaddch(screen_y, screen_x + 1, ' ');
            attroff(COLOR_PAIR(color) | A_BOLD);
        }
    }
}

/* Отрисовка меню */
void renderer_draw_menu(Renderer *renderer, const char *title, const char **options, int option_count, int selected) {
    int menu_width = 40;
    int menu_height = option_count + 6;
    int start_x = (renderer->screen_width - menu_width) / 2;
    int start_y = (renderer->screen_height - menu_height) / 2;
    
    widget_draw_box(start_x, start_y, menu_width, menu_height);
    widget_draw_text_centered(start_x, start_y + 2, menu_width, title);
    
    for (int i = 0; i < option_count; i++) {
        int y = start_y + 4 + i;
        if (i == selected) {
            attron(A_REVERSE);
        }
        widget_draw_text_centered(start_x, y, menu_width, options[i]);
        if (i == selected) {
            attroff(A_REVERSE);
        }
    }
}

/* Отрисовка экрана ожидания */
void renderer_draw_waiting(Renderer *renderer, int seconds_left, char *players, int player_count, int max_players) {
    int box_width = 40;
    int box_height = 10 + player_count;
    int start_x = (renderer->screen_width - box_width) / 2;
    int start_y = (renderer->screen_height - box_height) / 2;
    
    widget_draw_box(start_x, start_y, box_width, box_height);
    widget_draw_text_centered(start_x, start_y + 2, box_width, "Ожидание игроков");
    
    char buf[64];
    snprintf(buf, sizeof(buf), "Игроков: %d/%d", player_count, max_players);
    widget_draw_text_centered(start_x, start_y + 4, box_width, buf);
    
    /* Список игроков (все белым цветом) */
    for (int i = 0; i < player_count; i++) {
        snprintf(buf, sizeof(buf), "Игрок %d: %c", i + 1, players[i]);
        attron(COLOR_PAIR(COLOR_PLAYER_WHITE) | A_BOLD);
        widget_draw_text_centered(start_x, start_y + 6 + i, box_width, buf);
        attroff(COLOR_PAIR(COLOR_PLAYER_WHITE) | A_BOLD);
    }
    
    if (seconds_left > 0) {
        snprintf(buf, sizeof(buf), "Начало через: %d сек", seconds_left);
        widget_draw_text_centered(start_x, start_y + box_height - 2, box_width, buf);
    }
}

/* Отрисовка статуса игроков с панелями */
void renderer_draw_status(Renderer *renderer, Game *game, int offset_x, int offset_y) {
    (void)renderer;
    
    /* Отрисовка панелей для каждого игрока */
    for (int i = 0; i < game->player_count; i++) {
        Player *player = &game->players[i];
        int panel_y = offset_y + i * (PANEL_HEIGHT + 1);
        
        int health = 0, max_health = 100, energy = 0, max_energy = 100;
        int is_alive = 0;
        
        if (game->arena && player->entity_id >= 0) {
            Entity *entity = arena_get_entity(game->arena, player->entity_id);
            if (entity) {
                health = entity->health;
                max_health = entity->max_health;
                energy = entity->energy;
                max_energy = entity->max_energy;
                is_alive = entity->alive;
            }
        }
        
        /* is_current = 1 для первого игрока (можно передавать текущего игрока) */
        draw_player_panel(offset_x, panel_y, player->symbol, player->points,
                         health, max_health, energy, max_energy, i == 0, is_alive);
    }
    
    /* Информация об арене */
    int info_y = offset_y + game->player_count * (PANEL_HEIGHT + 1) + 1;
    char buf[64];
    snprintf(buf, sizeof(buf), "Арена: %d", game->arena_number);
    widget_draw_text(offset_x, info_y, buf);
    snprintf(buf, sizeof(buf), "До победы: %d очков", game->winner_points);
    widget_draw_text(offset_x, info_y + 1, buf);
}

/* Отрисовка экрана подключения */
void renderer_draw_connect(Renderer *renderer, const char *host, int port, char selected_char) {
    int box_width = 50;
    int box_height = 12;
    int start_x = (renderer->screen_width - box_width) / 2;
    int start_y = (renderer->screen_height - box_height) / 2;
    
    widget_draw_box(start_x, start_y, box_width, box_height);
    
    attron(A_BOLD);
    widget_draw_text_centered(start_x, start_y + 2, box_width, "ASCII Arena");
    attroff(A_BOLD);
    
    char buf[64];
    snprintf(buf, sizeof(buf), "Сервер: %s:%d", host, port);
    widget_draw_text_centered(start_x, start_y + 4, box_width, buf);
    
    snprintf(buf, sizeof(buf), "Ваш символ: %c", selected_char);
    attron(COLOR_PAIR(COLOR_PLAYER_WHITE) | A_BOLD);
    widget_draw_text_centered(start_x, start_y + 6, box_width, buf);
    attroff(COLOR_PAIR(COLOR_PLAYER_WHITE) | A_BOLD);
    
    widget_draw_text_centered(start_x, start_y + 8, box_width, "Нажмите A-Z для выбора символа");
    widget_draw_text_centered(start_x, start_y + 9, box_width, "Enter - подключиться, Q - выход");
}

/* Отрисовка экрана окончания игры */
void renderer_draw_game_over(Renderer *renderer, char winner, int winner_points) {
    int box_width = 40;
    int box_height = 8;
    int start_x = (renderer->screen_width - box_width) / 2;
    int start_y = (renderer->screen_height - box_height) / 2;
    
    widget_draw_box(start_x, start_y, box_width, box_height);
    
    attron(A_BOLD);
    widget_draw_text_centered(start_x, start_y + 2, box_width, "Игра окончена!");
    attroff(A_BOLD);
    
    char buf[64];
    snprintf(buf, sizeof(buf), "Победитель: %c (%d очков)", winner, winner_points);
    attron(COLOR_PAIR(COLOR_PLAYER_WHITE) | A_BOLD);
    widget_draw_text_centered(start_x, start_y + 4, box_width, buf);
    attroff(COLOR_PAIR(COLOR_PLAYER_WHITE) | A_BOLD);
    
    widget_draw_text_centered(start_x, start_y + 6, box_width, "Нажмите любую клавишу...");
}

/* Отрисовка сообщения */
void renderer_draw_message(Renderer *renderer, const char *message) {
    int len = (int)strlen(message);
    int box_width = len + 4;
    int box_height = 5;
    int start_x = (renderer->screen_width - box_width) / 2;
    int start_y = (renderer->screen_height - box_height) / 2;
    
    widget_draw_box(start_x, start_y, box_width, box_height);
    widget_draw_text_centered(start_x, start_y + 2, box_width, message);
}

/* Обновление экрана */
void renderer_refresh(void) {
    refresh();
}

/* Освобождение ресурсов рендерера */
void renderer_destroy(Renderer *renderer) {
    (void)renderer;
}
