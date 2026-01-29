/*
 * input.h - Обработка ввода с клавиатуры
 * Неблокирующий ввод через ncurses
 */

#ifndef INPUT_H
#define INPUT_H

#include "../core/direction.h"

/* Специальные коды клавиш */
#define KEY_QUIT 'q'
#define KEY_ESCAPE 27
#define KEY_SPACE ' '
#define KEY_ENTER_ALT '\n'
#define KEY_SPELL_1 '1'
#define KEY_SPELL_2 '2'

/* Получение нажатой клавиши (неблокирующий) */
int input_get_key(void);

/* Получение нажатой клавиши с таймаутом (в миллисекундах) */
int input_get_key_timeout(int timeout_ms);

/* Конвертация клавиши в направление */
Direction input_key_to_direction(int key);

/* Проверка, является ли клавиша клавишей выхода */
int input_is_quit_key(int key);

/* Проверка, является ли клавиша клавишей действия (пробел) */
int input_is_action_key(int key);

/* Проверка, является ли клавиша клавишей подтверждения (Enter) */
int input_is_confirm_key(int key);

/* Проверка, является ли клавиша клавишей выбора заклинания 1 */
int input_is_spell_1_key(int key);

/* Проверка, является ли клавиша клавишей выбора заклинания 2 */
int input_is_spell_2_key(int key);

#endif /* INPUT_H */

