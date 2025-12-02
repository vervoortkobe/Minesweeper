SDL_INCLUDE ?= C:/Users/$(USERNAME)/scoop/apps/sdl2/current/include
SDL_LIBS ?= C:/Users/$(USERNAME)/scoop/apps/sdl2/current/lib/SDL2main.lib C:/Users/$(USERNAME)/scoop/apps/sdl2/current/lib/SDL2.lib

SRC_DIR = ./src
OUT_DIR = ./out
OUT_NAME = game.exe

CXX = gcc
CC_FLAGS = -g -Wall -O3 -I"$(SDL_INCLUDE)"
LINK_FLAGS = $(SDL_LIBS)

SOURCES = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c,$(OUT_DIR)/%.o,$(SOURCES))

all: main

main: $(OUT_DIR) $(OBJS)
	$(CXX) $(OBJS) $(LINK_FLAGS) -o $(OUT_NAME)

$(OUT_DIR)/%.o: $(SRC_DIR)/%.c
	$(CXX) $(CC_FLAGS) -o $@ -c $^

run: all
	./$(OUT_NAME)
	
clean:
	rm -f *.o game.exe