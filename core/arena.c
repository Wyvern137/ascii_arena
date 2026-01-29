/*
 * arena.c - Реализация арены боя
 */

#include "arena.h"
#include <stdlib.h>
#include <stdio.h>

/* Константы заклинаний */
#define SPELL_BASIC_DAMAGE 5       /* Урон базовой атаки */
#define SPELL_BASIC_SPEED 5.0f     /* Скорость базовой атаки (уменьшена для видимости) */
#define SPELL_BASIC_ENERGY 0       /* Затрата маны базовой атаки */

#define SPELL_POWER_DAMAGE 10      /* Урон усиленной атаки */
#define SPELL_POWER_SPEED 10.0f    /* Скорость усиленной атаки (уменьшена для видимости) */
#define SPELL_POWER_ENERGY 10      /* Затрата маны усиленной атаки */

/* Создание арены с картой заданного размера */
Arena arena_create(int map_size) {
    Arena arena;
    arena.map = map_create(map_size);
    arena.entity_count = 0;
    arena.spell_count = 0;
    arena.next_entity_id = 1;
    arena.next_spell_id = 1;
    return arena;
}

/* Добавление сущности на арену */
int arena_add_entity(Arena *arena, char symbol, Vec2 pos, int max_health, int max_energy) {
    if (arena->entity_count >= MAX_ENTITIES) {
        return -1;
    }
    
    int id = arena->next_entity_id++;
    arena->entities[arena->entity_count] = entity_create(id, symbol, pos, max_health, max_energy);
    arena->entity_count++;
    return id;
}

/* Добавление заклинания на арену */
int arena_add_spell(Arena *arena, int caster_id, Vec2 pos, Direction dir, int damage, float speed, int spell_type) {
    if (arena->spell_count >= MAX_SPELLS) {
        return -1;
    }
    
    int id = arena->next_spell_id++;
    arena->spells[arena->spell_count] = spell_create(id, caster_id, pos, dir, damage, speed, spell_type);
    arena->spell_count++;
    printf("DEBUG: Spell created id=%d pos=(%d,%d) dir=%d type=%d speed=%.1f\n", 
           id, (int)pos.x, (int)pos.y, dir, spell_type, speed);
    return id;
}

/* Время между перемещениями заклинания */
#define SPELL_MOVE_INTERVAL 0.066f

/* Проверка коллизии заклинания с сущностями на текущей позиции */
static int check_spell_collision(Arena *arena, Spell *spell) {
    for (int j = 0; j < arena->entity_count; j++) {
        Entity *entity = &arena->entities[j];
        if (!entity->alive) continue;
        if (entity->id == spell->caster_id) continue;  /* Не бьём себя */
        if (spell_has_affected(spell, entity->id)) continue;
        
        if (vec2_equals(spell->position, entity->position)) {
            entity_take_damage(entity, spell->damage);
            spell_mark_affected(spell, entity->id);
            spell_destroy(spell);
            return 1;  /* Коллизия произошла */
        }
    }
    return 0;  /* Нет коллизии */
}

/* Обновление арены */
void arena_update(Arena *arena, float delta_time) {
    /* Обновляем кулдауны сущностей */
    for (int i = 0; i < arena->entity_count; i++) {
        entity_update_cooldowns(&arena->entities[i], delta_time);
    }
    
    /* Обновляем заклинания с проверкой коллизий на каждом шаге */
    for (int i = 0; i < arena->spell_count; i++) {
        Spell *spell = &arena->spells[i];
        if (spell->destroyed) continue;
        
        /* Обновляем таймер движения */
        spell->move_timer += delta_time * spell->speed;
        
        /* Перемещаем заклинание пошагово, проверяя коллизии на каждом шаге */
        while (spell->move_timer >= SPELL_MOVE_INTERVAL && !spell->destroyed) {
            spell->move_timer -= SPELL_MOVE_INTERVAL;
            
            /* Вычисляем новую позицию */
            Vec2 delta = direction_to_vec2(spell->direction);
            spell->position = vec2_add(spell->position, delta);
            
            /* Проверяем коллизию со стеной */
            if (!map_is_walkable(&arena->map, spell->position)) {
                spell_destroy(spell);
                break;
            }
            
            /* Проверяем коллизию с сущностями */
            if (check_spell_collision(arena, spell)) {
                break;  /* Заклинание уничтожено при коллизии */
            }
        }
    }
    
    /* Удаляем уничтоженные заклинания */
    arena_cleanup_spells(arena);
}

