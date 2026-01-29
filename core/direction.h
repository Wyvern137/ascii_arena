/*
 * direction.h - Направления движения
 * Используется для движения игроков и заклинаний
 */

#ifndef DIRECTION_H
#define DIRECTION_H

#include "vec2.h"

/* Направление движения */
typedef enum {
    DIR_UP = 0,     /* Вверх */
    DIR_DOWN = 1,   /* Вниз */
    DIR_LEFT = 2,   /* Влево */
    DIR_RIGHT = 3,  /* Вправо */
    DIR_NONE = -1   /* Нет направления */
} Direction;

/* Конвертация направления в вектор смещения */
Vec2 direction_to_vec2(Direction dir);

/* Конвертация символа клавиши (WASD) в направление */
Direction direction_from_char(char c);

/* Получение противоположного направления */
Direction direction_opposite(Direction dir);

#endif /* DIRECTION_H */

