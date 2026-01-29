/*
 * menu.c - Реализация меню игры
 */

#include "menu.h"
#include "widgets.h"
#include "terminal.h"
#include <ncurses.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>

/* ASCII-арт логотип */
static const char *MAIN_TITLE =
    "   _____                .__.__   _____                                \n"
    "  /  _  \\   ______ ____ |__|__| /  _  \\_______   ____   ____ _____    \n"
    " /  /_\\  \\ /  ___// ___\\|  |  |/  /_\\  \\_  __ \\_/ __ \\ /    \\\\__  \\   \n"
    "/    |    \\\\___ \\\\  \\___|  |  /    |    \\  | \\/\\  ___/|   |  \\/ __ \\_ \n"
    "\\____|__  /______>\\_____>__|__\\____|__  /__|    \\_____>___|__(______/ \n"
    "        \\/                            \\/";

/* Версия клиента */
static const char *CLIENT_VERSION = "1.0.0";

/* Константы layout */
#define TITLE_WIDTH 70
#define TITLE_HEIGHT 6
#define VERSION_X_OFFSET 54
#define CLIENT_INFO_HEIGHT 2
#define SERVER_INFO_HEIGHT 7
#define WAITING_ROOM_PANEL_WIDTH 20
#define NOTIFICATION_HEIGHT 2

/* Получение текущего времени в миллисекундах */
static int64_t get_time_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (int64_t)tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

/* Инициализация waiting room */
static void waiting_room_init(WaitingRoom *wr, int width, int height) {
    wr->width = width / 2;  /* Двойная ширина символов */
    wr->height = height;
    wr->player_count = 0;
    memset(wr->players, 0, sizeof(wr->players));
}

/* Обновление waiting room */
static void waiting_room_update(WaitingRoom *wr, const char *players, int count) {
    int64_t now = get_time_ms();
    const int64_t MOVE_INTERVAL = 500;  /* 500мс между движениями */
    
    /* Удаляем игроков, которых больше нет */
    for (int i = 0; i < wr->player_count; ) {
        int found = 0;
        for (int j = 0; j < count; j++) {
            if (wr->players[i].symbol == players[j]) {
                found = 1;
                break;
            }
        }
        if (!found) {
            /* Удаляем игрока */
            for (int k = i; k < wr->player_count - 1; k++) {
                wr->players[k] = wr->players[k + 1];
            }
            wr->player_count--;
        } else {
            i++;
        }
    }
    
    /* Добавляем новых игроков */
    for (int i = 0; i < count && wr->player_count < MAX_WAITING_ROOM_PLAYERS; i++) {
        int found = 0;
        for (int j = 0; j < wr->player_count; j++) {
            if (wr->players[j].symbol == players[i]) {
                found = 1;
                break;
            }
        }
        if (!found) {
            WaitingRoomPlayer *p = &wr->players[wr->player_count++];
            p->symbol = players[i];
            p->pos_x = rand() % wr->width;
            p->pos_y = rand() % wr->height;
            p->direction = rand() % 4;
            p->last_move = now - MOVE_INTERVAL;
        }
    }
    
    /* Двигаем игроков */
    for (int i = 0; i < wr->player_count; i++) {
        WaitingRoomPlayer *p = &wr->players[i];
        
        if (now - p->last_move >= MOVE_INTERVAL) {
            p->last_move = now;
            
            /* 85% шанс движения */
            if ((rand() % 100) < 85) {
                /* 33% шанс поворота */
                if ((rand() % 100) < 33) {
                    if (rand() % 2) {
                        p->direction = (p->direction + 1) % 4;  /* Поворот направо */
                    } else {
                        p->direction = (p->direction + 3) % 4;  /* Поворот налево */
                    }
                }
                
                /* Движение */
                int new_x = p->pos_x;
                int new_y = p->pos_y;
                
                switch (p->direction) {
                    case 0: new_y--; break;  /* Up */
                    case 1: new_x++; break;  /* Right */
                    case 2: new_y++; break;  /* Down */
                    case 3: new_x--; break;  /* Left */
                }
                
                /* Проверка границ */
                if (new_x >= 0 && new_x < wr->width && new_y >= 0 && new_y < wr->height) {
                    /* Проверка коллизий с другими игроками */
                    int collision = 0;
                    for (int j = 0; j < wr->player_count; j++) {
                        if (i != j && wr->players[j].pos_x == new_x && wr->players[j].pos_y == new_y) {
                            collision = 1;
                            break;
                        }
                    }
                    
                    if (!collision) {
                        p->pos_x = new_x;
                        p->pos_y = new_y;
                    }
                } else {
                    /* Поворот при столкновении со стеной */
                    p->direction = (p->direction + 1) % 4;
                }
            }
        }
    }
}

