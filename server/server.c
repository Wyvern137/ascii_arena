/*
 * server.c - Реализация серверной логики
 */

#include "server.h"
#include "../net/encoder.h"
#include "../net/protocol.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>

#define FRAME_TIME_MS 16    /* ~60 FPS */
#define PROTOCOL_VERSION "1.0.0"

/* Прототипы внутренних функций */
static void process_session_tcp_buffer(Server *server, Session **session);
static void handle_single_packet(Server *server, Session **session, uint8_t *buffer, int len);

/* Получение времени в миллисекундах */
static long get_time_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

/* Создание сервера */
Server* server_create(int tcp_port, int udp_port, int max_players, int map_size, int winner_points) {
    Server *server = (Server *)malloc(sizeof(Server));
    if (!server) return NULL;
    
    server->tcp_port = tcp_port;
    server->udp_port = udp_port;
    server->running = 0;
    
    /* Создаём TCP listener с автоматическим выбором порта */
    server->tcp_listener = socket_tcp_create();
    int actual_tcp_port = tcp_port;
    if (socket_bind_with_fallback(&server->tcp_listener, &actual_tcp_port, 10) < 0) {
        fprintf(stderr, "Ошибка: не удалось привязать TCP порт (пробовали %d-%d)\n", 
                tcp_port, tcp_port + 9);
        free(server);
        return NULL;
    }
    server->tcp_port = actual_tcp_port;
    
    if (socket_listen(&server->tcp_listener, 10) < 0) {
        fprintf(stderr, "Ошибка: не удалось начать прослушивание\n");
        socket_close(&server->tcp_listener);
        free(server);
        return NULL;
    }
    socket_set_nonblocking(&server->tcp_listener);
    
    /* Создаём UDP сокет с автоматическим выбором порта */
    server->udp_socket = socket_udp_create();
    int actual_udp_port = udp_port;
    if (socket_bind_with_fallback(&server->udp_socket, &actual_udp_port, 10) < 0) {
        fprintf(stderr, "Ошибка: не удалось привязать UDP порт (пробовали %d-%d)\n",
                udp_port, udp_port + 9);
        socket_close(&server->tcp_listener);
        free(server);
        return NULL;
    }
    server->udp_port = actual_udp_port;
    socket_set_nonblocking(&server->udp_socket);
    
    /* Создаём комнату и игру */
    server->room = room_session_create(max_players);
    server->game = game_create(map_size, winner_points, max_players);
    
    printf("Сервер запущен на TCP:%d UDP:%d\n", server->tcp_port, server->udp_port);
    printf("Ожидание %d игроков...\n", max_players);
    
    return server;
}

