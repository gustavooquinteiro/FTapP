APP_SERVER_NAME = Server
APP_CLIENT_NAME = Client

GTK_FLAGS = `pkg-config --cflags --libs gtk+-3.0`
FLAGS = -lpthread
SERVER_C = ./src/server.c
CLIENT_C = ./src/application.c

USER_INTERFACE_SRC = ./src/user_interface.c
USER_INTERFACE_LIB = ./include/user_interface.h
USER_INTERFACE_OBJ = ./build/user_interface.o

CLIENT_IGNORE = $(SERVER_C) $(CLIENT_C)
CLIENT_DEPS = $(subst .c,.o, $(subst ./src/,./build/, $(filter-out $(CLIENT_IGNORE), $(wildcard ./src/*.c))))

SERVER_IGNORE = $(SERVER_C) $(CLIENT_C) $(USER_INTERFACE_SRC) ./src/client.c
SERVER_DEPS = $(subst .c,.o, $(subst ./src/,./build/, $(filter-out $(SERVER_IGNORE), $(wildcard ./src/*.c))))

GREEN =
NC =

ifeq ($(OS), Windows_NT)
	APP_EXTENSION = .exe
	REM_CMD = del
	ECHO = ECHO
else
	APP_EXTENSION =
	UNAME_S := $(shell uname -s)
	ifeq ($(UNAME_S), Linux)
		REM_CMD = rm
		ECHO = echo -e
		GREEN=\033[0;32m
		NC=\033[0m
	endif
endif

all: build $(APP_CLIENT_NAME) $(APP_SERVER_NAME)

build:
	@ mkdir build
	@ $(ECHO) " [$(GREEN) OK $(NC)] Criado diretório para objetos"

$(APP_CLIENT_NAME): $(CLIENT_DEPS)
	@ gcc $(CLIENT_C) $^ $(GTK_FLAGS) -o $@ -g
	@ $(ECHO) " [$(GREEN) OK $(NC)] Executável construido: $@"

$(APP_SERVER_NAME): $(SERVER_DEPS)
	@ gcc $(SERVER_C) $^ $(FLAGS) -o $@ -g
	@ $(ECHO) " [$(GREEN) OK $(NC)] Executável construido: $@"

$(USER_INTERFACE_OBJ): $(USER_INTERFACE_SRC) $(USER_INTERFACE_LIB)
	@ gcc -c $(GTK_FLAGS) $< -o $@
	@ $(ECHO) " [$(GREEN) OK $(NC)] Compilado $< em $@"

build/%.o: ./src/%.c ./include/%.h 
	@ gcc -c $< -o $@ 
	@ $(ECHO) " [$(GREEN) OK $(NC)] Compilado $< em $@"

clean:
	@ $(ECHO) " Limpando workspace..."
	@ $(REM_CMD) -f $(APP_CLIENT_NAME)$(APP_EXTENSION)
	@ $(REM_CMD) -f $(APP_SERVER_NAME)$(APP_EXTENSION)
	@ $(REM_CMD) -f build/*.o
	@ $(ECHO) " [$(GREEN) OK $(NC)] Workspace limpo"
