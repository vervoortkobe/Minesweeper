//
// Created by vervo on 13/10/2025.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "filehandler.h"

int map_w = 10;
int map_h = 10;
int map_mines = 10;

char map[10][10];

void add_mines() {
    int placed = 0;
    srand((unsigned int)time(NULL));
    while (placed < map_mines) {
        int x = rand() % map_w;
        int y = rand() % map_h;
        if (map[y][x] == '0') {
            map[y][x] = 'M';
            placed++;
        }
    }
}

void fill_numbers() {
    for (int y = 0; y < map_h; y++) {
        for (int x = 0; x < map_w; x++) {
            if (map[y][x] == 'M') continue;
            int count = 0;
            for (int dy = -1; dy <= 1; dy++) {
                for (int dx = -1; dx <= 1; dx++) {
                    if (dx == 0 && dy == 0) continue;
                    int ny = y + dy, nx = x + dx;
                    if (ny >= 0 && ny < map_h && nx >= 0 && nx < map_w) {
                        if (map[ny][nx] == 'M') count++;
                    }
                }
            }
            map[y][x] = '0' + count;
        }
    }
}

void create_map() {
    for (int y = 0; y < map_h; y++) {
        for (int x = 0; x < map_w; x++) {
            map[y][x] ='0';
        }
    }
    add_mines();
    fill_numbers();
}

void export_map() {
    char buffer[map_w * map_h * 2 + map_h + 1];
    memset(buffer, 0, sizeof(buffer));
    int pos = 0;
    for (int y = 0; y < map_h; y++) {
        for (int x = 0; x < map_w; x++) {
            pos += snprintf(buffer + pos, sizeof(buffer) - pos, "%c ", map[y][x]);
        }
        pos += snprintf(buffer + pos, sizeof(buffer) - pos, "\n");
    }
    writeFile("field.txt", buffer);
}


void print_map() {
    for (int y = 0; y < map_h; y++) {
        for (int x = 0; x < map_w; x++) {
            printf("%c ", map[y][x]);
        }
        printf("\n");
    }
}

void run_map() {
    create_map();
    print_map();
    export_map();
}