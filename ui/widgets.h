/*
 * widgets.h - Базовые виджеты интерфейса
 * Примитивы для отрисовки элементов UI
 */

#ifndef WIDGETS_H
#define WIDGETS_H

/* Отрисовка рамки (обычная с ACS символами) */
void widget_draw_box(int x, int y, int width, int height);

/* Отрисовка рамки с закругленными углами (используя Unicode) */
void widget_draw_rounded_box(int x, int y, int width, int height);

/* Отрисовка рамки с закругленными углами и цветом */
void widget_draw_rounded_box_colored(int x, int y, int width, int height, int color_pair);

/* Отрисовка рамки с закругленными углами и заголовком */
void widget_draw_rounded_box_titled(int x, int y, int width, int height, const char *title, int color_pair);

/* Отрисовка текста */
void widget_draw_text(int x, int y, const char *text);

/* Отрисовка текста с цветом */
void widget_draw_text_colored(int x, int y, const char *text, int color_pair, int bold);

/* Отрисовка текста по центру в заданной области */
void widget_draw_text_centered(int x, int y, int width, const char *text);

/* Отрисовка текста по центру с цветом */
void widget_draw_text_centered_colored(int x, int y, int width, const char *text, int color_pair, int bold);

/* Отрисовка текста по правому краю */
void widget_draw_text_right(int x, int y, int width, const char *text);

/* Отрисовка текста по правому краю с цветом */
void widget_draw_text_right_colored(int x, int y, int width, const char *text, int color_pair, int bold);

/* Отрисовка символа */
void widget_draw_char(int x, int y, char c);

/* Отрисовка символа с цветом */
void widget_draw_char_colored(int x, int y, char c, int color_pair);

/* Отрисовка символа с цветом и BOLD */
void widget_draw_char_styled(int x, int y, char c, int color_pair, int bold);

/* Отрисовка горизонтальной линии */
void widget_draw_hline(int x, int y, int length);

/* Отрисовка вертикальной линии */
void widget_draw_vline(int x, int y, int length);

/* Очистка области */
void widget_clear_area(int x, int y, int width, int height);

/* Отрисовка прогресс-бара (старый формат) */
void widget_draw_progress_bar(int x, int y, int width, int current, int max, int color_pair);

/* Отрисовка прогресс-бара в точном формате: [======---.] 65/100 */
void widget_draw_progress_bar_exact(int x, int y, int current, int max, int color_pair);

/* Отрисовка ASCII-арта (многострочный текст) */
void widget_draw_ascii_art(int x, int y, const char *art, int color_pair, int bold);

/* Отрисовка ASCII-арта по центру экрана по горизонтали */
void widget_draw_ascii_art_centered(int y, int screen_width, const char *art, int color_pair, int bold);

/* Отрисовка ASCII-арта с чередующимися цветами по символам */
void widget_draw_ascii_art_dual_color(int x, int y, const char *art, int color1, int color2, int bold);

#endif /* WIDGETS_H */