/* Отрисовка waiting room */
static void waiting_room_render(WaitingRoom *wr, int x, int y) {
    for (int i = 0; i < wr->player_count; i++) {
        WaitingRoomPlayer *p = &wr->players[i];
        int draw_x = x + p->pos_x * 2;  /* Двойная ширина */
        int draw_y = y + p->pos_y;
        
        widget_draw_char_styled(draw_x, draw_y, p->symbol, COLOR_PLAYER_WHITE, 1);
    }
}

/* Создание меню */
Menu menu_create(const char *default_addr, char default_char) {
    Menu menu;
    memset(&menu, 0, sizeof(menu));
    
    /* Инициализация поля адреса */
    if (default_addr) {
        strncpy(menu.server_addr_input.content, default_addr, MAX_SERVER_ADDR_LEN - 1);
        menu.server_addr_input.cursor_pos = (int)strlen(menu.server_addr_input.content);
    }
    menu.server_addr_input.has_focus = 1;  /* Фокус по умолчанию на адресе */
    
    /* Инициализация поля символа */
    menu.character_input.content = default_char;
    menu.character_input.has_focus = 0;
    
    /* Инициализация waiting room */
    waiting_room_init(&menu.waiting_room, WAITING_ROOM_WIDTH, WAITING_ROOM_HEIGHT);
    
    /* Начальное состояние */
    menu.connection_status = CONNECTION_NOT_CONNECTED;
    menu.login_status = LOGIN_NOT_LOGGED;
    menu.has_version_info = 0;
    menu.server_info.has_info = 0;
    menu.server_info.udp_confirmed = -1;
    menu.logged_players_count = 0;
    menu.start_countdown = -1;
    
    /* Инициализация генератора случайных чисел */
    srand((unsigned int)time(NULL));
    
    return menu;
}

/* Обновление меню */
void menu_update(Menu *menu) {
    /* Обновляем waiting room */
    waiting_room_update(&menu->waiting_room, menu->logged_players, menu->logged_players_count);
    
    /* Обновляем фокус в зависимости от состояния:
     * 1. Если не подключен или версия несовместима -> фокус на адресе сервера
     * 2. Если подключен и получена информация о сервере, но не залогинен -> фокус на имени персонажа
     * 3. Если залогинен -> нет фокуса (ждём начала игры)
     */
    if (menu->connection_status != CONNECTION_CONNECTED || !menu_has_compatible_version(menu)) {
        /* Не подключен или версия несовместима - редактируем адрес сервера */
        menu->server_addr_input.has_focus = 1;
        menu->character_input.has_focus = 0;
    } else if (menu->login_status != LOGIN_LOGGED) {
        /* Подключен, но не залогинен - редактируем имя персонажа.
         * Информация о сервере и список игроков отображаются,
         * пользователь может выбрать свободное имя. */
        menu->server_addr_input.has_focus = 0;
        menu->character_input.has_focus = 1;
    } else {
        /* Залогинен - ждём начала игры */
        menu->server_addr_input.has_focus = 0;
        menu->character_input.has_focus = 0;
    }
}

