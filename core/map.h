/*
 * map.h - Карта арены
 * Определяет структуру карты с террейном (стены, пол)
 */

#ifndef MAP_H
#define MAP_H

#include "vec2.h"

/* Тип террейна */
typedef enum {
    TERRAIN_FLOOR = 0,  /* Пол - можно ходить */
    TERRAIN_WALL = 1    /* Стена - нельзя ходить */
} Terrain;

/* Карта арены */
typedef struct {
    int size;           /* Размер карты (size x size) */
    Terrain *ground;    /* Массив террейна [size * size] */
} Map;

/* Создание карты заданного размера (стены по периметру, пол внутри) */
Map map_create(int size);

/* Получение террейна в позиции */
Terrain map_get_terrain(Map *map, Vec2 pos);

/* Установка террейна в позиции */
void map_set_terrain(Map *map, Vec2 pos, Terrain terrain);

/* Конвертация индекса массива в позицию */
Vec2 map_index_to_pos(Map *map, int index);

/* Конвертация позиции в индекс массива */
int map_pos_to_index(Map *map, Vec2 pos);

/* Проверка, находится ли позиция в пределах карты */
int map_is_valid_pos(Map *map, Vec2 pos);

/* Проверка, можно ли пройти в позицию */
int map_is_walkable(Map *map, Vec2 pos);

/* Освобождение памяти карты */
void map_destroy(Map *map);

#endif /* MAP_H */

