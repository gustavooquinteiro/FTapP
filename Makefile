APP_SERVER_NAME = Server
APP_CLIENT_NAME = Client
GTK_FLAGS = `pkg-config --cflags --libs gtk+-3.0`
ECHO = 
GREEN =
NC =
ifeq ($(OS), Windows_NT)
	APP_EXTENSION = .exe
else
	APP_EXTENSION =
endif

ifeq ($(OS), Windows_NT)
	REM_CMD = del
	ECHO = ECHO
else
	UNAME_S := $(shell uname -s)
	ifeq ($(UNAME_S), Linux)
		REM_CMD = rm
		ECHO = echo -e -n
		GREEN=\033[0;32m
		NC=\033[0m
	endif
endif

user_interface: build client server

build:
	@ mkdir build
	@ $(ECHO) " [$(GREEN) OK $(NC)] Criado diretório para objetos\n"

client: include/user_interface.h
	@ $(ECHO) " Compilando client.c...\n"
	@ gcc $(GTK_FLAGS) -o $(APP_CLIENT_NAME) src/user_interface.c src/client.c $(GTK_FLAGS)
	@ $(ECHO) " [$(GREEN) OK $(NC)] Compilado client.c em $(APP_CLIENT_NAME)\n"

server:
	@ $(ECHO) " Compilando server.c...\n"
	@ gcc src/server.c $(GTK_FLAGS) -o $(APP_SERVER_NAME)
	@ $(ECHO) " [$(GREEN) OK $(NC)] Compilado server.c em $(APP_SERVER_NAME)\n"

clean:
	@ $(ECHO) " Limpando workspace...\n"
	@ $(REM_CMD) -f $(APP_CLIENT_NAME)$(APP_EXTENSION)
	@ $(REM_CMD) -f $(APP_SERVER_NAME)$(APP_EXTENSION)
	@ $(REM_CMD) -f build/*.o
	@ $(ECHO) " [$(GREEN) OK $(NC)] Workspace limpo\n"
