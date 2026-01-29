/*
 * entity.c - Реализация игровых сущностей
 */

#include "entity.h"

/* Константы кулдаунов */
#define MOVE_COOLDOWN 0.15f    /* Кулдаун движения в секундах */
#define SKILL_COOLDOWN 0.5f   /* Кулдаун способности в секундах */

/* Время анимации урона в секундах (66ms) */
#define DAMAGE_ANIMATION_TIME 0.066f

/* Создание сущности */
Entity entity_create(int id, char symbol, Vec2 pos, int max_health, int max_energy) {
    Entity e;
    e.id = id;
    e.symbol = symbol;
    e.position = pos;
    e.health = max_health;
    e.max_health = max_health;
    e.energy = max_energy;
    e.max_energy = max_energy;
    e.direction = DIR_DOWN;
    e.spell_type = SPELL_TYPE_BASIC;  /* По умолчанию базовая атака */
    e.alive = 1;
    e.move_cooldown = 0.0f;
    e.skill_cooldown = 0.0f;
    e.damage_timer = 0.0f;
    return e;
}

/* Перемещение сущности */
void entity_move(Entity *entity, Vec2 new_pos) {
    if (entity->alive) {
        entity->position = new_pos;
        entity->move_cooldown = MOVE_COOLDOWN;
    }
}

/* Получение урона */
void entity_take_damage(Entity *entity, int damage) {
    if (!entity->alive) return;
    
    entity->health -= damage;
    entity->damage_timer = DAMAGE_ANIMATION_TIME;  /* Запускаем анимацию урона */
    
    if (entity->health <= 0) {
        entity->health = 0;
        entity->alive = 0;
    }
}

/* Восстановление здоровья */
void entity_heal(Entity *entity, int amount) {
    if (!entity->alive) return;
    
    entity->health += amount;
    if (entity->health > entity->max_health) {
        entity->health = entity->max_health;
    }
}

/* Использование энергии - возвращает 1 если успешно, 0 если не хватает */
int entity_use_energy(Entity *entity, int amount) {
    if (entity->energy >= amount) {
        entity->energy -= amount;
        entity->skill_cooldown = SKILL_COOLDOWN;
        return 1;
    }
    return 0;
}

/* Восстановление энергии */
void entity_restore_energy(Entity *entity, int amount) {
    entity->energy += amount;
    if (entity->energy > entity->max_energy) {
        entity->energy = entity->max_energy;
    }
}

/* Проверка, жива ли сущность */
int entity_is_alive(Entity *entity) {
    return entity->alive;
}

/* Обновление кулдаунов */
void entity_update_cooldowns(Entity *entity, float delta_time) {
    if (entity->move_cooldown > 0) {
        entity->move_cooldown -= delta_time;
        if (entity->move_cooldown < 0) entity->move_cooldown = 0;
    }
    if (entity->skill_cooldown > 0) {
        entity->skill_cooldown -= delta_time;
        if (entity->skill_cooldown < 0) entity->skill_cooldown = 0;
    }
    if (entity->damage_timer > 0) {
        entity->damage_timer -= delta_time;
        if (entity->damage_timer < 0) entity->damage_timer = 0;
    }
}

/* Проверка, может ли сущность двигаться */
int entity_can_move(Entity *entity) {
    return entity->alive && entity->move_cooldown <= 0;
}

/* Проверка, может ли сущность использовать способность */
int entity_can_cast(Entity *entity) {
    if (!entity->alive || entity->skill_cooldown > 0) {
        return 0;
    }
    /* Базовая атака не требует маны, усиленная требует 10 */
    if (entity->spell_type == SPELL_TYPE_POWER && entity->energy < 10) {
        return 0;
    }
    return 1;
}

