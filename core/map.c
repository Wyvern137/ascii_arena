/*
 * map.c - Реализация карты арены
 */

#include "map.h"
#include <stdlib.h>

/* Создание карты заданного размера (стены по периметру, пол внутри) */
Map map_create(int size) {
    Map map;
    map.size = size;
    map.ground = (Terrain *)malloc(size * size * sizeof(Terrain));
    
    /* Заполняем карту: стены по периметру, пол внутри */
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            int index = y * size + x;
            if (x == 0 || x == size - 1 || y == 0 || y == size - 1) {
                map.ground[index] = TERRAIN_WALL;
            } else {
                map.ground[index] = TERRAIN_FLOOR;
            }
        }
    }
    
    return map;
}

/* Получение террейна в позиции */
Terrain map_get_terrain(Map *map, Vec2 pos) {
    if (!map_is_valid_pos(map, pos)) {
        return TERRAIN_WALL;  /* За пределами карты - стена */
    }
    return map->ground[pos.y * map->size + pos.x];
}

/* Установка террейна в позиции */
void map_set_terrain(Map *map, Vec2 pos, Terrain terrain) {
    if (map_is_valid_pos(map, pos)) {
        map->ground[pos.y * map->size + pos.x] = terrain;
    }
}

/* Конвертация индекса массива в позицию */
Vec2 map_index_to_pos(Map *map, int index) {
    return vec2_create(index % map->size, index / map->size);
}

/* Конвертация позиции в индекс массива */
int map_pos_to_index(Map *map, Vec2 pos) {
    return pos.y * map->size + pos.x;
}

/* Проверка, находится ли позиция в пределах карты */
int map_is_valid_pos(Map *map, Vec2 pos) {
    return pos.x >= 0 && pos.x < map->size && 
           pos.y >= 0 && pos.y < map->size;
}

/* Проверка, можно ли пройти в позицию */
int map_is_walkable(Map *map, Vec2 pos) {
    return map_get_terrain(map, pos) == TERRAIN_FLOOR;
}

/* Освобождение памяти карты */
void map_destroy(Map *map) {
    if (map->ground) {
        free(map->ground);
        map->ground = NULL;
    }
    map->size = 0;
}

