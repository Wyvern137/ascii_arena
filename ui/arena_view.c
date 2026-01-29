/*
 * arena_view.c - Реализация отображения арены
 */

#include "arena_view.h"
#include "widgets.h"
#include "terminal.h"
#include <ncurses.h>
#include <string.h>
#include <stdio.h>
#include <sys/time.h>

/* Время анимации урона в миллисекундах */
#define DAMAGE_ANIMATION_TIME 66

/* Получение текущего времени в миллисекундах */
static int64_t get_time_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (int64_t)tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

/* Вычисление позиций луча заклинания (5 кадров назад от текущей позиции) */
static void compute_spell_ray(ArenaSpell *s, float interp_x, float interp_y, int ray_positions[SPELL_TRAIL_LENGTH][2]) {
    /* Вычисляем противоположное направление для луча (луч идёт назад от текущей позиции) */
    /* DIR_UP=0, DIR_DOWN=1, DIR_LEFT=2, DIR_RIGHT=3 */
    int dx = 0, dy = 0;
    switch (s->direction) {
        case 0: dy = 1; break;   /* DIR_UP -> луч идёт вниз (назад) */
        case 1: dy = -1; break;   /* DIR_DOWN -> луч идёт вверх (назад) */
        case 2: dx = 1; break;    /* DIR_LEFT -> луч идёт вправо (назад) */
        case 3: dx = -1; break;   /* DIR_RIGHT -> луч идёт влево (назад) */
    }
    
    /* Вычисляем 5 позиций луча от текущей позиции назад */
    for (int i = 0; i < SPELL_TRAIL_LENGTH; i++) {
        ray_positions[i][0] = (int)(interp_x + dx * i);
        ray_positions[i][1] = (int)(interp_y + dy * i);
    }
}

