/*
 * terminal.c - Реализация инициализации терминала
 */

#include "terminal.h"
#include <ncurses.h>
#include <locale.h>
#include <stdlib.h>
#include <string.h>

/* Инициализация терминала (ncurses) */
int terminal_init(void) {
    /* Устанавливаем локаль для Unicode */
    setlocale(LC_ALL, "");
    
    /* Исправление для Linux: устанавливаем TERM, если он не установлен или равен "dumb" */
    const char *term = getenv("TERM");
    if (term == NULL || strcmp(term, "dumb") == 0) {
        /* Пробуем установить TERM для поддержки цветов */
        const char *fallback_term = "xterm-256color";
        setenv("TERM", fallback_term, 1);
    }
    
    /* Инициализация ncurses */
    initscr();
    
    /* Настройка режимов */
    cbreak();               /* Отключаем буферизацию ввода */
    noecho();               /* Не отображаем вводимые символы */
    keypad(stdscr, TRUE);   /* Включаем обработку специальных клавиш */
    curs_set(0);            /* Скрываем курсор */
    nodelay(stdscr, TRUE);  /* Неблокирующий ввод */
    leaveok(stdscr, TRUE);  /* Не обновлять позицию курсора - уменьшает мерцание */
    
    /* Инициализация цветов */
    if (has_colors()) {
        start_color();
        use_default_colors();
        
        /* Все игроки белые (Color::White) */
        init_pair(COLOR_PLAYER_WHITE, COLOR_WHITE, -1);
        
        /* Красный при уроне (Color::LightRed) */
        init_pair(COLOR_PLAYER_DAMAGED, COLOR_RED, -1);
        
        /* Стены/рамки - белые (Color::White) */
        init_pair(COLOR_WALL, COLOR_WHITE, -1);
        
        /* Пол - тёмно-серый */
        init_pair(COLOR_FLOOR, COLOR_BLACK, -1);
        
        /* Заклинания - оранжевый */
        init_pair(COLOR_SPELL, COLOR_YELLOW, -1);
        
        /* Усиленные заклинания - красный */
        init_pair(COLOR_SPELL_POWER, COLOR_RED, -1);
        
        /* Здоровье - зелёный (Color::Green) */
        init_pair(COLOR_HEALTH, COLOR_GREEN, -1);
        
        /* Энергия - голубой (Color::Cyan) */
        init_pair(COLOR_ENERGY, COLOR_CYAN, -1);
        
        /* Активная панель - белый (Color::White с BOLD) */
        init_pair(COLOR_PANEL_ACTIVE, COLOR_WHITE, -1);
        
        /* Неактивная панель - серый (Color::Gray) */
        init_pair(COLOR_PANEL_INACTIVE, COLOR_WHITE, -1);  /* Будет использоваться без BOLD */
        
        /* Мёртвые игроки - тёмно-серый (Color::DarkGray) */
        init_pair(COLOR_PANEL_DEAD, COLOR_BLACK, -1);
        
        /* Статус: Connected, Logged, Compatible, Checked - светло-зелёный (Color::LightGreen) */
        init_pair(COLOR_STATUS_CONNECTED, COLOR_GREEN, -1);  /* С BOLD будет ярче */
        
        /* Статус: Waiting, Checking - светло-жёлтый (Color::LightYellow) */
        init_pair(COLOR_STATUS_WARNING, COLOR_YELLOW, -1);
        
        /* Статус: ошибки - светло-красный (Color::LightRed) */
        init_pair(COLOR_STATUS_ERROR, COLOR_RED, -1);
        
        /* Статус: Not connected, Not logged - тёмно-серый (Color::DarkGray) */
        init_pair(COLOR_STATUS_INACTIVE, COLOR_BLACK, -1);
        
        /* Серый для версии и неактивных элементов (Color::Gray) */
        init_pair(COLOR_STATUS_GRAY, COLOR_WHITE, -1);  /* Без BOLD */
        
        /* Белый текст */
        init_pair(COLOR_TEXT_WHITE, COLOR_WHITE, -1);
        
        /* Cyan для <Enter> (Color::Cyan) */
        init_pair(COLOR_TEXT_CYAN, COLOR_CYAN, -1);
        
        /* Yellow для <Esc> (Color::Yellow) */
        init_pair(COLOR_TEXT_YELLOW, COLOR_YELLOW, -1);
        
        /* LightCyan для сообщений о таймере */
        init_pair(COLOR_TEXT_LIGHTCYAN, COLOR_CYAN, -1);
        
        /* Заголовок AsciiArena - используем init_color для точных RGB значений */
        /* #92A8B3 = RGB(146, 168, 179) -> ncurses RGB (0-1000): 573, 659, 702 */
        /* #7F977B = RGB(127, 151, 123) -> ncurses RGB (0-1000): 498, 592, 482 */
        if (can_change_color()) {
            /* Создаём кастомные цвета */
            init_color(20, 573, 659, 702);  /* #92A8B3 светло-голубой */
            init_color(21, 498, 592, 482);  /* #7F977B светло-зелёный */
            init_pair(COLOR_TITLE_BLUE, 20, -1);
            init_pair(COLOR_TITLE_GREEN, 21, -1);
        } else {
            /* Fallback: используем яркие cyan и green */
            init_pair(COLOR_TITLE_BLUE, COLOR_CYAN, -1);
            init_pair(COLOR_TITLE_GREEN, COLOR_GREEN, -1);
        }
    }
    
    return 0;
}

/* Завершение работы с терминалом */
void terminal_cleanup(void) {
    endwin();
}

/* Получение размера терминала */
void terminal_get_size(int *width, int *height) {
    getmaxyx(stdscr, *height, *width);
}

/* Установка цвета */
void terminal_set_color(int color_pair) {
    attron(COLOR_PAIR(color_pair));
}

/* Сброс цвета */
void terminal_reset_color(void) {
    attroff(A_COLOR);
}

/* Получение цвета для игрока (белый или красный при уроне) */
int terminal_get_player_color(int is_damaged) {
    return is_damaged ? COLOR_PLAYER_DAMAGED : COLOR_PLAYER_WHITE;
}

/* Установка видимости курсора */
void terminal_set_cursor(int visible) {
    curs_set(visible ? 1 : 0);
}
