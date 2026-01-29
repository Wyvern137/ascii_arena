/*
 * util.c - Реализация вспомогательных функций
 */

#include "util.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Минимум из двух чисел */
int util_min(int a, int b) {
    return (a < b) ? a : b;
}

/* Максимум из двух чисел */
int util_max(int a, int b) {
    return (a > b) ? a : b;
}

/* Ограничение значения в диапазоне */
int util_clamp(int value, int min_val, int max_val) {
    if (value < min_val) return min_val;
    if (value > max_val) return max_val;
    return value;
}

/* Безопасное копирование строки */
void util_strncpy(char *dest, const char *src, size_t n) {
    if (n == 0) return;
    strncpy(dest, src, n - 1);
    dest[n - 1] = '\0';
}

/* Генерация случайного числа в диапазоне [min, max] */
int util_random(int min_val, int max_val) {
    if (min_val >= max_val) return min_val;
    return min_val + rand() % (max_val - min_val + 1);
}

/* Инициализация генератора случайных чисел */
void util_random_init(void) {
    srand((unsigned int)time(NULL));
}

