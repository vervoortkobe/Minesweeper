SDL_INCLUDE ?= C:/Users/$(USERNAME)/scoop/apps/sdl2/current/include

all: GUI.o main.o map.o args.o filehandler.o
	gcc GUI.o main.o map.o args.o filehandler.o -o minesweeper.exe $(SDL_LIBS)

SDL_LIBS ?= C:/Users/$(USERNAME)/scoop/apps/sdl2/current/lib/SDL2main.lib C:/Users/$(USERNAME)/scoop/apps/sdl2/current/lib/SDL2.lib

GUI.o: GUI.c GUI.h
	gcc -g -I"$(SDL_INCLUDE)" -c GUI.c -o GUI.o

main.o: main.c GUI.h args.h map.h
	gcc -g -I"$(SDL_INCLUDE)" -c main.c -o main.o

map.o: map.c map.h
	gcc -g -c map.c -o map.o

args.o: args.c args.h
	gcc -g -c args.c -o args.o

filehandler.o: filehandler.c filehandler.h
	gcc -g -c filehandler.c -o filehandler.o

clean:
	rm -f *.o minesweeper.exe
