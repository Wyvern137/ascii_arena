/*
 * input.c - Реализация обработки ввода
 */

#include "input.h"
#include <ncurses.h>

/* Получение нажатой клавиши (неблокирующий) */
int input_get_key(void) {
    return getch();
}

/* Получение нажатой клавиши с таймаутом */
int input_get_key_timeout(int timeout_ms) {
    timeout(timeout_ms);
    int key = getch();
    timeout(-1);  /* Возвращаем блокирующий режим */
    return key;
}

/* Конвертация клавиши в направление */
Direction input_key_to_direction(int key) {
    switch (key) {
        case 'w': case 'W': case KEY_UP:
            return DIR_UP;
        case 's': case 'S': case KEY_DOWN:
            return DIR_DOWN;
        case 'a': case 'A': case KEY_LEFT:
            return DIR_LEFT;
        case 'd': case 'D': case KEY_RIGHT:
            return DIR_RIGHT;
        default:
            return DIR_NONE;
    }
}

/* Проверка, является ли клавиша клавишей выхода */
int input_is_quit_key(int key) {
    return key == KEY_QUIT || key == KEY_ESCAPE;
}

/* Проверка, является ли клавиша клавишей действия (пробел) */
int input_is_action_key(int key) {
    return key == KEY_SPACE;
}

/* Проверка, является ли клавиша клавишей подтверждения (Enter) */
int input_is_confirm_key(int key) {
    return key == KEY_ENTER || key == KEY_ENTER_ALT;
}

/* Проверка, является ли клавиша клавишей выбора заклинания 1 */
int input_is_spell_1_key(int key) {
    return key == KEY_SPELL_1;
}

/* Проверка, является ли клавиша клавишей выбора заклинания 2 */
int input_is_spell_2_key(int key) {
    return key == KEY_SPELL_2;
}

