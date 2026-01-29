# Makefile для CAsciiarena
# Сборка клиента и сервера для macOS/Linux

CC = clang
CFLAGS = -Wall -Wextra -std=c11 -g -I. -D_DEFAULT_SOURCE
CLIENT_LIBS = -lncurses

# Определение ОС
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    # macOS - проверяем Homebrew ncurses
    NCURSES_PREFIX := $(shell brew --prefix ncurses 2>/dev/null)
    ifneq ($(NCURSES_PREFIX),)
        CFLAGS += -I$(NCURSES_PREFIX)/include
        CLIENT_LIBS = -L$(NCURSES_PREFIX)/lib -lncurses
    endif
endif

# Объектные файлы
CORE_OBJS = core/vec2.o core/direction.o core/character.o core/map.o \
            core/entity.o core/spell.o core/arena.o core/player.o core/game.o
NET_OBJS = net/socket.o net/encoder.o net/protocol.o
UI_OBJS = ui/terminal.o ui/input.o ui/renderer.o ui/widgets.o ui/menu.o ui/arena_view.o
COMMON_OBJS = common/util.o

CLIENT_OBJS = client/client.o client/app.o client/state.o \
              $(UI_OBJS) $(NET_OBJS) $(CORE_OBJS) $(COMMON_OBJS)
SERVER_OBJS = server/server_main.o server/server.o server/session.o \
              $(NET_OBJS) $(CORE_OBJS) $(COMMON_OBJS)

# Цели
all: asciiarena_client asciiarena_server

asciiarena_client: $(CLIENT_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(CLIENT_LIBS)

asciiarena_server: $(SERVER_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# Правило компиляции .c -> .o
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Тестовые программы
test_game: test_game.o $(CORE_OBJS) $(COMMON_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

test_render: test_render.o $(CORE_OBJS) $(UI_OBJS) $(COMMON_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(CLIENT_LIBS)

# Очистка
clean:
	rm -f asciiarena_client asciiarena_server test_game test_render
	rm -f $(CLIENT_OBJS) $(SERVER_OBJS)
	rm -f test_game.o test_render.o

.PHONY: all clean