/* Отрисовка панели игрока */
static void render_player_panel(ArenaPlayer *player, ArenaEntity *entity, 
                                int x, int y, int is_current_user) {
    int is_alive = entity != NULL;
    
    /* Определяем стиль рамки */
    int border_color;
    int use_bold;
    
    if (is_current_user) {
        border_color = is_alive ? COLOR_PANEL_ACTIVE : COLOR_PANEL_DEAD;
        use_bold = 1;
    } else {
        border_color = is_alive ? COLOR_PANEL_INACTIVE : COLOR_PANEL_DEAD;
        use_bold = 0;
    }
    
    /* Рамка для символа игрока (5x3) */
    if (use_bold) attron(A_BOLD);
    attron(COLOR_PAIR(border_color));
    widget_draw_rounded_box(x, y, 5, 3);
    attroff(COLOR_PAIR(border_color));
    if (use_bold) attroff(A_BOLD);
    
    /* Символ игрока внутри рамки */
    int symbol_style = is_current_user ? 1 : 0;
    widget_draw_char_styled(x + 2, y + 1, player->symbol, COLOR_PLAYER_WHITE, symbol_style);
    
    /* Основная панель (22x4) справа от символа */
    int panel_x = x + 5;
    int panel_width = 22;
    int panel_height = 4;
    
    /* Заголовок панели с очками */
    /* title больше не используется, оставляем для совместимости */
    
    if (use_bold) attron(A_BOLD);
    attron(COLOR_PAIR(border_color));
    
    /* Верхняя граница с заголовком */
    mvaddch(y, panel_x, ACS_ULCORNER);
    mvaddch(y, panel_x + 1, ACS_HLINE);
    mvaddch(y, panel_x + 2, ACS_HLINE);
    attroff(COLOR_PAIR(border_color));
    if (use_bold) attroff(A_BOLD);
    
    /* Заголовок "Pts: N" */
    attron(COLOR_PAIR(COLOR_PLAYER_WHITE));
    mvprintw(y, panel_x + 3, " Pts: ");
    attron(A_BOLD);
    mvprintw(y, panel_x + 9, "%d", player->points);
    attroff(A_BOLD);
    mvprintw(y, panel_x + 9 + (player->points >= 10 ? 2 : 1), " ");
    attroff(COLOR_PAIR(COLOR_PLAYER_WHITE));
    
    /* Продолжение верхней границы */
    if (use_bold) attron(A_BOLD);
    attron(COLOR_PAIR(border_color));
    int title_end = panel_x + 10 + (player->points >= 10 ? 2 : 1);
    for (int i = title_end; i < panel_x + panel_width - 1; i++) {
        mvaddch(y, i, ACS_HLINE);
    }
    mvaddch(y, panel_x + panel_width - 1, ACS_URCORNER);
    
    /* Боковые границы */
    for (int i = 1; i < panel_height - 1; i++) {
        mvaddch(y + i, panel_x, ACS_VLINE);
        mvaddch(y + i, panel_x + panel_width - 1, ACS_VLINE);
    }
    
    /* Нижняя граница со стрелками */
    mvaddch(y + panel_height - 1, panel_x, ACS_LLCORNER);
    mvaddch(y + panel_height - 1, panel_x + 1, ACS_HLINE);
    attroff(COLOR_PAIR(border_color));
    if (use_bold) attroff(A_BOLD);
    
    /* Стрелка ">" */
    if (use_bold) attron(A_BOLD);
    attron(COLOR_PAIR(border_color));
    mvaddch(y + panel_height - 1, panel_x + 2, '>');
    attroff(COLOR_PAIR(border_color));
    if (use_bold) attroff(A_BOLD);
    
    /* Тип заклинания в середине нижней границы */
    int spell_type = entity ? entity->spell_type : 1;
    attron(COLOR_PAIR(COLOR_PLAYER_WHITE));
    mvprintw(y + panel_height - 1, panel_x + 5, " Spell: ");
    attron(A_BOLD);
    mvprintw(y + panel_height - 1, panel_x + 13, "%d", spell_type);
    attroff(A_BOLD);
    mvprintw(y + panel_height - 1, panel_x + 14, " ");
    attroff(COLOR_PAIR(COLOR_PLAYER_WHITE));
    
    /* Стрелка "<" */
    if (use_bold) attron(A_BOLD);
    attron(COLOR_PAIR(border_color));
    mvaddch(y + panel_height - 1, panel_x + panel_width - 3, '<');
    
    /* Завершение нижней границы */
    mvaddch(y + panel_height - 1, panel_x + panel_width - 2, ACS_HLINE);
    mvaddch(y + panel_height - 1, panel_x + panel_width - 1, ACS_LRCORNER);
    attroff(COLOR_PAIR(border_color));
    if (use_bold) attroff(A_BOLD);
    
    /* Прогресс-бары внутри панели */
    int bar_x = panel_x + 1;
    int bar_y = y + 1;
    
    int health = entity ? entity->health : 0;
    int max_health = entity ? entity->max_health : 100;
    int energy = entity ? entity->energy : 0;
    int max_energy = entity ? entity->max_energy : 100;
    
    /* Прогресс-бар здоровья (зелёный) */
    widget_draw_progress_bar_exact(bar_x, bar_y, health, max_health, COLOR_HEALTH);
    
    /* Прогресс-бар энергии (голубой) */
    widget_draw_progress_bar_exact(bar_x, bar_y + 1, energy, max_energy, COLOR_ENERGY);
}

