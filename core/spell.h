/*
 * spell.h - Заклинания и проектилы
 * Представляет снаряды, летящие по арене
 */

#ifndef SPELL_H
#define SPELL_H

#include "vec2.h"
#include "direction.h"

/* Максимальное количество заклинаний на арене */
#define MAX_SPELLS 64
/* Максимальное количество затронутых сущностей одним заклинанием */
#define MAX_AFFECTED 16

/* Типы заклинаний (для Spell) */
#define SPELL_TYPE_BASIC_VAL 1   /* Базовая атака */
#define SPELL_TYPE_POWER_VAL 2   /* Усиленная атака */

/* Заклинание (проектил) */
typedef struct {
    int id;                         /* Уникальный ID заклинания */
    int caster_id;                  /* ID сущности, создавшей заклинание */
    Vec2 position;                  /* Текущая позиция */
    Direction direction;            /* Направление движения */
    int damage;                     /* Урон при попадании */
    float speed;                    /* Скорость движения */
    float move_timer;               /* Таймер для движения */
    int spell_type;                 /* Тип заклинания (1=базовая, 2=усиленная) */
    int affected_ids[MAX_AFFECTED]; /* ID затронутых сущностей */
    int affected_count;             /* Количество затронутых */
    int destroyed;                  /* Флаг уничтожения */
} Spell;

/* Создание заклинания */
Spell spell_create(int id, int caster_id, Vec2 pos, Direction dir, int damage, float speed, int spell_type);

/* Обновление заклинания (движение) */
void spell_update(Spell *spell, float delta_time);

/* Проверка, затронута ли сущность */
int spell_has_affected(Spell *spell, int entity_id);

/* Пометить сущность как затронутую */
void spell_mark_affected(Spell *spell, int entity_id);

/* Уничтожить заклинание */
void spell_destroy(Spell *spell);

/* Получить следующую позицию заклинания */
Vec2 spell_next_position(Spell *spell);

#endif /* SPELL_H */

