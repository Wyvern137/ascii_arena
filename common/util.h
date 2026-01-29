/*
 * util.h - Вспомогательные функции
 * Общие утилиты для клиента и сервера
 */

#ifndef UTIL_H
#define UTIL_H

#include <stddef.h>

/* Минимум из двух чисел */
int util_min(int a, int b);

/* Максимум из двух чисел */
int util_max(int a, int b);

/* Ограничение значения в диапазоне */
int util_clamp(int value, int min_val, int max_val);

/* Безопасное копирование строки */
void util_strncpy(char *dest, const char *src, size_t n);

/* Генерация случайного числа в диапазоне [min, max] */
int util_random(int min_val, int max_val);

/* Инициализация генератора случайных чисел */
void util_random_init(void);

#endif /* UTIL_H */

