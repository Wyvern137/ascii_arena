/*
 * widgets.c - Реализация базовых виджетов
 */

#include "widgets.h"
#include "terminal.h"
#include <ncurses.h>
#include <string.h>
#include <stdio.h>

/* Используем ACS символы вместо Unicode для совместимости */
/* Они выглядят похоже на закругленные углы */
#define ROUNDED_UL ACS_ULCORNER  /* ┌ */
#define ROUNDED_UR ACS_URCORNER  /* ┐ */
#define ROUNDED_LL ACS_LLCORNER  /* └ */
#define ROUNDED_LR ACS_LRCORNER  /* ┘ */
#define ROUNDED_H  ACS_HLINE     /* ─ */
#define ROUNDED_V  ACS_VLINE     /* │ */

/* Отрисовка рамки (обычная с ACS символами) */
void widget_draw_box(int x, int y, int width, int height) {
    /* Углы */
    mvaddch(y, x, ACS_ULCORNER);
    mvaddch(y, x + width - 1, ACS_URCORNER);
    mvaddch(y + height - 1, x, ACS_LLCORNER);
    mvaddch(y + height - 1, x + width - 1, ACS_LRCORNER);
    
    /* Горизонтальные линии */
    for (int i = 1; i < width - 1; i++) {
        mvaddch(y, x + i, ACS_HLINE);
        mvaddch(y + height - 1, x + i, ACS_HLINE);
    }
    
    /* Вертикальные линии */
    for (int i = 1; i < height - 1; i++) {
        mvaddch(y + i, x, ACS_VLINE);
        mvaddch(y + i, x + width - 1, ACS_VLINE);
    }
}

/* Отрисовка рамки с закругленными углами (используя Unicode) */
void widget_draw_rounded_box(int x, int y, int width, int height) {
    widget_draw_rounded_box_colored(x, y, width, height, 0);
}

/* Отрисовка рамки с закругленными углами и цветом */
void widget_draw_rounded_box_colored(int x, int y, int width, int height, int color_pair) {
    if (color_pair > 0) {
        attron(COLOR_PAIR(color_pair));
    }
    
    /* Углы (ACS символы) */
    mvaddch(y, x, ROUNDED_UL);
    mvaddch(y, x + width - 1, ROUNDED_UR);
    mvaddch(y + height - 1, x, ROUNDED_LL);
    mvaddch(y + height - 1, x + width - 1, ROUNDED_LR);
    
    /* Горизонтальные линии */
    for (int i = 1; i < width - 1; i++) {
        mvaddch(y, x + i, ROUNDED_H);
        mvaddch(y + height - 1, x + i, ROUNDED_H);
    }
    
    /* Вертикальные линии */
    for (int i = 1; i < height - 1; i++) {
        mvaddch(y + i, x, ROUNDED_V);
        mvaddch(y + i, x + width - 1, ROUNDED_V);
    }
    
    if (color_pair > 0) {
        attroff(COLOR_PAIR(color_pair));
    }
}

/* Отрисовка рамки с закругленными углами и заголовком */
void widget_draw_rounded_box_titled(int x, int y, int width, int height, const char *title, int color_pair) {
    if (color_pair > 0) {
        attron(COLOR_PAIR(color_pair));
    }
    
    /* Левый верхний угол */
    mvaddch(y, x, ROUNDED_UL);
    
    /* Заголовок с рамкой */
    if (title && strlen(title) > 0) {
        /* Немного горизонтальной линии перед заголовком */
        mvaddch(y, x + 1, ROUNDED_H);
        
        /* Заголовок */
        if (color_pair > 0) {
            attroff(COLOR_PAIR(color_pair));
        }
        attron(A_BOLD);
        mvprintw(y, x + 2, "%s", title);
        attroff(A_BOLD);
        if (color_pair > 0) {
            attron(COLOR_PAIR(color_pair));
        }
        
        /* Остаток горизонтальной линии после заголовка */
        int title_len = (int)strlen(title);
        for (int i = 2 + title_len; i < width - 1; i++) {
            mvaddch(y, x + i, ROUNDED_H);
        }
    } else {
        /* Просто горизонтальная линия */
        for (int i = 1; i < width - 1; i++) {
            mvaddch(y, x + i, ROUNDED_H);
        }
    }
    
    /* Правый верхний угол */
    mvaddch(y, x + width - 1, ROUNDED_UR);
    
    /* Нижняя линия */
    mvaddch(y + height - 1, x, ROUNDED_LL);
    for (int i = 1; i < width - 1; i++) {
        mvaddch(y + height - 1, x + i, ROUNDED_H);
    }
    mvaddch(y + height - 1, x + width - 1, ROUNDED_LR);
    
    /* Вертикальные линии */
    for (int i = 1; i < height - 1; i++) {
        mvaddch(y + i, x, ROUNDED_V);
        mvaddch(y + i, x + width - 1, ROUNDED_V);
    }
    
    if (color_pair > 0) {
        attroff(COLOR_PAIR(color_pair));
    }
}

/* Отрисовка текста */
void widget_draw_text(int x, int y, const char *text) {
    mvprintw(y, x, "%s", text);
}

