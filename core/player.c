/*
 * player.c - Реализация игрока
 */

#include "player.h"

/* Создание игрока */
Player player_create(char symbol) {
    Player p;
    p.symbol = symbol;
    p.points = 0;
    p.entity_id = -1;
    p.connected = 0;
    return p;
}

/* Добавление очков игроку */
void player_add_points(Player *player, int points) {
    player->points += points;
}

/* Проверка, жив ли игрок на арене (имеет сущность) */
int player_is_alive(Player *player) {
    return player->entity_id >= 0;
}

/* Сброс игрока для новой арены */
void player_reset(Player *player) {
    player->entity_id = -1;
}

