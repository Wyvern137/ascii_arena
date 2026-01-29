/*
 * app.c - Реализация клиентского приложения
 * Интегрирует новое меню и арену
 */

#include "app.h"
#include "../ui/terminal.h"
#include "../ui/input.h"
#include "../ui/menu.h"
#include "../ui/arena_view.h"
#include "../net/encoder.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <ncurses.h>

/* Константы */
#define FRAME_TIME_MS 16    /* ~60 FPS */
#define PROTOCOL_VERSION "1.0.0"

/* Создание приложения */
ClientApp client_app_create(const char *host, int tcp_port) {
    ClientApp app;
    memset(&app, 0, sizeof(app));
    
    app.state = client_state_create();
    app.running = 1;
    
    /* Формируем адрес по умолчанию */
    char default_addr[64] = "";
    if (host) {
        strncpy(app.state.host, host, sizeof(app.state.host) - 1);
        snprintf(default_addr, sizeof(default_addr), "%s:%d", host, tcp_port > 0 ? tcp_port : 3042);
    }
    if (tcp_port > 0) {
        app.state.tcp_port = tcp_port;
    }
    
    /* Создаём меню */
    app.menu = menu_create(default_addr, app.state.player_symbol ? app.state.player_symbol : 'A');
    
    /* Создаём view арены (будет настроена при получении информации о сервере) */
    app.arena_view = arena_view_create(20, 5);
    
    return app;
}

/* Получение текущего времени в миллисекундах */
static long get_time_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

/* Синхронизация состояния меню с состоянием клиента */
static void sync_menu_state(ClientApp *app) {
    /* Синхронизация статуса подключения */
    ConnectionStatus conn_status;
    switch (app->state.state) {
        case CLIENT_STATE_CONNECTING:
            conn_status = CONNECTION_CONNECTING;
            break;
        case CLIENT_STATE_MENU:
            /* Клиент может быть в состоянии MENU и при этом быть подключенным к серверу
             * (когда он подключен, получил информацию о сервере, но еще не залогинен).
             * Проверяем наличие валидного TCP сокета. */
            if (socket_is_valid(&app->state.tcp_socket)) {
                conn_status = CONNECTION_CONNECTED;
            } else {
                conn_status = CONNECTION_NOT_CONNECTED;
            }
            break;
        case CLIENT_STATE_WAITING:
        case CLIENT_STATE_PLAYING:
        case CLIENT_STATE_GAME_OVER:
            conn_status = CONNECTION_CONNECTED;
            break;
        case CLIENT_STATE_DISCONNECTED:
            conn_status = CONNECTION_NOT_FOUND;
            break;
        default:
            conn_status = CONNECTION_NOT_CONNECTED;
    }
    menu_set_connection_status(&app->menu, conn_status);
    
    /* Синхронизация списка игроков */
    menu_set_players(&app->menu, app->state.players, app->state.player_count);
    
    /* Синхронизация таймера */
    if (app->state.wait_seconds > 0) {
        menu_set_countdown(&app->menu, app->state.wait_seconds);
    }
}

/* Синхронизация состояния арены */
static void sync_arena_state(ClientApp *app) {
    /* Очищаем предыдущие данные */
    arena_view_clear_entities(&app->arena_view);
    arena_view_clear_spells(&app->arena_view);
    arena_view_clear_players(&app->arena_view);
    
    /* Добавляем игроков */
    for (int i = 0; i < app->state.player_data_count; i++) {
        PlayerData *p = &app->state.player_data[i];
        
        /* Ищем entity_id для этого игрока */
        int entity_id = -1;
        for (int j = 0; j < app->state.entity_count; j++) {
            if (app->state.entities[j].symbol == p->symbol) {
                entity_id = j;
                break;
            }
        }
        
        int is_current = (p->symbol == app->state.player_symbol);
        arena_view_set_player(&app->arena_view, i, p->symbol, p->points, entity_id, is_current);
    }
    
    /* Добавляем сущности */
    for (int i = 0; i < app->state.entity_count; i++) {
        EntityData *e = &app->state.entities[i];
        arena_view_set_entity(&app->arena_view, i, e->symbol, e->pos_x, e->pos_y,
                             e->health, 100, e->energy, 100, e->direction, e->spell_type, 1);
    }
    
    /* Добавляем заклинания */
    for (int i = 0; i < app->state.spell_count; i++) {
        SpellData *s = &app->state.spells[i];
        arena_view_set_spell(&app->arena_view, s->id, s->pos_x, s->pos_y, s->direction, s->spell_type, 1);
    }
    
    /* Устанавливаем текущего игрока */
    for (int i = 0; i < app->state.entity_count; i++) {
        if (app->state.entities[i].symbol == app->state.player_symbol) {
            arena_view_set_current_player(&app->arena_view, i, app->state.entities[i].direction);
            break;
        }
    }
    
    /* Устанавливаем локальное направление (на основе нажатых клавиш) */
    arena_view_set_local_direction(&app->arena_view, app->state.last_direction);
}