/* Отрисовка карты */
static void render_map(ArenaView *view, int x, int y, int width, int height) {
    int64_t now = get_time_ms();
    
    /* Рамка карты */
    attron(COLOR_PAIR(COLOR_PLAYER_WHITE));
    widget_draw_rounded_box(x, y, width + 2, height + 2);
    attroff(COLOR_PAIR(COLOR_PLAYER_WHITE));
    
    /* Смещение для содержимого внутри рамки */
    int inner_x = x + 1;
    int inner_y = y + 1;
    
    /* Сначала рисуем карту (пол) - это очистит все артефакты */
    for (int map_y = 0; map_y < view->map_size; map_y++) {
        for (int map_x = 0; map_x < view->map_size; map_x++) {
            int screen_x = inner_x + map_x * 2;
            int screen_y = inner_y + map_y;
            
            /* Рисуем пол (точка и пробел) */
            attron(COLOR_PAIR(COLOR_FLOOR));
            mvaddch(screen_y, screen_x, '.');
            mvaddch(screen_y, screen_x + 1, ' ');
            attroff(COLOR_PAIR(COLOR_FLOOR));
        }
    }
    
    /* Индикатор направления текущего игрока (стрелка на основе реального направления сущности) */
    for (int i = 0; i < view->player_count; i++) {
        if (view->players[i].is_current_user && view->players[i].entity_id >= 0) {
            /* Находим сущность игрока */
            for (int j = 0; j < view->entity_count; j++) {
                if (view->entities[j].id == view->players[i].entity_id) {
                    ArenaEntity *e = &view->entities[j];
                    int dir_x = e->pos_x;
                    int dir_y = e->pos_y;
                    
                    /* Смещение по реальному направлению сущности (e->direction) */
                    /* DIR_UP=0, DIR_DOWN=1, DIR_LEFT=2, DIR_RIGHT=3 */
                    switch (e->direction) {
                        case 0: dir_y--; break;  /* Up (DIR_UP) */
                        case 1: dir_y++; break;  /* Down (DIR_DOWN) */
                        case 2: dir_x--; break;  /* Left (DIR_LEFT) */
                        case 3: dir_x++; break;  /* Right (DIR_RIGHT) */
                    }
                    
                    /* Проверяем границы */
                    if (dir_x >= 0 && dir_x < view->map_size && 
                        dir_y >= 0 && dir_y < view->map_size) {
                        int screen_x = inner_x + dir_x * 2;
                        int screen_y = inner_y + dir_y;
                        
                        /* Определяем символ стрелки в зависимости от направления */
                        /* DIR_UP=0, DIR_DOWN=1, DIR_LEFT=2, DIR_RIGHT=3 */
                        char arrow = '.';
                        switch (e->direction) {
                            case 0: arrow = '^'; break;  /* Up (DIR_UP) */
                            case 1: arrow = 'v'; break;  /* Down (DIR_DOWN) */
                            case 2: arrow = '<'; break;  /* Left (DIR_LEFT) */
                            case 3: arrow = '>'; break;  /* Right (DIR_RIGHT) */
                        }
                        
                        /* Рисуем стрелку направления (белая) */
                        attron(COLOR_PAIR(COLOR_PLAYER_WHITE) | A_BOLD);
                        mvaddch(screen_y, screen_x, arrow);
                        /* Очищаем вторую позицию для двойной ширины */
                        mvaddch(screen_y, screen_x + 1, ' ');
                        attroff(COLOR_PAIR(COLOR_PLAYER_WHITE) | A_BOLD);
                    }
                    break;
                }
            }
            break;
        }
    }
    
    /* Заклинания с разными типами и лучом (5 кадров) */
    for (int i = 0; i < view->spell_count; i++) {
        ArenaSpell *s = &view->spells[i];
        if (s->active) {
            /* Интерполяция позиции для головы заклинания */
            float t = s->interp_timer;
            float interp_x = (float)s->prev_pos_x + t * (float)(s->pos_x - s->prev_pos_x);
            float interp_y = (float)s->prev_pos_y + t * (float)(s->pos_y - s->prev_pos_y);
            
            /* Вычисляем позиции луча (5 кадров назад) */
            int ray_positions[SPELL_TRAIL_LENGTH][2];
            compute_spell_ray(s, interp_x, interp_y, ray_positions);
            
            /* Определяем цвет и символ в зависимости от типа */
            int spell_color = (s->spell_type == 2) ? COLOR_SPELL_POWER : COLOR_SPELL;
            char trail_char = (s->spell_type == 2) ? 'o' : '*';
            char head_char = (s->spell_type == 2) ? 'O' : '*';
            
            /* Рисуем луч (от дальних позиций к ближним, от еле видимых к ярким) */
            for (int j = SPELL_TRAIL_LENGTH - 1; j >= 0; j--) {
                /* Вычисляем яркость: j=0 (самая дальняя) -> еле видно, j=4 (самая ближняя) -> очень ярко */
                float brightness = (float)(j + 1) / (float)SPELL_TRAIL_LENGTH;
                
                int ray_x = inner_x + ray_positions[j][0] * 2;
                int ray_y = inner_y + ray_positions[j][1];
                
                /* Проверяем границы */
                if (ray_x >= inner_x && ray_x < inner_x + view->map_size * 2 &&
                    ray_y >= inner_y && ray_y < inner_y + view->map_size) {
                    
                    /* Определяем уровень яркости */
                    int attr = 0;
                    if (brightness < 0.3f) {
                        attr = A_DIM;  /* Еле видно */
                    } else if (brightness < 0.7f) {
                        attr = 0;  /* Нормальная яркость */
                    } else {
                        attr = A_BOLD;  /* Очень ярко */
                    }
                    
                    /* Для последней позиции (j=0, текущая позиция) используем головной символ */
                    char draw_char = (j == 0) ? head_char : trail_char;
                    
                    attron(COLOR_PAIR(spell_color) | attr);
                    mvaddch(ray_y, ray_x, draw_char);
                    /* Очищаем вторую позицию для двойной ширины */
                    mvaddch(ray_y, ray_x + 1, ' ');
                    attroff(COLOR_PAIR(spell_color) | attr);
                }
            }
        }
    }
    
    /* Сущности */
    for (int i = 0; i < view->entity_count; i++) {
        ArenaEntity *e = &view->entities[i];
        int screen_x = inner_x + e->pos_x * 2;
        int screen_y = inner_y + e->pos_y;
        
        /* Цвет: красный при уроне, иначе белый */
        int is_damaged = (e->damage_time > 0 && (now - e->damage_time) < DAMAGE_ANIMATION_TIME);
        int color = is_damaged ? COLOR_PLAYER_DAMAGED : COLOR_PLAYER_WHITE;
        
        /* Игроки с BOLD */
        if (e->is_player) {
            attron(COLOR_PAIR(color) | A_BOLD);
        } else {
            attron(COLOR_PAIR(color));
        }
        mvaddch(screen_y, screen_x, e->symbol);
        /* Очищаем вторую позицию для двойной ширины */
        mvaddch(screen_y, screen_x + 1, ' ');
        if (e->is_player) {
            attroff(COLOR_PAIR(color) | A_BOLD);
        } else {
            attroff(COLOR_PAIR(color));
        }
    }
    
    (void)width;
    (void)height;
}