/* Отрисовка текста с цветом */
void widget_draw_text_colored(int x, int y, const char *text, int color_pair, int bold) {
    if (color_pair > 0) {
        attron(COLOR_PAIR(color_pair));
    }
    if (bold) {
        attron(A_BOLD);
    }
    mvprintw(y, x, "%s", text);
    if (bold) {
        attroff(A_BOLD);
    }
    if (color_pair > 0) {
        attroff(COLOR_PAIR(color_pair));
    }
}

/* Отрисовка текста по центру */
void widget_draw_text_centered(int x, int y, int width, const char *text) {
    int len = (int)strlen(text);
    int offset = (width - len) / 2;
    if (offset < 0) offset = 0;
    mvprintw(y, x + offset, "%s", text);
}

/* Отрисовка текста по центру с цветом */
void widget_draw_text_centered_colored(int x, int y, int width, const char *text, int color_pair, int bold) {
    int len = (int)strlen(text);
    int offset = (width - len) / 2;
    if (offset < 0) offset = 0;
    
    if (color_pair > 0) {
        attron(COLOR_PAIR(color_pair));
    }
    if (bold) {
        attron(A_BOLD);
    }
    mvprintw(y, x + offset, "%s", text);
    if (bold) {
        attroff(A_BOLD);
    }
    if (color_pair > 0) {
        attroff(COLOR_PAIR(color_pair));
    }
}

/* Отрисовка текста по правому краю */
void widget_draw_text_right(int x, int y, int width, const char *text) {
    int len = (int)strlen(text);
    int offset = width - len;
    if (offset < 0) offset = 0;
    mvprintw(y, x + offset, "%s", text);
}

/* Отрисовка текста по правому краю с цветом */
void widget_draw_text_right_colored(int x, int y, int width, const char *text, int color_pair, int bold) {
    int len = (int)strlen(text);
    int offset = width - len;
    if (offset < 0) offset = 0;
    
    if (color_pair > 0) {
        attron(COLOR_PAIR(color_pair));
    }
    if (bold) {
        attron(A_BOLD);
    }
    mvprintw(y, x + offset, "%s", text);
    if (bold) {
        attroff(A_BOLD);
    }
    if (color_pair > 0) {
        attroff(COLOR_PAIR(color_pair));
    }
}

/* Отрисовка символа */
void widget_draw_char(int x, int y, char c) {
    mvaddch(y, x, c);
}

/* Отрисовка символа с цветом */
void widget_draw_char_colored(int x, int y, char c, int color_pair) {
    attron(COLOR_PAIR(color_pair));
    mvaddch(y, x, c);
    attroff(COLOR_PAIR(color_pair));
}

/* Отрисовка символа с цветом и BOLD */
void widget_draw_char_styled(int x, int y, char c, int color_pair, int bold) {
    if (color_pair > 0) {
        attron(COLOR_PAIR(color_pair));
    }
    if (bold) {
        attron(A_BOLD);
    }
    mvaddch(y, x, c);
    if (bold) {
        attroff(A_BOLD);
    }
    if (color_pair > 0) {
        attroff(COLOR_PAIR(color_pair));
    }
}

/* Отрисовка горизонтальной линии */
void widget_draw_hline(int x, int y, int length) {
    for (int i = 0; i < length; i++) {
        mvaddch(y, x + i, ACS_HLINE);
    }
}

/* Отрисовка вертикальной линии */
void widget_draw_vline(int x, int y, int length) {
    for (int i = 0; i < length; i++) {
        mvaddch(y + i, x, ACS_VLINE);
    }
}

/* Очистка области */
void widget_clear_area(int x, int y, int width, int height) {
    for (int j = 0; j < height; j++) {
        for (int i = 0; i < width; i++) {
            mvaddch(y + j, x + i, ' ');
        }
    }
}

/* Отрисовка прогресс-бара (старый формат) */
void widget_draw_progress_bar(int x, int y, int width, int current, int max, int color_pair) {
    if (max <= 0) max = 1;
    int filled = (current * width) / max;
    if (filled > width) filled = width;
    
    attron(COLOR_PAIR(color_pair));
    for (int i = 0; i < filled; i++) {
        mvaddch(y, x + i, ACS_CKBOARD);
    }
    attroff(COLOR_PAIR(color_pair));
    
    for (int i = filled; i < width; i++) {
        mvaddch(y, x + i, ' ');
    }
}

/* Отрисовка прогресс-бара в точном формате: [======---.] 65/100 
 * Формат: [==========] или [======-...] с числами справа
 * Длина бара: 10 символов
 * = для заполненных сегментов (BOLD)
 * - для частично заполненного сегмента (BOLD)
 * . для пустых сегментов
 */
