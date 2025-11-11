//
// Created by vervo on 13/10/2025.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "map.h"

int map_w = 10;
int map_h = 10;
int map_mines = 10;

char *map = NULL; /* linear buffer map_h * map_w */

static int rng_seeded = 0;

void seed_rng_if_needed() {
    if (!rng_seeded) {
        srand((unsigned int)time(NULL));
        rng_seeded = 1;
    }
}

void fill_numbers() {
    for (int y = 0; y < map_h; y++) {
        for (int x = 0; x < map_w; x++) {
            if (MAP(y,x) == 'M') continue;
            int count = 0;
            for (int dy = -1; dy <= 1; dy++) {
                for (int dx = -1; dx <= 1; dx++) {
                    if (dx == 0 && dy == 0) continue;
                    int ny = y + dy, nx = x + dx;
                    if (ny >= 0 && ny < map_h && nx >= 0 && nx < map_w) {
                        if (MAP(ny,nx) == 'M') count++;
                    }
                }
            }
            MAP(y,x) = '0' + count;
        }
    }
}

void create_map() {
    if (map == NULL) init_map(map_w, map_h, map_mines);
    for (int y = 0; y < map_h; y++) {
        for (int x = 0; x < map_w; x++) {
            MAP(y,x) ='0';
        }
    }
}


void print_map() {
    for (int y = 0; y < map_h; y++) {
        for (int x = 0; x < map_w; x++) {
            printf("%c ", MAP(y,x));
        }
        printf("\n");
    }
}

void add_mines_excluding(int exclude_x, int exclude_y) {
    seed_rng_if_needed();
    int placed = 0;
    while (placed < map_mines) {
        int x = rand() % map_w;
        int y = rand() % map_h;
        if (exclude_x >= 0 && x == exclude_x && y == exclude_y) continue;
        if (MAP(y,x) == '0') {
            MAP(y,x) = 'M';
            placed++;
        }
    }
    fill_numbers();
}

int init_map(int w, int h, int mines) {
    if (w <= 0 || h <= 0) return -1;
    map_w = w; map_h = h; map_mines = mines;
    if (map) free(map);
    map = (char*)malloc((size_t)map_w * map_h);
    if (!map) return -1;
    for (int i = 0; i < map_w * map_h; ++i) map[i] = '0';
    return 0;
}

void free_map() {
    if (map) {
        free(map);
        map = NULL;
    }
}

int load_map_from_file(const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) return -1;
    // read lines into buffer
    char line[1024];
    char *lines[1024];
    int rows = 0;
    while (fgets(line, sizeof(line), f)) {
        size_t len = strlen(line);
        while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r')) { line[--len] = '\0'; }
        lines[rows] = strdup(line);
        rows++;
    }
    fclose(f);
    if (rows == 0) return -1;
    // determine columns by splitting first line
    int cols = 0;
    char *p = strtok(lines[0], " \t");
    while (p) { cols++; p = strtok(NULL, " \t"); }
    if (cols <= 0) return -1;
    // allocate map
    if (init_map(cols, rows, 0) != 0) return -1;
    // fill map and count mines
    int mines = 0;
    for (int y = 0; y < rows; ++y) {
        int c = 0;
        char *token = strtok(lines[y], " \t");
        while (token && c < cols) {
            MAP(y,c) = token[0];
            if (MAP(y,c) == 'M') mines++;
            c++;
            token = strtok(NULL, " \t");
        }
        // free line
        free(lines[y]);
    }
    map_mines = mines;
    fill_numbers();
    return 0;
}