/* Отрисовка сообщения о конце игры */
static void render_finish_message(ArenaView *view, int x, int y, int width, int height) {
    if (!view->game_finished) return;
    
    int center_y = y + height / 2;
    
    /* Player X wins! */
    const char *msg1_pre = "Player ";
    const char *msg1_post = " wins!";
    int msg1_len = (int)(strlen(msg1_pre) + 1 + strlen(msg1_post));
    
    /* Очищаем область */
    for (int i = -1; i <= 3; i++) {
        for (int j = 0; j < width + 2; j++) {
            mvaddch(center_y + i, x + j, ' ');
        }
    }
    
    /* Первая строка */
    int msg1_x = x + (width + 2 - msg1_len) / 2;
    mvprintw(center_y, msg1_x, "%s", msg1_pre);
    attron(A_BOLD);
    mvaddch(center_y, msg1_x + 7, view->winner_symbol);
    attroff(A_BOLD);
    mvprintw(center_y, msg1_x + 8, "%s", msg1_post);
    
    /* Пустая строка */
    center_y += 2;
    
    /* Press <Enter> to back to the menu */
    const char *msg2 = "Press <Enter> to back to the menu";
    int msg2_len = (int)strlen(msg2);
    int msg2_x = x + (width + 2 - msg2_len) / 2;
    
    mvprintw(center_y, msg2_x, "Press ");
    attron(COLOR_PAIR(COLOR_TEXT_CYAN) | A_BOLD);
    mvprintw(center_y, msg2_x + 6, "<Enter>");
    attroff(COLOR_PAIR(COLOR_TEXT_CYAN) | A_BOLD);
    mvprintw(center_y, msg2_x + 13, " to back to the menu");
}

/* Создание view арены */
ArenaView arena_view_create(int map_size, int winner_points) {
    ArenaView view;
    memset(&view, 0, sizeof(view));
    
    view.map_size = map_size;
    view.winner_points = winner_points;
    view.arena_number = 1;
    view.current_player_id = -1;
    view.current_direction = 0;
    view.local_direction = 2;  /* По умолчанию вниз (DIR_DOWN) */
    view.game_finished = 0;
    view.next_arena_countdown = -1;
    
    return view;
}

