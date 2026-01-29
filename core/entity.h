/*
 * entity.h - Игровые сущности
 * Представляет игроков на арене с позицией, здоровьем и энергией
 */

#ifndef ENTITY_H
#define ENTITY_H

#include "vec2.h"
#include "direction.h"

/* Максимальное количество сущностей на арене */
#define MAX_ENTITIES 16

/* Типы заклинаний */
typedef enum {
    SPELL_TYPE_BASIC = 1,   /* Базовая атака: урон 5, без затрат маны */
    SPELL_TYPE_POWER = 2    /* Усиленная атака: урон 10, затрата маны 10, скорость x2 */
} SpellType;

/* Игровая сущность (игрок на арене) */
typedef struct {
    int id;                 /* Уникальный ID сущности */
    char symbol;            /* Символ персонажа */
    Vec2 position;          /* Позиция на карте */
    int health;             /* Текущее здоровье */
    int max_health;         /* Максимальное здоровье */
    int energy;             /* Текущая энергия */
    int max_energy;         /* Максимальная энергия */
    Direction direction;    /* Текущее направление */
    SpellType spell_type;   /* Выбранный тип заклинания */
    int alive;              /* Флаг жизни (1 = жив, 0 = мёртв) */
    float move_cooldown;    /* Кулдаун движения */
    float skill_cooldown;   /* Кулдаун способности */
    float damage_timer;     /* Таймер анимации урона (>0 = показывать красным) */
} Entity;

/* Создание сущности */
Entity entity_create(int id, char symbol, Vec2 pos, int max_health, int max_energy);

/* Перемещение сущности */
void entity_move(Entity *entity, Vec2 new_pos);

/* Получение урона */
void entity_take_damage(Entity *entity, int damage);

/* Восстановление здоровья */
void entity_heal(Entity *entity, int amount);

/* Использование энергии */
int entity_use_energy(Entity *entity, int amount);

/* Восстановление энергии */
void entity_restore_energy(Entity *entity, int amount);

/* Проверка, жива ли сущность */
int entity_is_alive(Entity *entity);

/* Обновление кулдаунов */
void entity_update_cooldowns(Entity *entity, float delta_time);

/* Проверка, может ли сущность двигаться */
int entity_can_move(Entity *entity);

/* Проверка, может ли сущность использовать способность */
int entity_can_cast(Entity *entity);

#endif /* ENTITY_H */