void widget_draw_progress_bar_exact(int x, int y, int current, int max, int color_pair) {
    if (max <= 0) max = 1;
    if (current < 0) current = 0;
    if (current > max) current = max;
    
    const int bar_len = 10;
    
    /* Рассчитываем количество заполненных сегментов (каждый сегмент = max/bar_len) */
    int segment_value = max / bar_len;
    if (segment_value < 1) segment_value = 1;
    
    int full_segments = current / segment_value;
    int partial = current % segment_value;
    
    if (full_segments > bar_len) full_segments = bar_len;
    
    /* Скобка открытия - белая */
    attron(COLOR_PAIR(COLOR_PLAYER_WHITE));
    mvaddch(y, x, '[');
    attroff(COLOR_PAIR(COLOR_PLAYER_WHITE));
    
    /* Отрисовка бара */
    attron(COLOR_PAIR(color_pair));
    for (int i = 0; i < bar_len; i++) {
        char ch;
        int use_bold = 0;
        
        if (i < full_segments) {
            /* Полностью заполненный сегмент */
            ch = '=';
            use_bold = 1;
        } else if (i == full_segments && partial > 0) {
            /* Частично заполненный сегмент */
            ch = '-';
            use_bold = 1;
        } else {
            /* Пустой сегмент */
            ch = '.';
            use_bold = 0;
        }
        
        if (use_bold) {
            attron(A_BOLD);
        }
        mvaddch(y, x + 1 + i, ch);
        if (use_bold) {
            attroff(A_BOLD);
        }
    }
    attroff(COLOR_PAIR(color_pair));
    
    /* Скобка закрытия - белая */
    attron(COLOR_PAIR(COLOR_PLAYER_WHITE));
    mvaddch(y, x + 1 + bar_len, ']');
    attroff(COLOR_PAIR(COLOR_PLAYER_WHITE));
    
    /* Числа справа: " 65/100" */
    char num_buf[16];
    snprintf(num_buf, sizeof(num_buf), "%3d", current);
    
    /* Текущее значение - цветное BOLD */
    attron(COLOR_PAIR(color_pair) | A_BOLD);
    mvprintw(y, x + bar_len + 3, "%s", num_buf);
    attroff(COLOR_PAIR(color_pair) | A_BOLD);
    
    /* Разделитель - белый */
    attron(COLOR_PAIR(COLOR_PLAYER_WHITE));
    mvaddch(y, x + bar_len + 6, '/');
    attroff(COLOR_PAIR(COLOR_PLAYER_WHITE));
    
    /* Максимальное значение - цветное BOLD */
    snprintf(num_buf, sizeof(num_buf), "%d", max);
    attron(COLOR_PAIR(color_pair) | A_BOLD);
    mvprintw(y, x + bar_len + 7, "%s", num_buf);
    attroff(COLOR_PAIR(color_pair) | A_BOLD);
}

/* Отрисовка ASCII-арта (многострочный текст) */
void widget_draw_ascii_art(int x, int y, const char *art, int color_pair, int bold) {
    if (!art) return;
    
    if (color_pair > 0) {
        attron(COLOR_PAIR(color_pair));
    }
    if (bold) {
        attron(A_BOLD);
    }
    
    int line = 0;
    int col = 0;
    const char *p = art;
    
    while (*p) {
        if (*p == '\n') {
            line++;
            col = 0;
        } else {
            mvaddch(y + line, x + col, *p);
            col++;
        }
        p++;
    }
    
    if (bold) {
        attroff(A_BOLD);
    }
    if (color_pair > 0) {
        attroff(COLOR_PAIR(color_pair));
    }
}

/* Отрисовка ASCII-арта по центру экрана по горизонтали */
void widget_draw_ascii_art_centered(int y, int screen_width, const char *art, int color_pair, int bold) {
    if (!art) return;
    
    /* Находим ширину первой строки (или максимальную) */
    int max_width = 0;
    int current_width = 0;
    const char *p = art;
    
    while (*p) {
        if (*p == '\n') {
            if (current_width > max_width) {
                max_width = current_width;
            }
            current_width = 0;
        } else {
            current_width++;
        }
        p++;
    }
    if (current_width > max_width) {
        max_width = current_width;
    }
    
    int x = (screen_width - max_width) / 2;
    if (x < 0) x = 0;
    
    widget_draw_ascii_art(x, y, art, color_pair, bold);
}

/* Отрисовка ASCII-арта с чередующимися цветами по символам */
void widget_draw_ascii_art_dual_color(int x, int y, const char *art, int color1, int color2, int bold) {
    if (!art) return;
    
    int line = 0;
    int col = 0;
    int char_index = 0;  /* Счётчик непробельных символов для чередования */
    const char *p = art;
    
    while (*p) {
        if (*p == '\n') {
            line++;
            col = 0;
        } else {
            if (*p != ' ') {
                /* Чередуем цвета для непробельных символов */
                int current_color = (char_index % 2 == 0) ? color1 : color2;
                
                if (current_color > 0) {
                    attron(COLOR_PAIR(current_color));
                }
                if (bold) {
                    attron(A_BOLD);
                }
                
                mvaddch(y + line, x + col, *p);
                
                if (bold) {
                    attroff(A_BOLD);
                }
                if (current_color > 0) {
                    attroff(COLOR_PAIR(current_color));
                }
                
                char_index++;
            } else {
                /* Пробел - просто выводим без цвета */
                mvaddch(y + line, x + col, ' ');
            }
            col++;
        }
        p++;
    }
}