/* Обновление арены */
void arena_view_update(ArenaView *view) {
    int64_t now = get_time_ms();
    
    /* Очищаем устаревшие анимации урона */
    for (int i = 0; i < view->entity_count; i++) {
        if (view->entities[i].damage_time > 0 && 
            (now - view->entities[i].damage_time) >= DAMAGE_ANIMATION_TIME) {
            view->entities[i].damage_time = 0;
        }
    }
}

/* Обновление интерполяции заклинаний */
void arena_view_update_interpolation(ArenaView *view, float delta_time) {
    /* Скорость интерполяции - заклинание должно достичь цели за время между кадрами сервера */
    /* При 60 FPS сервера это ~16ms, интерполяция должна быть быстрой */
    float interp_speed = 15.0f;  /* Быстрая интерполяция */
    
    for (int i = 0; i < view->spell_count; i++) {
        ArenaSpell *s = &view->spells[i];
        if (s->active && s->interp_timer < 1.0f) {
            s->interp_timer += delta_time * interp_speed;
            if (s->interp_timer > 1.0f) {
                s->interp_timer = 1.0f;
            }
        }
    }
}

/* Отрисовка арены */
void arena_view_render(ArenaView *view, int screen_width, int screen_height) {
    /* Вычисляем размеры */
    int map_inner_width = view->map_size * 2;  /* Внутренняя ширина карты */
    int map_inner_height = view->map_size;
    int map_total_width = map_inner_width + 2;   /* +2 для рамки */
    int map_total_height = map_inner_height + 2;  /* +2 для рамки */
    
    int total_width = PLAYER_PANEL_WIDTH + 1 + map_total_width;
    int total_height = 1 + 1 + map_total_height + 2;  /* margin + info + map + notifications */
    
    int start_x = (screen_width - total_width) / 2;
    int start_y = (screen_height - total_height) / 2;
    
    if (start_x < 0) start_x = 0;
    if (start_y < 0) start_y = 0;
    
    int y = start_y;
    
    /* 1. Margin */
    y += 1;
    
    /* 2. Arena Info Label */
    char info_label[64];
    snprintf(info_label, sizeof(info_label), "Arena %d - Points to win: %d", 
             view->arena_number, view->winner_points);
    
    /* "Arena " */
    int info_x = start_x + (total_width - (int)strlen(info_label)) / 2;
    mvprintw(y, info_x, "Arena ");
    
    /* Номер арены (BOLD) */
    attron(A_BOLD);
    mvprintw(y, info_x + 6, "%d", view->arena_number);
    attroff(A_BOLD);
    
    /* " - Points to win: " */
    int num_len = (view->arena_number >= 10) ? 2 : 1;
    mvprintw(y, info_x + 6 + num_len, " - Points to win: ");
    
    /* Очки (BOLD) */
    attron(A_BOLD);
    mvprintw(y, info_x + 6 + num_len + 18, "%d", view->winner_points);
    attroff(A_BOLD);
    
    y += 1;
    
    /* 3. Player Panels (слева) и Map (справа) */
    int panels_x = start_x;
    int panels_y = y;
    
    /* Отрисовка панелей игроков */
    for (int i = 0; i < view->player_count; i++) {
        ArenaPlayer *player = &view->players[i];
        
        /* Находим сущность игрока */
        ArenaEntity *entity = NULL;
        if (player->entity_id >= 0) {
            for (int j = 0; j < view->entity_count; j++) {
                if (view->entities[j].id == player->entity_id) {
                    entity = &view->entities[j];
                    break;
                }
            }
        }
        
        render_player_panel(player, entity, panels_x, panels_y + i * PLAYER_PANEL_HEIGHT, 
                           player->is_current_user);
    }
    
    /* Отрисовка карты */
    int map_x = start_x + PLAYER_PANEL_WIDTH + 1;
    int map_y = y;
    
    render_map(view, map_x, map_y, map_inner_width, map_inner_height);
    
    /* Сообщение о конце игры (поверх карты) */
    if (view->game_finished) {
        render_finish_message(view, map_x, map_y, map_inner_width, map_inner_height);
    }
    
    y += map_total_height;
    
    /* DEBUG: Отображение количества заклинаний */
    mvprintw(0, 0, "Spells: %d", view->spell_count);
    if (view->spell_count > 0) {
        ArenaSpell *s = &view->spells[0];
        mvprintw(0, 15, "S0: id=%d pos=(%d,%d) dir=%d type=%d active=%d", 
                 s->id, s->pos_x, s->pos_y, s->direction, s->spell_type, s->active);
    }
    
    /* 4. Notifications (2 строки) */
    if (view->next_arena_countdown > 0 && !view->game_finished) {
        /* Сообщение о выжившем */
        int notif_y = y;
        
        if (view->arena_winner_symbol != '\0') {
            const char *part1 = "Player ";
            const char *part2 = " survived. ";
            
            int msg_len = (int)(strlen(part1) + 1 + strlen(part2));
            int msg_x = start_x + (total_width - msg_len) / 2;
            
            attron(COLOR_PAIR(COLOR_TEXT_LIGHTCYAN));
            mvprintw(notif_y, msg_x, "%s", part1);
            attroff(COLOR_PAIR(COLOR_TEXT_LIGHTCYAN));
            
            attron(COLOR_PAIR(COLOR_PLAYER_WHITE) | A_BOLD);
            mvaddch(notif_y, msg_x + 7, view->arena_winner_symbol);
            attroff(COLOR_PAIR(COLOR_PLAYER_WHITE) | A_BOLD);
            
            attron(COLOR_PAIR(COLOR_TEXT_LIGHTCYAN));
            mvprintw(notif_y, msg_x + 8, "%s", part2);
            attroff(COLOR_PAIR(COLOR_TEXT_LIGHTCYAN));
        } else {
            const char *msg = "No one survived";
            int msg_x = start_x + (total_width - (int)strlen(msg)) / 2;
            attron(COLOR_PAIR(COLOR_TEXT_LIGHTCYAN));
            mvprintw(notif_y, msg_x, "%s", msg);
            attroff(COLOR_PAIR(COLOR_TEXT_LIGHTCYAN));
        }
        
        notif_y++;
        
        /* Таймер */
        char timer_msg[64];
        snprintf(timer_msg, sizeof(timer_msg), "Starting new arena in %d...", view->next_arena_countdown);
        int timer_x = start_x + (total_width - (int)strlen(timer_msg)) / 2;
        
        attron(COLOR_PAIR(COLOR_TEXT_LIGHTCYAN));
        mvprintw(notif_y, timer_x, "Starting new arena in ");
        attron(A_BOLD);
        mvprintw(notif_y, timer_x + 22, "%d", view->next_arena_countdown);
        attroff(A_BOLD);
        mvprintw(notif_y, timer_x + 22 + (view->next_arena_countdown >= 10 ? 2 : 1), "...");
        attroff(COLOR_PAIR(COLOR_TEXT_LIGHTCYAN));
    }
}

