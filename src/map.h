#ifndef MINESWEEPER_MAP_H
#define MINESWEEPER_MAP_H

#include <stdbool.h>

extern int map_w;
extern int map_h;
extern int map_mines;
int init_map(int w, int h, int mines);
void create_map();
void free_map();

typedef struct
{
    int x;
    int y;
} Coord;

typedef struct
{
    bool is_mine;
    int neighbour_mines;
    bool uncovered;
    bool flagged;
    bool removed;
    bool saved_uncovered;
} Cell;

int coord_index(Coord c);
void add_mines(const Coord *exclude);
void fill_map();
extern Cell *map;
#define MAP_AT(coord) (map[coord_index(coord)])
void print_map();

#endif // MINESWEEPER_MAP_H