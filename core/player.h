/*
 * player.h - Игрок
 * Представляет игрока с очками и связью с сущностью на арене
 */

#ifndef PLAYER_H
#define PLAYER_H

/* Максимальное количество игроков */
#define MAX_PLAYERS 8

/* Игрок */
typedef struct {
    char symbol;    /* Символ игрока */
    int points;     /* Набранные очки */
    int entity_id;  /* ID сущности на арене (-1 если нет) */
    int connected;  /* Флаг подключения */
} Player;

/* Создание игрока */
Player player_create(char symbol);

/* Добавление очков игроку */
void player_add_points(Player *player, int points);

/* Проверка, жив ли игрок на арене */
int player_is_alive(Player *player);

/* Сброс игрока для новой арены */
void player_reset(Player *player);

#endif /* PLAYER_H */