/* Обработка ввода клавиши */
int menu_handle_key(Menu *menu, int key) {
    /* Обработка для поля адреса */
    if (menu->server_addr_input.has_focus) {
        int len = (int)strlen(menu->server_addr_input.content);
        
        if (key >= 32 && key <= 126 && len < MAX_SERVER_ADDR_LEN - 1) {
            /* Вставка символа */
            for (int i = len; i >= menu->server_addr_input.cursor_pos; i--) {
                menu->server_addr_input.content[i + 1] = menu->server_addr_input.content[i];
            }
            menu->server_addr_input.content[menu->server_addr_input.cursor_pos++] = (char)key;
            return 0;
        }
        
        switch (key) {
            case KEY_BACKSPACE:
            case 127:
            case 8:
                if (menu->server_addr_input.cursor_pos > 0) {
                    for (int i = menu->server_addr_input.cursor_pos - 1; i < len; i++) {
                        menu->server_addr_input.content[i] = menu->server_addr_input.content[i + 1];
                    }
                    menu->server_addr_input.cursor_pos--;
                }
                return 0;
                
            case KEY_DC:  /* Delete */
                if (menu->server_addr_input.cursor_pos < len) {
                    for (int i = menu->server_addr_input.cursor_pos; i < len; i++) {
                        menu->server_addr_input.content[i] = menu->server_addr_input.content[i + 1];
                    }
                }
                return 0;
                
            case KEY_LEFT:
                if (menu->server_addr_input.cursor_pos > 0) {
                    menu->server_addr_input.cursor_pos--;
                }
                return 0;
                
            case KEY_RIGHT:
                if (menu->server_addr_input.cursor_pos < len) {
                    menu->server_addr_input.cursor_pos++;
                }
                return 0;
                
            case KEY_HOME:
                menu->server_addr_input.cursor_pos = 0;
                return 0;
                
            case KEY_END:
                menu->server_addr_input.cursor_pos = len;
                return 0;
        }
    }
    
    /* Обработка для поля символа */
    if (menu->character_input.has_focus) {
        if ((key >= 'A' && key <= 'Z') || (key >= 'a' && key <= 'z')) {
            menu->character_input.content = (char)((key >= 'a') ? (key - 'a' + 'A') : key);
            return 0;
        }
        
        if (key == KEY_BACKSPACE || key == 127 || key == 8 || key == KEY_DC) {
            menu->character_input.content = '\0';
            return 0;
        }
    }
    
    return key;  /* Возвращаем необработанную клавишу */
}