/* Получение сущности по ID */
Entity* arena_get_entity(Arena *arena, int id) {
    for (int i = 0; i < arena->entity_count; i++) {
        if (arena->entities[i].id == id) {
            return &arena->entities[i];
        }
    }
    return NULL;
}

/* Получение сущности по позиции */
Entity* arena_get_entity_at(Arena *arena, Vec2 pos) {
    for (int i = 0; i < arena->entity_count; i++) {
        if (arena->entities[i].alive && vec2_equals(arena->entities[i].position, pos)) {
            return &arena->entities[i];
        }
    }
    return NULL;
}

/* Перемещение сущности в направлении */
int arena_move_entity(Arena *arena, int entity_id, Direction dir) {
    Entity *entity = arena_get_entity(arena, entity_id);
    if (!entity || !entity_can_move(entity)) {
        return 0;
    }
    
    Vec2 delta = direction_to_vec2(dir);
    Vec2 new_pos = vec2_add(entity->position, delta);
    
    /* Проверяем, можно ли пройти */
    if (!map_is_walkable(&arena->map, new_pos)) {
        return 0;
    }
    
    /* Проверяем, не занята ли позиция другой сущностью */
    if (arena_is_position_occupied(arena, new_pos)) {
        return 0;
    }
    
    entity->direction = dir;
    entity_move(entity, new_pos);
    return 1;
}

/* Создание заклинания от сущности */
int arena_cast_spell(Arena *arena, int entity_id, Direction dir, SpellType spell_type) {
    Entity *entity = arena_get_entity(arena, entity_id);
    if (!entity || !entity->alive || entity->skill_cooldown > 0) {
        return -1;
    }
    
    /* Определяем параметры заклинания в зависимости от типа */
    int damage;
    float speed;
    int energy_cost;
    
    if (spell_type == SPELL_TYPE_POWER) {
        damage = SPELL_POWER_DAMAGE;
        speed = SPELL_POWER_SPEED;
        energy_cost = SPELL_POWER_ENERGY;
    } else {
        /* По умолчанию базовая атака */
        damage = SPELL_BASIC_DAMAGE;
        speed = SPELL_BASIC_SPEED;
        energy_cost = SPELL_BASIC_ENERGY;
    }
    
    /* Проверяем и тратим энергию */
    if (entity->energy < energy_cost) {
        return -1;
    }
    
    if (energy_cost > 0) {
        entity->energy -= energy_cost;
    }
    entity->skill_cooldown = 0.5f;  /* Кулдаун способности */
    
    /* Создаём заклинание перед сущностью */
    Vec2 spell_pos = vec2_add(entity->position, direction_to_vec2(dir));
    entity->direction = dir;
    
    /* Передаём тип заклинания (1 = базовая, 2 = усиленная) */
    int spell_type_val = (spell_type == SPELL_TYPE_POWER) ? SPELL_TYPE_POWER_VAL : SPELL_TYPE_BASIC_VAL;
    return arena_add_spell(arena, entity_id, spell_pos, dir, damage, speed, spell_type_val);
}

/* Проверка, занята ли позиция сущностью */
int arena_is_position_occupied(Arena *arena, Vec2 pos) {
    return arena_get_entity_at(arena, pos) != NULL;
}

/* Подсчёт живых сущностей */
int arena_count_alive(Arena *arena) {
    int count = 0;
    for (int i = 0; i < arena->entity_count; i++) {
        if (arena->entities[i].alive) {
            count++;
        }
    }
    return count;
}

/* Удаление уничтоженных заклинаний */
void arena_cleanup_spells(Arena *arena) {
    int write_idx = 0;
    for (int i = 0; i < arena->spell_count; i++) {
        if (!arena->spells[i].destroyed) {
            if (write_idx != i) {
                arena->spells[write_idx] = arena->spells[i];
            }
            write_idx++;
        }
    }
    arena->spell_count = write_idx;
}

/* Освобождение памяти арены */
void arena_destroy(Arena *arena) {
    map_destroy(&arena->map);
    arena->entity_count = 0;
    arena->spell_count = 0;
}

