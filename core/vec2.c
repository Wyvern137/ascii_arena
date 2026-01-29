/*
 * vec2.c - Реализация двумерного вектора
 */

#include "vec2.h"

/* Создание вектора с заданными координатами */
Vec2 vec2_create(int x, int y) {
    Vec2 v = {x, y};
    return v;
}

/* Сложение двух векторов */
Vec2 vec2_add(Vec2 a, Vec2 b) {
    return vec2_create(a.x + b.x, a.y + b.y);
}

/* Вычитание векторов */
Vec2 vec2_sub(Vec2 a, Vec2 b) {
    return vec2_create(a.x - b.x, a.y - b.y);
}

/* Квадрат длины вектора */
int vec2_length_sq(Vec2 v) {
    return v.x * v.x + v.y * v.y;
}

/* Проверка равенства двух векторов */
int vec2_equals(Vec2 a, Vec2 b) {
    return a.x == b.x && a.y == b.y;
}

