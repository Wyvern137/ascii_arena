/*
 * server_main.c - Точка входа сервера
 * Парсинг аргументов и запуск сервера
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <getopt.h>
#include "server.h"

/* Глобальный указатель для обработки сигналов */
static Server *g_server = NULL;

/* Обработчик сигнала прерывания. Никогда не используйте глобальную переменную в обработчике сигналов
 * (гонка данных: handler может сработать до инициализации или во время её изменения в main).
 * Обработчик вызывается асинхронно в любой момент и может прервать неатомарную операцию — внутри
 * допустимы только async-signal-safe функции.*/
static void signal_handler(int sig) {
    (void)sig;
    if (g_server) {
        printf("\nОстановка сервера...\n");
        server_stop(g_server);
    }
}

/* Вывод справки */
static void print_usage(const char *prog_name) {
    printf("Использование: %s [опции]\n", prog_name);
    printf("Опции:\n");
    printf("  -p, --players NUM   Количество игроков (по умолчанию: 2)\n");
    printf("  -t, --tcp PORT      TCP порт (по умолчанию: 3042)\n");
    printf("  -u, --udp PORT      UDP порт (по умолчанию: 3043)\n");
    printf("  -m, --map SIZE      Размер карты (по умолчанию: 20)\n");
    printf("  -w, --winner POINTS Очки для победы (по умолчанию: 5)\n");
    printf("  --help              Показать эту справку\n");
}

int main(int argc, char *argv[]) {
    int max_players = 2;
    int tcp_port = 3042;
    int udp_port = 3043;
    int map_size = 20;
    int winner_points = 5;
    
    /* Опции командной строки */
    static struct option long_options[] = {
        {"players", required_argument, 0, 'p'},
        {"tcp", required_argument, 0, 't'},
        {"udp", required_argument, 0, 'u'},
        {"map", required_argument, 0, 'm'},
        {"winner", required_argument, 0, 'w'},
        {"help", no_argument, 0, 0},
        {0, 0, 0, 0}
    };
    
    int opt;
    int option_index = 0;
    
    while ((opt = getopt_long(argc, argv, "p:t:u:m:w:", long_options, &option_index)) != -1) {
        switch (opt) {
            case 'p':
                max_players = atoi(optarg);
                if (max_players < 2) max_players = 2;
                if (max_players > 8) max_players = 8;
                break;
            case 't':
                tcp_port = atoi(optarg);
                break;
            case 'u':
                udp_port = atoi(optarg);
                break;
            case 'm':
                map_size = atoi(optarg);
                if (map_size < 10) map_size = 10;
                if (map_size > 50) map_size = 50;
                break;
            case 'w':
                winner_points = atoi(optarg);
                if (winner_points < 1) winner_points = 1;
                break;
            case 0:
                if (strcmp(long_options[option_index].name, "help") == 0) {
                    print_usage(argv[0]);
                    return 0;
                }
                break;
            default:
                print_usage(argv[0]);
                return 1;
        }
    }
    
    /* Устанавливаем обработчик сигналов */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    /* Создаём и запускаем сервер */
    g_server = server_create(tcp_port, udp_port, max_players, map_size, winner_points);
    if (!g_server) {
        fprintf(stderr, "Ошибка создания сервера\n");
        return 1;
    }
    
    server_run(g_server);
    server_destroy(g_server);
    
    printf("Сервер остановлен\n");
    return 0;
}