/* Главный цикл сервера */
void server_run(Server *server) {
    server->running = 1;
    long last_frame = get_time_ms();
    
    while (server->running) {
        long now = get_time_ms();
        long elapsed = now - last_frame;
        
        if (elapsed < FRAME_TIME_MS) {
            usleep((unsigned int)((FRAME_TIME_MS - elapsed) * 1000));
            continue;
        }
        last_frame = now;
        
        /* Проверяем новые подключения */
        server_handle_connection(server);
        
        /* Обрабатываем TCP сообщения от клиентов с буферизацией */
        for (int i = 0; i < MAX_PLAYERS; i++) {
            Session *s = &server->room.sessions[i];
            if (s->tcp_socket.fd < 0) continue;  /* Пропускаем сессии без сокета */
            
            if (socket_has_data(&s->tcp_socket, 0)) {
                /* Читаем данные в конец TCP буфера */
                size_t available_space = SESSION_TCP_BUFFER_SIZE - s->tcp_buffer_len;
                if (available_space > 0) {
                    int n = socket_recv(&s->tcp_socket, 
                                       s->tcp_buffer + s->tcp_buffer_len,
                                       available_space);
                    if (n > 0) {
                        s->tcp_buffer_len += (size_t)n;
                        /* Обрабатываем все полные пакеты в буфере */
                        /* Передаем указатель на указатель, чтобы можно было изменить сессию */
                        process_session_tcp_buffer(server, &s);
                        /* После обработки s может указывать на другую сессию (например, после LOGIN) */
                    } else if (n == 0) {
                        /* Клиент отключился */
                        if (s->active) {
                            printf("Игрок %c отключился\n", s->symbol);
                            game_remove_player(server->game, game_get_player_index(server->game, s->symbol));
                            room_session_remove(&server->room, s->token);
                            
                            /* Отправляем обновлённый список игроков */
                            char symbols[MAX_PLAYERS];
                            int count = room_session_get_symbols(&server->room, symbols);
                            uint8_t buf[64];
                            int len = encode_dynamic_info(buf, symbols, count);
                            server_broadcast(server, buf, len);
                        } else {
                            /* Неавторизованный клиент отключился */
                            socket_close(&s->tcp_socket);
                            s->tcp_buffer_len = 0;
                        }
                    } else if (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
                        /* Ошибка чтения */
                        if (s->active) {
                            printf("Ошибка чтения от игрока %c\n", s->symbol);
                            game_remove_player(server->game, game_get_player_index(server->game, s->symbol));
                            room_session_remove(&server->room, s->token);
                            
                            char symbols[MAX_PLAYERS];
                            int count = room_session_get_symbols(&server->room, symbols);
                            uint8_t buf[64];
                            int len = encode_dynamic_info(buf, symbols, count);
                            server_broadcast(server, buf, len);
                        } else {
                            socket_close(&s->tcp_socket);
                            s->tcp_buffer_len = 0;
                        }
                    }
                }
            }
        }
        
        /* Обрабатываем UDP сообщения */
        if (socket_has_data(&server->udp_socket, 0)) {
            uint8_t buffer[MAX_PACKET_SIZE];
            struct sockaddr_in src;
            int n = socket_recvfrom(&server->udp_socket, buffer, sizeof(buffer), &src);
            if (n > 0) {
                server_handle_udp(server, buffer, n, &src);
            }
        }
        
        /* Обновляем игру */
        if (server->game->state == GAME_STATE_PLAYING) {
            game_step(server->game, FRAME_TIME_MS / 1000.0f);
            server_broadcast_game_step(server);
            
            /* Проверяем окончание игры */
            if (server->game->state == GAME_STATE_FINISHED) {
                char winner = game_get_winner(server->game);
                printf("Игра окончена! Победитель: %c\n", winner);
                
                uint8_t buf[64];
                int len = encode_finish_game(buf, winner);
                server_broadcast(server, buf, len);
                
                /* Сбрасываем игру */
                server->game->state = GAME_STATE_WAITING;
            }
        }
        
        /* Проверяем, готова ли игра к началу */
        if (server->game->state == GAME_STATE_WAITING && game_is_ready(server->game)) {
            printf("Все игроки подключены, начинаем игру!\n");
            
            uint8_t buf[64];
            int len = encode_start_game(buf, server->game->winner_points);
            server_broadcast(server, buf, len);
            
            game_start(server->game);
            
            /* Отправляем информацию об арене */
            uint8_t arena_buf[MAX_PACKET_SIZE];
            len = encode_start_arena(arena_buf, server->game->arena, server->game);
            server_broadcast(server, arena_buf, len);
        }
    }
}

/* Обработка нового подключения */
void server_handle_connection(Server *server) {
    if (socket_has_data(&server->tcp_listener, 0)) {
        Socket client = socket_accept(&server->tcp_listener);
        if (socket_is_valid(&client)) {
            socket_set_nonblocking(&client);
            
            /* Пока не добавляем в сессии - ждём логин */
            /* Сохраняем временно в первый свободный слот */
            for (int i = 0; i < MAX_PLAYERS; i++) {
                if (!server->room.sessions[i].active && server->room.sessions[i].tcp_socket.fd < 0) {
                    server->room.sessions[i].tcp_socket = client;
                    break;
                }
            }
        }
    }
}

/* Обработка TCP буфера сессии - извлекает и обрабатывает полные пакеты */
static void process_session_tcp_buffer(Server *server, Session **session) {
    while ((*session)->tcp_buffer_len >= PACKET_HEADER_SIZE) {
        PacketHeader header;
        decode_packet_header((*session)->tcp_buffer, &header);
        
        size_t packet_size = PACKET_HEADER_SIZE + header.data_length;
        
        /* Проверяем, есть ли полный пакет в буфере */
        if ((*session)->tcp_buffer_len < packet_size) {
            /* Неполный пакет, ждём больше данных */
            break;
        }
        
        /* Сохраняем текущую длину буфера на случай, если она изменится в handle_single_packet */
        size_t buffer_len_before = (*session)->tcp_buffer_len;
        
        /* Обрабатываем полный пакет */
        handle_single_packet(server, session, (*session)->tcp_buffer, (int)packet_size);
        
        /* Проверяем, изменилась ли длина буфера (например, при LOGIN) */
        if ((*session)->tcp_buffer_len != buffer_len_before) {
            /* Буфер был изменен в handle_single_packet (например, при LOGIN),
             * не нужно вычитать packet_size, так как это уже сделано */
            continue;
        }
        
        /* Сдвигаем оставшиеся данные в начало буфера */
        if ((*session)->tcp_buffer_len > packet_size) {
            memmove((*session)->tcp_buffer, 
                    (*session)->tcp_buffer + packet_size,
                    (*session)->tcp_buffer_len - packet_size);
        }
        (*session)->tcp_buffer_len -= packet_size;
    }
}

