/*
 * character.h - Характеристики персонажа
 * Определяет базовые параметры игрового персонажа
 */

#ifndef CHARACTER_H
#define CHARACTER_H

/* Характеристики персонажа */
typedef struct {
    char symbol;        /* ASCII символ персонажа */
    int max_health;     /* Максимальное здоровье */
    int max_energy;     /* Максимальная энергия */
    float speed_base;   /* Базовая скорость движения */
} Character;

/* Создание персонажа с заданными параметрами */
Character character_create(char symbol, int max_health, int max_energy, float speed);

/* Создание персонажа с параметрами по умолчанию */
Character character_default(char symbol);

#endif /* CHARACTER_H */

