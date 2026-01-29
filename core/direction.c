/*
 * direction.c - Реализация направлений движения
 */

#include "direction.h"

/* Конвертация направления в вектор смещения */
Vec2 direction_to_vec2(Direction dir) {
    switch (dir) {
        case DIR_UP:    return vec2_create(0, -1);
        case DIR_DOWN:  return vec2_create(0, 1);
        case DIR_LEFT:  return vec2_create(-1, 0);
        case DIR_RIGHT: return vec2_create(1, 0);
        default:        return vec2_create(0, 0);
    }
}

/* Конвертация символа клавиши (WASD) в направление */
Direction direction_from_char(char c) {
    switch (c) {
        case 'w': case 'W': return DIR_UP;
        case 's': case 'S': return DIR_DOWN;
        case 'a': case 'A': return DIR_LEFT;
        case 'd': case 'D': return DIR_RIGHT;
        default: return DIR_NONE;
    }
}

/* Получение противоположного направления */
Direction direction_opposite(Direction dir) {
    switch (dir) {
        case DIR_UP:    return DIR_DOWN;
        case DIR_DOWN:  return DIR_UP;
        case DIR_LEFT:  return DIR_RIGHT;
        case DIR_RIGHT: return DIR_LEFT;
        default:        return DIR_NONE;
    }
}

