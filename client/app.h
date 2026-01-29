/*
 * app.h - Клиентское приложение
 * Главный цикл клиента, обработка ввода и сети
 */

#ifndef CLIENT_APP_H
#define CLIENT_APP_H

#include "state.h"
#include "../ui/renderer.h"
#include "../ui/menu.h"
#include "../ui/arena_view.h"

/* Клиентское приложение */
typedef struct {
    ClientState state;      /* Состояние клиента */
    Renderer renderer;      /* Рендерер */
    Menu menu;              /* Меню */
    ArenaView arena_view;   /* View арены */
    int running;            /* Флаг работы */
} ClientApp;

/* Создание приложения */
ClientApp client_app_create(const char *host, int tcp_port);

/* Главный цикл приложения */
void client_app_run(ClientApp *app);

/* Обработка ввода */
void client_app_handle_input(ClientApp *app, int key);

/* Обработка сетевых событий */
void client_app_handle_network(ClientApp *app);

/* Подключение к серверу */
int client_app_connect(ClientApp *app);

/* Отключение от сервера */
void client_app_disconnect(ClientApp *app);

/* Отправка действия игрока */
void client_app_send_move(ClientApp *app, Direction dir);
void client_app_send_cast(ClientApp *app, Direction dir);

/* Освобождение ресурсов */
void client_app_destroy(ClientApp *app);

#endif /* CLIENT_APP_H */