/* Установка номера арены */
void arena_view_set_arena_number(ArenaView *view, int number) {
    view->arena_number = number;
}

/* Добавление/обновление игрока */
void arena_view_set_player(ArenaView *view, int id, char symbol, int points, int entity_id, int is_current) {
    /* Ищем существующего игрока */
    for (int i = 0; i < view->player_count; i++) {
        if (view->players[i].id == id) {
            view->players[i].symbol = symbol;
            view->players[i].points = points;
            view->players[i].entity_id = entity_id;
            view->players[i].is_current_user = is_current;
            return;
        }
    }
    
    /* Добавляем нового */
    if (view->player_count < MAX_ARENA_PLAYERS) {
        ArenaPlayer *p = &view->players[view->player_count++];
        p->id = id;
        p->symbol = symbol;
        p->points = points;
        p->entity_id = entity_id;
        p->is_current_user = is_current;
    }
}

/* Очистка списка игроков */
void arena_view_clear_players(ArenaView *view) {
    view->player_count = 0;
}

/* Добавление/обновление сущности */
void arena_view_set_entity(ArenaView *view, int id, char symbol, int pos_x, int pos_y, 
                           int health, int max_health, int energy, int max_energy, 
                           int direction, int spell_type, int is_player) {
    /* Ищем существующую сущность */
    for (int i = 0; i < view->entity_count; i++) {
        if (view->entities[i].id == id) {
            ArenaEntity *e = &view->entities[i];
            
            /* Проверяем, получил ли урон */
            if (health < e->health) {
                e->damage_time = get_time_ms();
            }
            
            e->symbol = symbol;
            e->pos_x = pos_x;
            e->pos_y = pos_y;
            e->health = health;
            e->max_health = max_health;
            e->energy = energy;
            e->max_energy = max_energy;
            e->direction = direction;
            e->spell_type = spell_type;
            e->is_player = is_player;
            return;
        }
    }
    
    /* Добавляем новую */
    if (view->entity_count < MAX_ARENA_ENTITIES) {
        ArenaEntity *e = &view->entities[view->entity_count++];
        e->id = id;
        e->symbol = symbol;
        e->pos_x = pos_x;
        e->pos_y = pos_y;
        e->health = health;
        e->max_health = max_health;
        e->energy = energy;
        e->max_energy = max_energy;
        e->direction = direction;
        e->spell_type = spell_type;
        e->is_player = is_player;
        e->damage_time = 0;
    }
}

