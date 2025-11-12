SDL_INCLUDE ?= C:/Users/$(USERNAME)/scoop/apps/sdl2/current/include

all: GUI.c main.c map.c args.c filehandler.c GUI.h map.h args.h filehandler.h
	gcc -g -I"$(SDL_INCLUDE)" GUI.c main.c map.c args.c filehandler.c -o minesweeper.exe $(SDL_LIBS)

out: GUI.o main.o map.o args.o filehandler.o
	gcc GUI.o main.o map.o args.o filehandler.o -o minesweeper.exe $(SDL_LIBS)

SDL_LIBS ?= C:/Users/$(USERNAME)/scoop/apps/sdl2/current/lib/SDL2main.lib C:/Users/$(USERNAME)/scoop/apps/sdl2/current/lib/SDL2.lib

gui: GUI.c GUI.h
	gcc -g -I"$(SDL_INCLUDE)" -c GUI.c -o GUI.o

main: main.c GUI.h args.h map.h
	gcc -g -I"$(SDL_INCLUDE)" -c main.c -o main.o

map: map.c map.h
	gcc -c map.c -o map.o

args: args.c args.h
	gcc -c args.c -o args.o

filehandler: filehandler.c filehandler.h
	gcc -c filehandler.c -o filehandler.o

link: GUI.o main.o map.o args.o filehandler.o
	gcc GUI.o main.o map.o args.o filehandler.o -o minesweeper.exe $(SDL_LIBS)

clean:
	rm -f *.o minesweeper.exe