/* Обработка одного полного пакета от сервера */
static void handle_single_packet(ClientApp *app, uint8_t *buffer, int len);

/* Обработка TCP буфера - извлекает и обрабатывает полные пакеты */
static void process_tcp_buffer(ClientApp *app) {
    while (app->state.tcp_buffer_len >= PACKET_HEADER_SIZE) {
        PacketHeader header;
        decode_packet_header(app->state.tcp_buffer, &header);
        
        size_t packet_size = PACKET_HEADER_SIZE + header.data_length;
        
        /* Проверяем, есть ли полный пакет в буфере */
        if (app->state.tcp_buffer_len < packet_size) {
            /* Неполный пакет, ждём больше данных */
            break;
        }
        
        /* Обрабатываем полный пакет */
        handle_single_packet(app, app->state.tcp_buffer, (int)packet_size);
        
        /* Сдвигаем оставшиеся данные в начало буфера */
        if (app->state.tcp_buffer_len > packet_size) {
            memmove(app->state.tcp_buffer, 
                    app->state.tcp_buffer + packet_size,
                    app->state.tcp_buffer_len - packet_size);
        }
        app->state.tcp_buffer_len -= packet_size;
    }
}

/* Обработка сообщения от сервера (для UDP - без буферизации) */
static void handle_server_message(ClientApp *app, uint8_t *buffer, int len) {
    if (len < (int)PACKET_HEADER_SIZE) return;
    handle_single_packet(app, buffer, len);
}

