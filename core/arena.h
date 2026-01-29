/*
 * arena.h - Арена боя
 * Содержит карту, сущности и заклинания
 */

#ifndef ARENA_H
#define ARENA_H

#include "map.h"
#include "entity.h"
#include "spell.h"

/* Арена */
typedef struct {
    Map map;                        /* Карта арены */
    Entity entities[MAX_ENTITIES];  /* Массив сущностей */
    int entity_count;               /* Количество сущностей */
    Spell spells[MAX_SPELLS];       /* Массив заклинаний */
    int spell_count;                /* Количество заклинаний */
    int next_entity_id;             /* Следующий ID для сущности */
    int next_spell_id;              /* Следующий ID для заклинания */
} Arena;

/* Создание арены с картой заданного размера */
Arena arena_create(int map_size);

/* Добавление сущности на арену, возвращает ID сущности или -1 при ошибке */
int arena_add_entity(Arena *arena, char symbol, Vec2 pos, int max_health, int max_energy);

/* Добавление заклинания на арену, возвращает ID заклинания или -1 при ошибке */
int arena_add_spell(Arena *arena, int caster_id, Vec2 pos, Direction dir, int damage, float speed, int spell_type);

/* Обновление арены (движение, коллизии, урон) */
void arena_update(Arena *arena, float delta_time);

/* Получение сущности по ID */
Entity* arena_get_entity(Arena *arena, int id);

/* Получение сущности по позиции */
Entity* arena_get_entity_at(Arena *arena, Vec2 pos);

/* Перемещение сущности в направлении */
int arena_move_entity(Arena *arena, int entity_id, Direction dir);

/* Создание заклинания от сущности (spell_type: SPELL_TYPE_BASIC или SPELL_TYPE_POWER) */
int arena_cast_spell(Arena *arena, int entity_id, Direction dir, SpellType spell_type);

/* Проверка, занята ли позиция сущностью */
int arena_is_position_occupied(Arena *arena, Vec2 pos);

/* Подсчёт живых сущностей */
int arena_count_alive(Arena *arena);

/* Удаление уничтоженных заклинаний */
void arena_cleanup_spells(Arena *arena);

/* Освобождение памяти арены */
void arena_destroy(Arena *arena);

#endif /* ARENA_H */

