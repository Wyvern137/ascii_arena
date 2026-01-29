/*
 * spell.c - Реализация заклинаний
 */

#include "spell.h"

/* Время между перемещениями заклинания (при скорости 15.0 - быстрое движение) */
#define SPELL_MOVE_INTERVAL 0.066f

/* Создание заклинания */
Spell spell_create(int id, int caster_id, Vec2 pos, Direction dir, int damage, float speed, int spell_type) {
    Spell s;
    s.id = id;
    s.caster_id = caster_id;
    s.position = pos;
    s.direction = dir;
    s.damage = damage;
    s.speed = speed;
    s.move_timer = 0.0f;
    s.spell_type = spell_type;
    s.affected_count = 0;
    s.destroyed = 0;
    
    /* Инициализируем массив затронутых сущностей */
    for (int i = 0; i < MAX_AFFECTED; i++) {
        s.affected_ids[i] = -1;
    }
    
    return s;
}

/* Обновление заклинания (движение) */
void spell_update(Spell *spell, float delta_time) {
    if (spell->destroyed) return;
    
    spell->move_timer += delta_time * spell->speed;
    
    /* Перемещаем заклинание, когда таймер достигает интервала */
    while (spell->move_timer >= SPELL_MOVE_INTERVAL) {
        spell->move_timer -= SPELL_MOVE_INTERVAL;
        Vec2 delta = direction_to_vec2(spell->direction);
        spell->position = vec2_add(spell->position, delta);
    }
}

/* Проверка, затронута ли сущность */
int spell_has_affected(Spell *spell, int entity_id) {
    for (int i = 0; i < spell->affected_count; i++) {
        if (spell->affected_ids[i] == entity_id) {
            return 1;
        }
    }
    return 0;
}

/* Пометить сущность как затронутую */
void spell_mark_affected(Spell *spell, int entity_id) {
    if (spell->affected_count < MAX_AFFECTED && !spell_has_affected(spell, entity_id)) {
        spell->affected_ids[spell->affected_count++] = entity_id;
    }
}

/* Уничтожить заклинание */
void spell_destroy(Spell *spell) {
    spell->destroyed = 1;
}

/* Получить следующую позицию заклинания */
Vec2 spell_next_position(Spell *spell) {
    Vec2 delta = direction_to_vec2(spell->direction);
    return vec2_add(spell->position, delta);
}