/* Обработка одного полного пакета от сервера */
static void handle_single_packet(ClientApp *app, uint8_t *buffer, int len) {
    if (len < (int)PACKET_HEADER_SIZE) return;
    
    PacketHeader header;
    decode_packet_header(buffer, &header);
    uint8_t *data = buffer + PACKET_HEADER_SIZE;
    switch (header.message_type) {
        case SERVER_MSG_VERSION: {
            /* Ответ версии - декодируем и отправляем подписку на информацию */
            uint8_t version_len = data[0];
            char server_version[33];
            if (version_len > 32) version_len = 32;
            memcpy(server_version, data + 1, version_len);
            server_version[version_len] = '\0';
            int compatible = data[1 + version_len];
            
            menu_set_version_info(&app->menu, server_version, compatible);
            
            uint8_t buf[64];
            int n = encode_subscribe_info(buf);
            socket_send_all(&app->state.tcp_socket, buf, (size_t)n);
            break;
        }
        
        case SERVER_MSG_STATIC_INFO: {
            decode_static_info(data, &app->state.udp_port, &app->state.map_size,
                             &app->state.winner_points, &app->state.max_players);
            /* Обновляем меню */
            menu_set_server_info(&app->menu, app->state.udp_port, app->state.map_size,
                               app->state.winner_points, app->state.max_players);
            
            /* Обновляем view арены */
            app->arena_view.map_size = app->state.map_size;
            app->arena_view.winner_points = app->state.winner_points;
            
            /* Переходим в состояние MENU для обработки выбора имени и логина.
             * НЕ отправляем логин автоматически - пользователь должен сам выбрать имя
             * и нажать Enter для входа. Информация о сервере и список игроков
             * будут отображены, чтобы пользователь мог выбрать свободное имя. */
            client_state_set(&app->state, CLIENT_STATE_MENU);
            break;
        }
        
        case SERVER_MSG_DYNAMIC_INFO: {
            app->state.player_count = data[0];
            for (int i = 0; i < app->state.player_count && i < MAX_PLAYERS; i++) {
                app->state.players[i] = (char)data[1 + i];
            }
            
            /* Обновляем меню */
            menu_set_players(&app->menu, app->state.players, app->state.player_count);
            break;
        }
        
        case SERVER_MSG_LOGIN_STATUS: {
            char symbol;
            LoginStatus status;
            decode_login_status(data, &symbol, &status, &app->state.session_token);
            
            if (status == LOGIN_OK) {
                menu_set_login_status(&app->menu, LOGIN_LOGGED);
                client_state_set(&app->state, CLIENT_STATE_WAITING);
                
                /* Создаём UDP сокет и отправляем handshake */
                app->state.udp_socket = socket_udp_create();
                socket_set_nonblocking(&app->state.udp_socket);
                
                /* Копируем адрес сервера для UDP */
                app->state.udp_socket.addr = app->state.tcp_socket.addr;
                app->state.udp_socket.addr.sin_port = htons((uint16_t)app->state.udp_port);
                
                uint8_t buf[64];
                int n = encode_connect_udp(buf, app->state.session_token);
                socket_sendto(&app->state.udp_socket, buf, (size_t)n, &app->state.udp_socket.addr);
            } else if (status == LOGIN_INVALID_CHAR) {
                menu_set_login_status(&app->menu, LOGIN_INVALID_NAME);
            } else if (status == LOGIN_ALREADY_USED) {
                menu_set_login_status(&app->menu, LOGIN_NAME_TAKEN);
            } else if (status == LOGIN_ROOM_FULL) {
                menu_set_login_status(&app->menu, LOGIN_PLAYER_LIMIT);
            }
            break;
        }
        
        case SERVER_MSG_UDP_CONNECTED: {
            app->state.udp_connected = 1;
            menu_set_udp_confirmed(&app->menu, 1);
            
            /* Отправляем подтверждение */
            uint8_t buf[64];
            int n = encode_trust_udp(buf);
            socket_send_all(&app->state.tcp_socket, buf, (size_t)n);
            break;
        }
        
        case SERVER_MSG_START_GAME: {
            client_state_set(&app->state, CLIENT_STATE_PLAYING);
            app->arena_view.game_finished = 0;
            app->arena_view.next_arena_countdown = -1;
            break;
        }
        
        case SERVER_MSG_FINISH_GAME: {
            app->state.winner = (char)data[0];
            client_state_set(&app->state, CLIENT_STATE_GAME_OVER);
            arena_view_set_game_finished(&app->arena_view, app->state.winner);
            break;
        }
        
        case SERVER_MSG_WAIT_ARENA: {
            uint16_t secs;
            memcpy(&secs, data, 2);
            app->state.wait_seconds = secs;
            
            /* Находим выжившего */
            char survivor = '\0';
            for (int i = 0; i < app->state.entity_count; i++) {
                if (app->state.entities[i].health > 0) {
                    survivor = app->state.entities[i].symbol;
                    break;
                }
            }
            
            arena_view_set_next_arena(&app->arena_view, secs, survivor);
            menu_set_countdown(&app->menu, secs);
            break;
        }
        
        case SERVER_MSG_GAME_STEP: {
            decode_game_step(data, (size_t)header.data_length,
                           app->state.entities, &app->state.entity_count,
                           app->state.spells, &app->state.spell_count,
                           app->state.player_data, &app->state.player_data_count);
            
            /* Синхронизируем view арены */
            sync_arena_state(app);
            break;
        }
        
        default:
            break;
    }
}

/* Парсинг адреса сервера (host:port) */
static int parse_server_addr(const char *addr, char *host, int *port) {
    const char *colon = strrchr(addr, ':');
    if (!colon) {
        strncpy(host, addr, 255);
        *port = 3042;  /* Порт по умолчанию */
        return 0;
    }
    
    int host_len = (int)(colon - addr);
    if (host_len > 255) host_len = 255;
    strncpy(host, addr, host_len);
    host[host_len] = '\0';
    
    *port = atoi(colon + 1);
    if (*port <= 0 || *port > 65535) {
        *port = 3042;
    }
    
    return 0;
}