/* Обработка одного полного пакета от клиента */
static void handle_single_packet(Server *server, Session **session, uint8_t *data, int len) {
    if (len < (int)PACKET_HEADER_SIZE) return;
    
    PacketHeader header;
    decode_packet_header(data, &header);
    uint8_t *payload = data + PACKET_HEADER_SIZE;
    
    uint8_t response[MAX_PACKET_SIZE];
    int resp_len = 0;
    
    switch (header.message_type) {
        case CLIENT_MSG_VERSION: {
            /* Отправляем ответ версии */
            resp_len = encode_version_response(response, PROTOCOL_VERSION, 1);
            socket_send_all(&(*session)->tcp_socket, response, (size_t)resp_len);
            break;
        }
        
        case CLIENT_MSG_SUBSCRIBE_INFO: {
            /* Отправляем статическую информацию */
            resp_len = encode_static_info(response, server->udp_port, 
                                         server->game->map_size,
                                         server->game->winner_points,
                                         server->game->max_players);
            socket_send_all(&(*session)->tcp_socket, response, (size_t)resp_len);
            
            /* Отправляем динамическую информацию */
            char symbols[MAX_PLAYERS];
            int count = room_session_get_symbols(&server->room, symbols);
            resp_len = encode_dynamic_info(response, symbols, count);
            socket_send_all(&(*session)->tcp_socket, response, (size_t)resp_len);
            break;
        }
        
        case CLIENT_MSG_LOGIN: {
            char symbol;
            decode_login(payload, &symbol);
            
            /* ВАЖНО: Сохраняем буфер ДО вызова room_session_add,
             * так как room_session_add может использовать тот же слот
             * и обнулить буфер.
             * Также сохраняем размер текущего пакета, чтобы вычесть его
             * из восстановленного буфера */
            size_t saved_buffer_len = (*session)->tcp_buffer_len;
            size_t current_packet_size = (size_t)len;  /* Размер обрабатываемого пакета */
            uint8_t saved_buffer[SESSION_TCP_BUFFER_SIZE];
            if (saved_buffer_len > 0) {
                memcpy(saved_buffer, (*session)->tcp_buffer, saved_buffer_len);
            }
            
            LoginStatus status = LOGIN_OK;
            int32_t token = 0;
            
            /* Проверяем символ */
            if (symbol < 'A' || symbol > 'Z') {
                status = LOGIN_INVALID_CHAR;
            } else if (room_session_find_by_symbol(&server->room, symbol)) {
                status = LOGIN_ALREADY_USED;
            } else if (room_session_is_full(&server->room)) {
                status = LOGIN_ROOM_FULL;
            } else {
                /* Добавляем сессию */
                token = room_session_add(&server->room, symbol, (*session)->tcp_socket);
                if (token < 0) {
                    status = LOGIN_ROOM_FULL;
                } else {
                    /* Добавляем игрока в игру */
                    game_add_player(server->game, symbol);
                    printf("Игрок %c подключился (токен: %d)\n", symbol, token);
                    
                    /* Получаем новую сессию и восстанавливаем буфер */
                    Session *new_session = room_session_find_by_token(&server->room, token);
                    if (new_session) {
                        /* Вычисляем размер оставшихся данных после текущего пакета */
                        size_t remaining_len = 0;
                        if (saved_buffer_len > current_packet_size) {
                            remaining_len = saved_buffer_len - current_packet_size;
                            /* Копируем оставшиеся данные (после текущего пакета) */
                            memcpy(new_session->tcp_buffer, 
                                   saved_buffer + current_packet_size, 
                                   remaining_len);
                        }
                        new_session->tcp_buffer_len = remaining_len;
                        
                        /* Очищаем старую сессию, если это другой слот */
                        if (new_session != *session) {
                            (*session)->tcp_buffer_len = 0;
                            (*session)->tcp_socket.fd = -1;  /* Сокет теперь в new_session */
                        }
                        
                        /* Обновляем указатель на сессию */
                        *session = new_session;
                    }
                }
            }
            
            resp_len = encode_login_status(response, symbol, status, token);
            socket_send_all(&(*session)->tcp_socket, response, (size_t)resp_len);
            
            if (status == LOGIN_OK) {
                /* Рассылаем обновлённый список игроков */
                char symbols[MAX_PLAYERS];
                int count = room_session_get_symbols(&server->room, symbols);
                resp_len = encode_dynamic_info(response, symbols, count);
                server_broadcast(server, response, resp_len);
            }
            break;
        }
        
        case CLIENT_MSG_LOGOUT: {
            if (!(*session)->active) break;
            printf("Игрок %c вышел\n", (*session)->symbol);
            game_remove_player(server->game, game_get_player_index(server->game, (*session)->symbol));
            room_session_remove(&server->room, (*session)->token);
            
            /* Рассылаем обновлённый список */
            char symbols[MAX_PLAYERS];
            int count = room_session_get_symbols(&server->room, symbols);
            resp_len = encode_dynamic_info(response, symbols, count);
            server_broadcast(server, response, resp_len);
            break;
        }
        
        case CLIENT_MSG_MOVE_PLAYER: {
            if (!(*session)->active) break;
            if (server->game->state != GAME_STATE_PLAYING) break;
            
            Direction dir;
            decode_move_player(payload, &dir);
            
            /* Находим сущность игрока и перемещаем */
            Player *player = game_get_player_by_symbol(server->game, (*session)->symbol);
            if (player && player->entity_id >= 0 && server->game->arena) {
                arena_move_entity(server->game->arena, player->entity_id, dir);
            }
            break;
        }
        
        case CLIENT_MSG_CAST_SKILL: {
            if (!(*session)->active) break;
            if (server->game->state != GAME_STATE_PLAYING) break;
            
            Direction dir;
            uint8_t spell_type_raw;
            decode_cast_skill(payload, &dir, &spell_type_raw);
            
            /* Преобразуем в SpellType */
            SpellType spell_type = (spell_type_raw == 2) ? SPELL_TYPE_POWER : SPELL_TYPE_BASIC;
            
            /* Находим сущность игрока и применяем способность */
            Player *player = game_get_player_by_symbol(server->game, (*session)->symbol);
            if (player && player->entity_id >= 0 && server->game->arena) {
                /* Обновляем выбранный тип заклинания у сущности */
                Entity *entity = arena_get_entity(server->game->arena, player->entity_id);
                if (entity) {
                    entity->spell_type = spell_type;
                }
                arena_cast_spell(server->game->arena, player->entity_id, dir, spell_type);
            }
            break;
        }
        
        case CLIENT_MSG_TRUST_UDP: {
            /* Клиент подтвердил получение UDP_CONNECTED */
            /* Ничего делать не нужно */
            break;
        }
        
        default:
            break;
    }
}

