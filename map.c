#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "map.h"

// Instantieer de standaardwaarden voor het speelveld.
int map_w = 10;
int map_h = 10;
int map_mines = 10;
// Instantieer een standaard speelveld met een pointer naar een char (begin van de array). Het wordt later lineair ingevuld.
char *map = NULL;

/*
* De waarden van w en h worden gecheckt of deze mogelijk zijn.
* Zo ja, worden deze toegekend aan map_w en map_h.
* We alloceren geheugen voor de standaard map van size map_w * map_h.
*/
int init_map(int w, int h, int mines) {
    if (w <= 0 || h <= 0) return -1;
    map_w = w;
    map_h = h;
    map_mines = mines;
    if (map) free(map);
    map = (char*)malloc((int)map_w * map_h);
    if (!map) return -1; // memory allocation niet gelukt -> return -1
    return 0;
}

// De aanmgemaakte map wordt opgevuld met '0' karakters.
void create_map() {
    if (map == NULL) init_map(map_w, map_h, map_mines);
    for (int x = 0; x < map_w; x++) {
        for (int y = 0; y < map_h; y++) {
            MAP(x, y) = '0';
        }
    }
}

// Om de map in de console uit te printen.
void print_map() {
    for (int x = 0; x < map_w; x++) {
        for (int y = 0; y < map_h; y++) {
            printf("%c ", MAP(x, y));
        }
        printf("\n");
    }
}

// Deze functie zal de map opvullen met nummers, rekening houdend met de reeds gelegde mijnen.
void fill_map() {
    for (int x = 0; x < map_w; x++) {
        for (int y = 0; y < map_h; y++) {
            if (MAP(x, y) == 'M') continue;
            int count = 0;
            for (int dy = -1; dy <= 1; dy++) {
                for (int dx = -1; dx <= 1; dx++) {
                    if (dx == 0 && dy == 0) continue;
                    int ny = y + dy, nx = x + dx;
                    if (ny >= 0 && ny < map_h && nx >= 0 && nx < map_w) {
                        if (MAP(nx, ny) == 'M') count++;
                    }
                }
            }
            MAP(x, y) = '0' + count;
        }
    }
}

/*
* Na het klikken op een veld, wordt de map aangemaakt en random opgevuld met mijnen.
* Daarna wordt de fill_map functie aangeroepen om de map verder op te vullen met nummers.
*/
void add_mines(int exclude_x, int exclude_y) {
    int placed = 0;
    while (placed < map_mines) {
        int x = rand() % map_w;
        int y = rand() % map_h;
        if (exclude_x >= 0 && x == exclude_x && y == exclude_y) continue;
        if (MAP(x, y) == '0') {
            MAP(x, y) = 'M';
            placed++;
        }
    }
    fill_map();
}

// Om de map te dealloceren, nadat het spel afgelopen is.
void free_map() {
    if (map) {
        free(map);
        map = NULL;
    }
}