/*
 * terminal.h - Инициализация терминала
 * Обёртка над ncurses для работы с терминалом
 */

#ifndef TERMINAL_H
#define TERMINAL_H

/* Цвета */

/* Основные цвета */
#define COLOR_PLAYER_WHITE 1      /* Белый для всех игроков */
#define COLOR_PLAYER_DAMAGED 2    /* Красный при уроне (LightRed) */
#define COLOR_WALL 3              /* Белый для рамок */
#define COLOR_FLOOR 4             /* Тёмно-серый для пола */
#define COLOR_SPELL 5             /* Оранжевый для заклинаний */
#define COLOR_SPELL_POWER 22      /* Красный для усиленных заклинаний */

/* Прогресс-бары */
#define COLOR_HEALTH 6            /* Зелёный для здоровья */
#define COLOR_ENERGY 7            /* Голубой для энергии */

/* Панели */
#define COLOR_PANEL_ACTIVE 8      /* Белый для активной панели */
#define COLOR_PANEL_INACTIVE 9    /* Серый для неактивной панели */
#define COLOR_PANEL_DEAD 10       /* Тёмно-серый для мёртвых игроков */

/* Статусы */
#define COLOR_STATUS_CONNECTED 11    /* LightGreen для "Connected", "Logged", "Compatible", "Checked" */
#define COLOR_STATUS_WARNING 12      /* LightYellow для "Waiting other players...", "Checking..." */
#define COLOR_STATUS_ERROR 13        /* LightRed для ошибок */
#define COLOR_STATUS_INACTIVE 14     /* DarkGray для "Not connected", "Not logged" */
#define COLOR_STATUS_GRAY 15         /* Gray для версии, неактивных элементов */

/* Текст */
#define COLOR_TEXT_WHITE 16          /* Белый текст */
#define COLOR_TEXT_CYAN 17           /* Cyan для <Enter> */
#define COLOR_TEXT_YELLOW 18         /* Yellow для <Esc> */
#define COLOR_TEXT_LIGHTCYAN 19      /* LightCyan для сообщений о таймере */

/* Заголовок AsciiArena - чередующиеся цвета */
#define COLOR_TITLE_BLUE 20          /* #92A8B3 - светло-голубой */
#define COLOR_TITLE_GREEN 21         /* #7F977B - светло-зелёный */

/* Инициализация терминала (ncurses) */
int terminal_init(void);

/* Завершение работы с терминалом */
void terminal_cleanup(void);

/* Получение размера терминала */
void terminal_get_size(int *width, int *height);

/* Установка цвета */
void terminal_set_color(int color_pair);

/* Сброс цвета */
void terminal_reset_color(void);

/* Получение цвета для игрока (белый или красный при уроне) */
int terminal_get_player_color(int is_damaged);

/* Установка видимости курсора */
void terminal_set_cursor(int visible);

#endif /* TERMINAL_H */