/* Обработка сообщения от клиента (для совместимости, перенаправляет в handle_single_packet) */
void server_handle_message(Server *server, Session *session, uint8_t *data, int len) {
    Session *session_ptr = session;
    handle_single_packet(server, &session_ptr, data, len);
}

/* Обработка UDP сообщения */
void server_handle_udp(Server *server, uint8_t *data, int len, struct sockaddr_in *src) {
    if (len < (int)PACKET_HEADER_SIZE) return;
    
    PacketHeader header;
    decode_packet_header(data, &header);
    uint8_t *payload = data + PACKET_HEADER_SIZE;
    
    if (header.message_type == CLIENT_MSG_CONNECT_UDP) {
        int32_t token;
        decode_connect_udp(payload, &token);
        
        Session *session = room_session_find_by_token(&server->room, token);
        if (session) {
            session->udp_addr = *src;
            session->udp_connected = 1;
            
            /* Отправляем подтверждение */
            uint8_t response[64];
            int resp_len = encode_udp_connected(response);
            socket_send_all(&session->tcp_socket, response, (size_t)resp_len);
        }
    }
}

/* Рассылка GameStep всем клиентам */
void server_broadcast_game_step(Server *server) {
    if (!server->game->arena) return;
    
    uint8_t buffer[MAX_PACKET_SIZE];
    int len = encode_game_step(buffer, server->game->arena, server->game);
    
    /* Отправляем через UDP тем, кто подключён */
    for (int i = 0; i < MAX_PLAYERS; i++) {
        Session *s = &server->room.sessions[i];
        if (s->active && s->udp_connected) {
            socket_sendto(&server->udp_socket, buffer, (size_t)len, &s->udp_addr);
        } else if (s->active) {
            /* Fallback на TCP */
            socket_send_all(&s->tcp_socket, buffer, (size_t)len);
        }
    }
}

/* Рассылка сообщения всем клиентам */
void server_broadcast(Server *server, uint8_t *data, int len) {
    for (int i = 0; i < MAX_PLAYERS; i++) {
        Session *s = &server->room.sessions[i];
        if (s->active) {
            socket_send_all(&s->tcp_socket, data, (size_t)len);
        }
    }
}

/* Остановка сервера */
void server_stop(Server *server) {
    server->running = 0;
}

/* Освобождение ресурсов */
void server_destroy(Server *server) {
    room_session_destroy(&server->room);
    socket_close(&server->tcp_listener);
    socket_close(&server->udp_socket);
    if (server->game) {
        game_destroy(server->game);
    }
    free(server);
}

