/*
 * state.c - Реализация состояния клиента
 */

#include "state.h"
#include <string.h>

/* Создание состояния клиента */
ClientState client_state_create(void) {
    ClientState state;
    memset(&state, 0, sizeof(state));
    
    state.state = CLIENT_STATE_MENU;
    strcpy(state.host, "localhost");
    state.tcp_port = 3042;
    state.udp_port = 3043;
    state.player_symbol = 'A';
    state.session_token = 0;
    
    state.tcp_socket.fd = -1;
    state.udp_socket.fd = -1;
    state.udp_connected = 0;
    
    /* Инициализация TCP буфера */
    state.tcp_buffer_len = 0;
    
    state.map_size = 20;
    state.winner_points = 5;
    state.max_players = 4;
    
    state.player_count = 0;
    state.wait_seconds = 0;
    
    state.entity_count = 0;
    state.spell_count = 0;
    state.player_data_count = 0;
    
    state.winner = '\0';
    state.selected_spell_type = 1;  /* По умолчанию базовая атака */
    state.last_direction = 2;       /* По умолчанию вниз (DIR_DOWN) */
    
    return state;
}

/* Установка состояния */
void client_state_set(ClientState *state, ClientStateType new_state) {
    state->state = new_state;
}

/* Сброс состояния */
void client_state_reset(ClientState *state) {
    /* Закрываем сокеты */
    if (state->tcp_socket.fd >= 0) {
        socket_close(&state->tcp_socket);
    }
    if (state->udp_socket.fd >= 0) {
        socket_close(&state->udp_socket);
    }
    
    /* Сбрасываем данные */
    state->udp_connected = 0;
    state->session_token = 0;
    state->player_count = 0;
    state->entity_count = 0;
    state->spell_count = 0;
    state->player_data_count = 0;
    state->winner = '\0';
    
    /* Очищаем TCP буфер */
    state->tcp_buffer_len = 0;
    
    state->state = CLIENT_STATE_MENU;
}

/* Освобождение ресурсов */
void client_state_destroy(ClientState *state) {
    if (state->tcp_socket.fd >= 0) {
        socket_close(&state->tcp_socket);
    }
    if (state->udp_socket.fd >= 0) {
        socket_close(&state->udp_socket);
    }
}