/* Отрисовка меню */
void menu_render(Menu *menu, int screen_width, int screen_height) {
    /* Вычисляем размеры и позицию */
    int total_height = TITLE_HEIGHT + 1 + 2 + CLIENT_INFO_HEIGHT + 2 + SERVER_INFO_HEIGHT + 1 + NOTIFICATION_HEIGHT;
    int start_x = (screen_width - TITLE_WIDTH) / 2;
    int start_y = (screen_height - total_height) / 2;
    
    if (start_x < 0) start_x = 0;
    if (start_y < 0) start_y = 0;
    
    int y = start_y;
    
    /* 1. ASCII-арт логотип с чередующимися цветами #92A8B3 и #7F977B */
    widget_draw_ascii_art_dual_color(start_x, y, MAIN_TITLE, COLOR_TITLE_BLUE, COLOR_TITLE_GREEN, 1);
    y += TITLE_HEIGHT;
    
    /* 2. Версия (справа от логотипа) */
    char version_str[64];
    snprintf(version_str, sizeof(version_str), "version: %s", CLIENT_VERSION);
    widget_draw_text_colored(start_x + VERSION_X_OFFSET, y, version_str, COLOR_STATUS_GRAY, 0);
    y += 1;
    
    /* 3. Отступ */
    y += 2;
    
    /* 4. Client Info Panel (2 строки) */
    int info_x = start_x + 4;
    
    /* Server address */
    widget_draw_text(info_x, y, "Server address:  ");
    widget_draw_text_colored(info_x + 17, y, menu->server_addr_input.content, COLOR_PLAYER_WHITE, 1);
    
    /* Статус подключения справа */
    const char *conn_status_text;
    int conn_status_color;
    
    if (strlen(menu->server_addr_input.content) == 0) {
        conn_status_text = "Not connected";
        conn_status_color = COLOR_STATUS_INACTIVE;
    } else {
        switch (menu->connection_status) {
            case CONNECTION_CONNECTED:
                conn_status_text = "Connected";
                conn_status_color = COLOR_STATUS_CONNECTED;
                break;
            case CONNECTION_NOT_CONNECTED:
                conn_status_text = "Not connected";
                conn_status_color = COLOR_STATUS_INACTIVE;
                break;
            case CONNECTION_NOT_FOUND:
                conn_status_text = "Server not found";
                conn_status_color = COLOR_STATUS_ERROR;
                break;
            case CONNECTION_LOST:
                if (!menu_has_compatible_version(menu)) {
                    conn_status_text = "Version error";
                } else {
                    conn_status_text = "Connection lost";
                }
                conn_status_color = COLOR_STATUS_ERROR;
                break;
            case CONNECTION_CONNECTING:
                conn_status_text = "Connecting...";
                conn_status_color = COLOR_STATUS_WARNING;
                break;
            default:
                conn_status_text = "Not connected";
                conn_status_color = COLOR_STATUS_INACTIVE;
        }
    }
    widget_draw_text_right_colored(start_x, y, TITLE_WIDTH, conn_status_text, conn_status_color, 0);
    
    /* Курсор для поля адреса */
    if (menu->server_addr_input.has_focus) {
        terminal_set_cursor(1);
        move(y, info_x + 17 + menu->server_addr_input.cursor_pos);
    }
    
    y++;
    
    /* Character name */
    widget_draw_text(info_x, y, "Character name:  ");
    char char_str[2] = {menu->character_input.content ? menu->character_input.content : ' ', '\0'};
    widget_draw_text_colored(info_x + 17, y, char_str, COLOR_PLAYER_WHITE, 1);
    
    /* Статус логина справа */
    const char *login_status_text;
    int login_status_color;
    
    if (menu->login_status == LOGIN_LOGGED) {
        login_status_text = "Logged";
        login_status_color = COLOR_STATUS_CONNECTED;
    } else if (menu->login_status == LOGIN_NAME_TAKEN) {
        login_status_text = "Name already chosen";
        login_status_color = COLOR_STATUS_ERROR;
    } else if (menu->login_status == LOGIN_PLAYER_LIMIT) {
        login_status_text = "Player limit reached";
        login_status_color = COLOR_STATUS_WARNING;
    } else if (menu->login_status == LOGIN_INVALID_NAME) {
        login_status_text = "Invalid player name";
        login_status_color = COLOR_STATUS_ERROR;
    } else {
        login_status_text = "Not logged";
        login_status_color = COLOR_STATUS_INACTIVE;
    }
    widget_draw_text_right_colored(start_x, y, TITLE_WIDTH, login_status_text, login_status_color, 0);
    
    /* Курсор для поля символа */
    if (menu->character_input.has_focus) {
        terminal_set_cursor(1);
        move(y, info_x + 17);
    }
    
    y++;
    
    /* 5. Отступ */
    y += 2;
    
    /* 6. Server Info Panel и Waiting Room Panel (горизонтально) */
    int server_info_width = TITLE_WIDTH - 2 - WAITING_ROOM_PANEL_WIDTH - 2;
    int server_info_x = start_x + 2;
    int waiting_room_x = start_x + server_info_width + 4;
    
    /* Server Info Panel */
    widget_draw_rounded_box_titled(server_info_x, y, server_info_width, SERVER_INFO_HEIGHT, "Server info", 0);
    
    int info_inner_x = server_info_x + 2;
    int info_inner_y = y + 1;
    
    if (menu->server_info.has_info) {
        /* Version */
        widget_draw_text(info_inner_x, info_inner_y, "Version:  ");
        widget_draw_text_colored(info_inner_x + 10, info_inner_y, menu->version_info.version, COLOR_PLAYER_WHITE, 1);
        
        const char *compat_text;
        int compat_color;
        if (menu->version_info.compatibility == 2) {
            compat_text = "Compatible";
            compat_color = COLOR_STATUS_CONNECTED;
        } else if (menu->version_info.compatibility == 1) {
            compat_text = "Compatible";
            compat_color = COLOR_STATUS_WARNING;
        } else {
            compat_text = "Incompatible";
            compat_color = COLOR_STATUS_ERROR;
        }
        widget_draw_text_right_colored(info_inner_x, info_inner_y, server_info_width - 4, compat_text, compat_color, 0);
        info_inner_y++;
        
        /* UDP port */
        char buf[32];
        snprintf(buf, sizeof(buf), "%d", menu->server_info.udp_port);
        widget_draw_text(info_inner_x, info_inner_y, "UDP port: ");
        widget_draw_text_colored(info_inner_x + 10, info_inner_y, buf, COLOR_PLAYER_WHITE, 1);
        
        if (menu->login_status == LOGIN_LOGGED) {
            const char *udp_status;
            int udp_color;
            if (menu->server_info.udp_confirmed == 1) {
                udp_status = "Checked";
                udp_color = COLOR_STATUS_CONNECTED;
            } else if (menu->server_info.udp_confirmed == 0) {
                udp_status = "Not available";
                udp_color = COLOR_STATUS_WARNING;
            } else {
                udp_status = "Checking...";
                udp_color = COLOR_STATUS_WARNING;
            }
            widget_draw_text_right_colored(info_inner_x, info_inner_y, server_info_width - 4, udp_status, udp_color, 0);
        }
        info_inner_y++;
        
        /* Map size */
        snprintf(buf, sizeof(buf), "%dx%d", menu->server_info.map_size, menu->server_info.map_size);
        widget_draw_text(info_inner_x, info_inner_y, "Map size: ");
        widget_draw_text_colored(info_inner_x + 10, info_inner_y, buf, COLOR_PLAYER_WHITE, 1);
        info_inner_y++;
        
        /* Points */
        snprintf(buf, sizeof(buf), "%d", menu->server_info.winner_points);
        widget_draw_text(info_inner_x, info_inner_y, "Points:   ");
        widget_draw_text_colored(info_inner_x + 10, info_inner_y, buf, COLOR_PLAYER_WHITE, 1);
        info_inner_y++;
        
        /* Players */
        snprintf(buf, sizeof(buf), "%d/%d", menu->server_info.current_players, menu->server_info.max_players);
        widget_draw_text(info_inner_x, info_inner_y, "Players:  ");
        widget_draw_text_colored(info_inner_x + 10, info_inner_y, buf, COLOR_PLAYER_WHITE, 1);
        
        const char *players_status;
        int players_color;
        if (menu->server_info.current_players == menu->server_info.max_players) {
            if (menu->login_status == LOGIN_LOGGED) {
                players_status = "Ready";
                players_color = COLOR_STATUS_CONNECTED;
            } else {
                players_status = "Completed";
                players_color = COLOR_STATUS_ERROR;
            }
        } else {
            players_status = "Waiting other players...";
            players_color = COLOR_STATUS_WARNING;
        }
        widget_draw_text_right_colored(info_inner_x, info_inner_y, server_info_width - 4, players_status, players_color, 0);
    } else {
        /* Нет информации */
        const char *msg;
        int msg_color;
        
        if (menu->has_version_info && menu->version_info.compatibility == 0) {
            msg = "Incompatible versions";
            msg_color = COLOR_STATUS_ERROR;
        } else if (menu->connection_status == CONNECTION_CONNECTED) {
            msg = "Loading information...";
            msg_color = COLOR_STATUS_GRAY;
        } else {
            msg = "Without information";
            msg_color = COLOR_STATUS_GRAY;
        }
        
        widget_draw_text_centered_colored(info_inner_x, info_inner_y + 2, server_info_width - 4, msg, msg_color, 0);
    }
    
    /* Waiting Room Panel */
    const char *wr_title = (menu_is_server_full(menu) && menu->login_status != LOGIN_LOGGED) 
                           ? "Players at game" : "Waiting room";
    widget_draw_rounded_box_titled(waiting_room_x, y, WAITING_ROOM_PANEL_WIDTH, SERVER_INFO_HEIGHT, wr_title, 0);
    
    /* Отрисовка игроков в waiting room */
    waiting_room_render(&menu->waiting_room, waiting_room_x + 1, y + 1);
    
    y += SERVER_INFO_HEIGHT;
    
    /* 7. Отступ */
    y += 1;
    
    /* 8. Notification Label (2 строки) */
    int notif_width = TITLE_WIDTH;
    
    if (menu->connection_status != CONNECTION_CONNECTED || !menu_has_compatible_version(menu)) {
        /* Не подключен - "Press <Enter> to connect to server" */
        const char *msg1 = "Press <Enter> to connect to server";
        int msg1_len = (int)strlen(msg1);
        int msg1_x = start_x + (notif_width - msg1_len) / 2;
        
        mvprintw(y, msg1_x, "Press ");
        attron(COLOR_PAIR(COLOR_TEXT_CYAN) | A_BOLD);
        mvprintw(y, msg1_x + 6, "<Enter>");
        attroff(COLOR_PAIR(COLOR_TEXT_CYAN) | A_BOLD);
        mvprintw(y, msg1_x + 13, " to connect to server");
        
        y++;
        
        /* "Press <Esc> to exit from asciiarena" */
        const char *msg2 = "Press <Esc> to exit from asciiarena";
        int msg2_len = (int)strlen(msg2);
        int msg2_x = start_x + (notif_width - msg2_len) / 2;
        
        mvprintw(y, msg2_x, "Press ");
        attron(COLOR_PAIR(COLOR_TEXT_YELLOW) | A_BOLD);
        mvprintw(y, msg2_x + 6, "<Esc>");
        attroff(COLOR_PAIR(COLOR_TEXT_YELLOW) | A_BOLD);
        mvprintw(y, msg2_x + 11, " to exit from asciiarena");
        
    } else if (menu->login_status != LOGIN_LOGGED) {
        /* Подключен, но не залогинен */
        if (menu->character_input.content == '\0') {
            widget_draw_text_centered(start_x, y, notif_width, "Choose a character (an ascii uppercase letter)");
        } else {
            const char *msg1 = "Press <Enter> to login with the character";
            int msg1_len = (int)strlen(msg1);
            int msg1_x = start_x + (notif_width - msg1_len) / 2;
            
            mvprintw(y, msg1_x, "Press ");
            attron(COLOR_PAIR(COLOR_TEXT_CYAN) | A_BOLD);
            mvprintw(y, msg1_x + 6, "<Enter>");
            attroff(COLOR_PAIR(COLOR_TEXT_CYAN) | A_BOLD);
            mvprintw(y, msg1_x + 13, " to login with the character");
        }
        
        y++;
        
        const char *msg2 = "Press <Esc> to disconnect from the server";
        int msg2_len = (int)strlen(msg2);
        int msg2_x = start_x + (notif_width - msg2_len) / 2;
        
        mvprintw(y, msg2_x, "Press ");
        attron(COLOR_PAIR(COLOR_TEXT_YELLOW) | A_BOLD);
        mvprintw(y, msg2_x + 6, "<Esc>");
        attroff(COLOR_PAIR(COLOR_TEXT_YELLOW) | A_BOLD);
        mvprintw(y, msg2_x + 11, " to disconnect from the server");
        
    } else if (menu->start_countdown > 0) {
        /* Обратный отсчёт */
        char countdown_msg[64];
        snprintf(countdown_msg, sizeof(countdown_msg), "Starting game in %d...", menu->start_countdown);
        widget_draw_text_centered_colored(start_x, y, notif_width, countdown_msg, COLOR_TEXT_LIGHTCYAN, 0);
    } else {
        /* Залогинен, ждём */
        const char *msg = "Press <Esc> to logout the character";
        int msg_len = (int)strlen(msg);
        int msg_x = start_x + (notif_width - msg_len) / 2;
        
        mvprintw(y, msg_x, "Press ");
        attron(COLOR_PAIR(COLOR_TEXT_YELLOW) | A_BOLD);
        mvprintw(y, msg_x + 6, "<Esc>");
        attroff(COLOR_PAIR(COLOR_TEXT_YELLOW) | A_BOLD);
        mvprintw(y, msg_x + 11, " to logout the character");
    }
    
    /* Скрываем курсор если нет фокуса */
    if (!menu->server_addr_input.has_focus && !menu->character_input.has_focus) {
        terminal_set_cursor(0);
    }
}

