APP_SERVER_NAME = Server
APP_CLIENT_NAME = Client
GTK_FLAGS = `pkg-config --cflags --libs gtk+-3.0`

INTERFACE_C = ./src/user_interface.c
INTERFACE_H = ./include/user_interface.h
INTERFACE_O = $(subst .c,.o,$(subst src,build,$(INTERFACE_C)))

CLIENT_C = ./src/client.c 
CLIENT_H = ./include/client.h
CLIENT_O = $(subst .c,.o,$(subst src,build,$(CLIENT_C)))

SERVER_C = ./src/server.c
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
		ECHO = echo -e -n
		GREEN=\033[0;32m
		NC=\033[0m
	endif
endif

all: build $(APP_CLIENT_NAME) $(APP_SERVER_NAME)

build:
	@ mkdir build
	@ $(ECHO) " [$(GREEN) OK $(NC)] Criado diretório para objetos\n"

$(APP_CLIENT_NAME): $(INTERFACE_O)
	@ $(ECHO) " Compilando client.c...\n"
	@ gcc $(GTK_FLAGS) -o $(APP_CLIENT_NAME) $(INTERFACE_C) $(CLIENT_C) $(GTK_FLAGS)
	@ $(ECHO) " [$(GREEN) OK $(NC)] Compilado client.c em $(APP_CLIENT_NAME)\n"

$(INTERFACE_O): $(INTERFACE_C) $(INTERFACE_H)
	@ gcc $(GTK_FLAGS) -c $< -o $@ 
	@ $(ECHO) " [$(GREEN) OK $(NC)] Compilado $< em $@\n"

$(APP_SERVER_NAME):
	@ $(ECHO) " Compilando server.c...\n"
	@ gcc $(SERVER_C) $(GTK_FLAGS) -o $(APP_SERVER_NAME)
	@ $(ECHO) " [$(GREEN) OK $(NC)] Compilado server.c em $(APP_SERVER_NAME)\n"

clean:
	@ $(ECHO) " Limpando workspace...\n"
	@ $(REM_CMD) -f $(APP_CLIENT_NAME)$(APP_EXTENSION)
	@ $(REM_CMD) -f $(APP_SERVER_NAME)$(APP_EXTENSION)
	@ $(REM_CMD) -f build/*.o
	@ $(ECHO) " [$(GREEN) OK $(NC)] Workspace limpo\n"
