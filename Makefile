APP_SERVER_NAME = Server
APP_CLIENT_NAME = Client
GTK_FLAGS = `pkg-config --cflags --libs gtk+-3.0`

ifeq ($(OS), Windows_NT)
APP_EXTENSION = .exe
else
APP_EXTENSION =
endif

ifeq ($(OS), Windows_NT)
REM_CMD = del
else
ifeq ($(UNAME_S), Linux)
REM_CMD = rm
endif
endif

user_interface: build

build:
	mkdir build

client: include/user_interface.h
	gcc `pkg-config --cflags gtk+-3.0` -o $(APP_CLIENT_NAME) src/user_interface.c src/client.c `pkg-config --libs gtk+-3.0`

server:
	gcc src/server.c $(GTK_FLAGS) -o $(APP_SERVER_NAME)

clean:
	$(REM_CMD) -f $(APP_CLIENT_NAME)$(APP_EXTENSION)
	$(REM_CMD) -f $(APP_SERVER_NAME)$(APP_EXTENSION)
	$(REM_CMD) -f /build/*.o