/* Установка состояния подключения */
void menu_set_connection_status(Menu *menu, ConnectionStatus status) {
    menu->connection_status = status;
    
    if (status != CONNECTION_CONNECTED) {
        menu->server_info.has_info = 0;
        menu->has_version_info = 0;
        menu->login_status = LOGIN_NOT_LOGGED;
        menu->logged_players_count = 0;
    }
}

/* Установка статуса логина */
void menu_set_login_status(Menu *menu, LoginStatusType status) {
    menu->login_status = status;
}

/* Установка информации о версии */
void menu_set_version_info(Menu *menu, const char *version, int compatibility) {
    strncpy(menu->version_info.version, version, sizeof(menu->version_info.version) - 1);
    menu->version_info.compatibility = compatibility;
    menu->has_version_info = 1;
}

/* Установка информации о сервере */
void menu_set_server_info(Menu *menu, int udp_port, int map_size, int winner_points, int max_players) {
    menu->server_info.has_info = 1;
    menu->server_info.udp_port = udp_port;
    menu->server_info.map_size = map_size;
    menu->server_info.winner_points = winner_points;
    menu->server_info.max_players = max_players;
}

/* Установка списка игроков */
void menu_set_players(Menu *menu, const char *players, int count) {
    menu->logged_players_count = count;
    if (count > MAX_WAITING_ROOM_PLAYERS) count = MAX_WAITING_ROOM_PLAYERS;
    memcpy(menu->logged_players, players, count);
    menu->server_info.current_players = count;
}