/* Главный цикл приложения */
void client_app_run(ClientApp *app) {
    terminal_init();
    app->renderer = renderer_create();
    
    int screen_width, screen_height;
    int prev_screen_width = 0, prev_screen_height = 0;
    terminal_get_size(&screen_width, &screen_height);
    
    long last_frame = get_time_ms();
    int frame_counter = 0;  /* Счётчик кадров для периодических операций */
    
    while (app->running) {
        long now = get_time_ms();
        long elapsed = now - last_frame;
        
        if (elapsed < FRAME_TIME_MS) {
            usleep((unsigned int)((FRAME_TIME_MS - elapsed) * 1000));
            continue;
        }
        last_frame = now;
        frame_counter++;
        
        /* Получаем размер экрана только каждые 30 кадров (~0.5 сек) или при первом запуске */
        if (frame_counter % 30 == 0 || prev_screen_width == 0) {
            terminal_get_size(&screen_width, &screen_height);
            /* Очищаем экран при изменении размера */
            if (screen_width != prev_screen_width || screen_height != prev_screen_height) {
                prev_screen_width = screen_width;
                prev_screen_height = screen_height;
                clear();  /* Полная очистка при изменении размера */
            }
        }
        
        /* Обработка ввода */
        int key = input_get_key();
        if (key != ERR) {
            client_app_handle_input(app, key);
        }
        
        /* Обработка сети */
        client_app_handle_network(app);
        
        /* Обновление состояния */
        menu_update(&app->menu);
        arena_view_update(&app->arena_view);
        arena_view_update_interpolation(&app->arena_view, FRAME_TIME_MS / 1000.0f);
        sync_menu_state(app);
        
        /* Обновляем локальное направление каждый кадр для мгновенного отображения индикатора */
        if (app->state.state == CLIENT_STATE_PLAYING) {
            arena_view_set_local_direction(&app->arena_view, app->state.last_direction);
        }
        
        /* Рендеринг - используем erase() вместо clear() для уменьшения мерцания */
        erase();
        
        switch (app->state.state) {
            case CLIENT_STATE_MENU:
            case CLIENT_STATE_CONNECTING:
            case CLIENT_STATE_WAITING:
                /* Отрисовка меню */
                menu_render(&app->menu, screen_width, screen_height);
                break;
                
            case CLIENT_STATE_PLAYING:
            case CLIENT_STATE_GAME_OVER:
                /* Отрисовка арены */
                arena_view_render(&app->arena_view, screen_width, screen_height);
                break;
                
            case CLIENT_STATE_DISCONNECTED:
                /* Отрисовка меню с сообщением об отключении */
                menu_set_connection_status(&app->menu, CONNECTION_LOST);
                menu_render(&app->menu, screen_width, screen_height);
                break;
        }
        
        renderer_refresh();
    }
    
    terminal_cleanup();
}

