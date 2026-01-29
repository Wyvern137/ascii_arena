/*
 * client.c - Точка входа клиента
 * Парсинг аргументов и запуск приложения
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include "app.h"

/* Вывод справки */
static void print_usage(const char *prog_name) {
    printf("Использование: %s [опции]\n", prog_name);
    printf("Опции:\n");
    printf("  -h, --host HOST     Адрес сервера (по умолчанию: localhost)\n");
    printf("  -p, --port PORT     TCP порт сервера (по умолчанию: 3042)\n");
    printf("  --help              Показать эту справку\n");
}

int main(int argc, char *argv[]) {
    const char *host = "localhost";
    int port = 3042;
    
    /* Опции командной строки */
    static struct option long_options[] = {
        {"host", required_argument, 0, 'h'},
        {"port", required_argument, 0, 'p'},
        {"help", no_argument, 0, 0},
        {0, 0, 0, 0}
    };
    
    int opt;
    int option_index = 0;
    
    while ((opt = getopt_long(argc, argv, "h:p:", long_options, &option_index)) != -1) {
        switch (opt) {
            case 'h':
                host = optarg;
                break;
            case 'p':
                port = atoi(optarg);
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
    
    /* Создаём и запускаем приложение */
    ClientApp app = client_app_create(host, port);
    client_app_run(&app);
    client_app_destroy(&app);
    
    return 0;
}