/* Установка таймера */
void menu_set_countdown(Menu *menu, int seconds) {
    menu->start_countdown = seconds;
}

/* Установка подтверждения UDP */
void menu_set_udp_confirmed(Menu *menu, int confirmed) {
    menu->server_info.udp_confirmed = confirmed;
}

/* Получение адреса сервера */
const char* menu_get_server_addr(Menu *menu) {
    return menu->server_addr_input.content;
}

/* Получение символа игрока */
char menu_get_character(Menu *menu) {
    return menu->character_input.content;
}

/* Проверка, залогинен ли пользователь */
int menu_is_logged(Menu *menu) {
    return menu->login_status == LOGIN_LOGGED;
}

/* Проверка, подключен ли к серверу */
int menu_is_connected(Menu *menu) {
    return menu->connection_status == CONNECTION_CONNECTED;
}

/* Проверка, совместима ли версия */
int menu_has_compatible_version(Menu *menu) {
    if (!menu->has_version_info) return 1;  /* Предполагаем совместимость */
    return menu->version_info.compatibility > 0;
}

/* Проверка, заполнен ли сервер */
int menu_is_server_full(Menu *menu) {
    if (!menu->server_info.has_info) return 0;
    return menu->server_info.current_players >= menu->server_info.max_players;
}