/* Обработка ввода */
void client_app_handle_input(ClientApp *app, int key) {
    switch (app->state.state) {
        case CLIENT_STATE_MENU:
        case CLIENT_STATE_WAITING: {
            /* Передаём ввод в меню */
            int unhandled = menu_handle_key(&app->menu, key);
            
            if (unhandled == 27) {  /* Escape */
                if (menu_is_logged(&app->menu)) {
                    /* Logout */
                    client_app_disconnect(app);
                    menu_set_login_status(&app->menu, LOGIN_NOT_LOGGED);
                    menu_set_connection_status(&app->menu, CONNECTION_NOT_CONNECTED);
                    client_state_set(&app->state, CLIENT_STATE_MENU);
                } else if (menu_is_connected(&app->menu)) {
                    /* Disconnect */
                    client_app_disconnect(app);
                    menu_set_connection_status(&app->menu, CONNECTION_NOT_CONNECTED);
                    client_state_set(&app->state, CLIENT_STATE_MENU);
                } else {
                    /* Exit */
                    app->running = 0;
                }
            } else if (unhandled == '\n' || unhandled == KEY_ENTER) {
                if (!menu_is_connected(&app->menu)) {
                    /* Connect */
                    const char *addr = menu_get_server_addr(&app->menu);
                    if (addr && strlen(addr) > 0) {
                        parse_server_addr(addr, app->state.host, &app->state.tcp_port);
                        client_app_connect(app);
                    }
                } else if (!menu_is_logged(&app->menu)) {
                    /* Login */
                    char player_char = menu_get_character(&app->menu);
                    if (player_char) {
                        app->state.player_symbol = player_char;
                        uint8_t buf[64];
                        int n = encode_login(buf, player_char);
                        socket_send_all(&app->state.tcp_socket, buf, (size_t)n);
                    }
                }
            }
            break;
        }
        
        case CLIENT_STATE_CONNECTING:
            if (key == 27) {  /* Escape */
                client_app_disconnect(app);
                menu_set_connection_status(&app->menu, CONNECTION_NOT_CONNECTED);
                client_state_set(&app->state, CLIENT_STATE_MENU);
            }
            break;
            
        case CLIENT_STATE_PLAYING: {
            if (key == 27 || input_is_quit_key(key)) {  /* Escape или Q */
                client_app_disconnect(app);
                menu_set_login_status(&app->menu, LOGIN_NOT_LOGGED);
                menu_set_connection_status(&app->menu, CONNECTION_NOT_CONNECTED);
                client_state_set(&app->state, CLIENT_STATE_MENU);
            } else if (input_is_spell_1_key(key)) {
                /* Переключение на базовую атаку */
                app->state.selected_spell_type = 1;
            } else if (input_is_spell_2_key(key)) {
                /* Переключение на усиленную атаку */
                app->state.selected_spell_type = 2;
            } else if (input_is_action_key(key)) {
                /* Атака в последнем выбранном направлении */
                Direction dir = (Direction)app->state.last_direction;
                client_app_send_cast(app, dir);
            } else {
                Direction dir = input_key_to_direction(key);
                if (dir != DIR_NONE) {
                    /* Сохраняем последнее направление */
                    app->state.last_direction = (int)dir;
                    client_app_send_move(app, dir);
                }
            }
            break;
        }
            
        case CLIENT_STATE_GAME_OVER:
            if (key == '\n' || key == KEY_ENTER) {
                /* Возврат в меню */
                client_app_disconnect(app);
                menu_set_login_status(&app->menu, LOGIN_NOT_LOGGED);
                menu_set_connection_status(&app->menu, CONNECTION_NOT_CONNECTED);
                client_state_set(&app->state, CLIENT_STATE_MENU);
                app->arena_view.game_finished = 0;
            }
            break;
            
        case CLIENT_STATE_DISCONNECTED:
            if (key == 27) {  /* Escape */
                app->running = 0;
            } else {
                client_state_set(&app->state, CLIENT_STATE_MENU);
                menu_set_connection_status(&app->menu, CONNECTION_NOT_CONNECTED);
            }
            break;
    }
}