/* Очистка списка сущностей */
void arena_view_clear_entities(ArenaView *view) {
    view->entity_count = 0;
}

/* Добавление заклинания */
void arena_view_set_spell(ArenaView *view, int id, int pos_x, int pos_y, int direction, int spell_type, int active) {
    /* Ищем существующее заклинание */
    for (int i = 0; i < view->spell_count; i++) {
        if (view->spells[i].id == id) {
            /* Сохраняем предыдущую позицию для интерполяции */
            if (view->spells[i].pos_x != pos_x || view->spells[i].pos_y != pos_y) {
                view->spells[i].prev_pos_x = view->spells[i].pos_x;
                view->spells[i].prev_pos_y = view->spells[i].pos_y;
                view->spells[i].interp_timer = 0.0f;
            }
            view->spells[i].pos_x = pos_x;
            view->spells[i].pos_y = pos_y;
            view->spells[i].direction = direction;
            view->spells[i].spell_type = spell_type;
            view->spells[i].active = active;
            return;
        }
    }
    
    /* Добавляем новое */
    if (view->spell_count < MAX_ARENA_SPELLS) {
        ArenaSpell *s = &view->spells[view->spell_count++];
        s->id = id;
        s->pos_x = pos_x;
        s->pos_y = pos_y;
        s->prev_pos_x = pos_x;
        s->prev_pos_y = pos_y;
        s->direction = direction;
        s->spell_type = spell_type;
        s->interp_timer = 1.0f;  /* Начинаем с полной позиции */
        s->active = active;
    }
}

/* Очистка списка заклинаний */
void arena_view_clear_spells(ArenaView *view) {
    view->spell_count = 0;
}

/* Установка текущего игрока */
void arena_view_set_current_player(ArenaView *view, int player_id, int direction) {
    view->current_player_id = player_id;
    view->current_direction = direction;
}

/* Установка локального направления (на основе нажатых клавиш) */
void arena_view_set_local_direction(ArenaView *view, int direction) {
    view->local_direction = direction;
}

/* Установка состояния окончания игры */
void arena_view_set_game_finished(ArenaView *view, char winner_symbol) {
    view->game_finished = 1;
    view->winner_symbol = winner_symbol;
}

/* Установка таймера следующей арены */
void arena_view_set_next_arena(ArenaView *view, int countdown, char winner_symbol) {
    view->next_arena_countdown = countdown;
    view->arena_winner_symbol = winner_symbol;
}

/* Нанесение урона сущности */
void arena_view_damage_entity(ArenaView *view, int entity_id) {
    for (int i = 0; i < view->entity_count; i++) {
        if (view->entities[i].id == entity_id) {
            view->entities[i].damage_time = get_time_ms();
            return;
        }
    }
}

/* Получение размеров арены для layout */
void arena_view_get_dimension(ArenaView *view, int *width, int *height) {
    int map_total_width = view->map_size * 2 + 2;  /* +2 для рамки */
    int map_total_height = view->map_size + 2;      /* +2 для рамки */
    
    *width = PLAYER_PANEL_WIDTH + 1 + map_total_width;
    *height = 1 + 1 + map_total_height + 2;
}

