/*
 * character.c - Реализация характеристик персонажа
 */

#include "character.h"

/* Создание персонажа с заданными параметрами */
Character character_create(char symbol, int max_health, int max_energy, float speed) {
    Character c;
    c.symbol = symbol;
    c.max_health = max_health;
    c.max_energy = max_energy;
    c.speed_base = speed;
    return c;
}

/* Создание персонажа с параметрами по умолчанию */
Character character_default(char symbol) {
    return character_create(symbol, 100, 100, 1.0f);
}