/* Обработка сетевых событий */
void client_app_handle_network(ClientApp *app) {
    /* Обрабатываем сетевые события во всех состояниях, кроме DISCONNECTED.
     * Важно: клиент может быть подключен к серверу и находиться в состоянии MENU
     * (когда он подключен, но еще не залогинен), и в этом состоянии он должен
     * продолжать обрабатывать сетевые события (например, SERVER_MSG_DYNAMIC_INFO). */
    if (app->state.state == CLIENT_STATE_DISCONNECTED) {
        return;
    }
    
    /* Проверяем TCP с буферизацией */
    if (socket_is_valid(&app->state.tcp_socket) && socket_has_data(&app->state.tcp_socket, 0)) {
        /* Читаем данные в конец TCP буфера */
        size_t available_space = TCP_BUFFER_SIZE - app->state.tcp_buffer_len;
        if (available_space > 0) {
            int n = socket_recv(&app->state.tcp_socket, 
                               app->state.tcp_buffer + app->state.tcp_buffer_len,
                               available_space);
            if (n > 0) {
                app->state.tcp_buffer_len += (size_t)n;
                /* Обрабатываем все полные пакеты в буфере */
                process_tcp_buffer(app);
            } else if (n == 0) {
                /* Сервер закрыл соединение */
                client_state_set(&app->state, CLIENT_STATE_DISCONNECTED);
                menu_set_connection_status(&app->menu, CONNECTION_LOST);
            } else if (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
                /* Ошибка чтения */
                client_state_set(&app->state, CLIENT_STATE_DISCONNECTED);
                menu_set_connection_status(&app->menu, CONNECTION_LOST);
            }
        }
    }
    
    /* Проверяем UDP (без буферизации - UDP пакеты приходят целиком) */
    if (socket_is_valid(&app->state.udp_socket) && socket_has_data(&app->state.udp_socket, 0)) {
        uint8_t udp_buffer[MAX_PACKET_SIZE];
        struct sockaddr_in src;
        int n = socket_recvfrom(&app->state.udp_socket, udp_buffer, sizeof(udp_buffer), &src);
        if (n > 0) {
            handle_server_message(app, udp_buffer, n);
        }
    }
}

/* Подключение к серверу */
int client_app_connect(ClientApp *app) {
    client_state_set(&app->state, CLIENT_STATE_CONNECTING);
    menu_set_connection_status(&app->menu, CONNECTION_CONNECTING);
    
    /* Создаём TCP сокет */
    app->state.tcp_socket = socket_tcp_create();
    if (!socket_is_valid(&app->state.tcp_socket)) {
        client_state_set(&app->state, CLIENT_STATE_DISCONNECTED);
        menu_set_connection_status(&app->menu, CONNECTION_NOT_FOUND);
        return -1;
    }
    
    /* Подключаемся */
    if (socket_connect(&app->state.tcp_socket, app->state.host, app->state.tcp_port) < 0) {
        socket_close(&app->state.tcp_socket);
        client_state_set(&app->state, CLIENT_STATE_MENU);
        menu_set_connection_status(&app->menu, CONNECTION_NOT_FOUND);
        return -1;
    }
    
    socket_set_nonblocking(&app->state.tcp_socket);
    menu_set_connection_status(&app->menu, CONNECTION_CONNECTED);
    
    /* Проверяем готовность сокета к записи перед первой отправкой */
    if (!socket_is_writable(&app->state.tcp_socket, 1000)) {
        /* Сокет не готов к записи в течение 1 секунды */
        socket_close(&app->state.tcp_socket);
        client_state_set(&app->state, CLIENT_STATE_MENU);
        menu_set_connection_status(&app->menu, CONNECTION_NOT_FOUND);
        return -1;
    }
    
    /* Отправляем версию */
    uint8_t buffer[64];
    int n = encode_version(buffer, PROTOCOL_VERSION);
    socket_send_all(&app->state.tcp_socket, buffer, (size_t)n);
    
    return 0;
}

/* Отключение от сервера */
void client_app_disconnect(ClientApp *app) {
    if (socket_is_valid(&app->state.tcp_socket)) {
        uint8_t buffer[64];
        int n = encode_logout(buffer);
        socket_send_all(&app->state.tcp_socket, buffer, (size_t)n);
    }
    
    client_state_reset(&app->state);
}

/* Отправка движения */
void client_app_send_move(ClientApp *app, Direction dir) {
    if (!socket_is_valid(&app->state.tcp_socket)) return;
    
    uint8_t buffer[64];
    int n = encode_move_player(buffer, dir);
    socket_send_all(&app->state.tcp_socket, buffer, (size_t)n);
}

/* Отправка атаки */
void client_app_send_cast(ClientApp *app, Direction dir) {
    if (!socket_is_valid(&app->state.tcp_socket)) return;
    
    uint8_t buffer[64];
    int n = encode_cast_skill(buffer, dir, app->state.selected_spell_type);
    socket_send_all(&app->state.tcp_socket, buffer, (size_t)n);
}

/* Освобождение ресурсов */
void client_app_destroy(ClientApp *app) {
    client_state_destroy(&app->state);
    renderer_destroy(&app->renderer);
}
