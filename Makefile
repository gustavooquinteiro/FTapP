APP_SERVER_NAME = Server
APP_CLIENT_NAME = Client

GTK_FLAGS = `pkg-config --cflags --libs gtk+-3.0`
FLAGS = -lpthread
SERVER_C = ./src/server.c
CLIENT_C = ./src/application.c

USER_INTERFACE = src/user_interface.c

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

$(APP_CLIENT_NAME): ./build/requisition_queue.o ./build/network.o ./build/transport.o \
./build/client.o ./build/user_interface.o ./build/pkg_queue.o
	gcc $(CLIENT_C) $^ $(GTK_FLAGS) -o $@  
	@ $(ECHO) " [$(GREEN) OK $(NC)] Executável construido: $@"

$(APP_SERVER_NAME): ./build/transport.o ./build/requisition_queue.o ./build/network.o ./build/pkg_queue.o
	gcc $(SERVER_C) $^ $(FLAGS) -o $@
	@ $(ECHO) " [$(GREEN) OK $(NC)] Executável construido: $@"



build/user_interface.o: ./src/user_interface.c ./include/user_interface.h
	gcc -c $(GTK_FLAGS) $< -o $@
	@ $(ECHO) " [$(GREEN) OK $(NC)] Compilado $< em $@"

build/%.o: ./src/%.c ./include/%.h 
	gcc -c $< -o $@ 
	@ $(ECHO) " [$(GREEN) OK $(NC)] Compilado $< em $@"
	


clean:
	@ $(ECHO) " Limpando workspace..."
	@ $(REM_CMD) -f $(APP_CLIENT_NAME)$(APP_EXTENSION)
	@ $(REM_CMD) -f $(APP_SERVER_NAME)$(APP_EXTENSION)
	@ $(REM_CMD) -f build/*.o
	@ $(ECHO) " [$(GREEN) OK $(NC)] Workspace limpo"
