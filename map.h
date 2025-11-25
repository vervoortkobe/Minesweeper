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
    int adjacent_mines;  // Count of adjacent mines
} Cell;

#define COORD_IDX(c, w) ((c).y * (w) + (c).x)

// Coord helper functions
static inline Coord coord_make(int x, int y)
{
    return (Coord){x, y};
}

static inline bool coord_equals(Coord a, Coord b)
{
    return a.x == b.x && a.y == b.y;
}

static inline bool coord_valid(Coord c)
{
    return c.x >= 0 && c.x < map_w && c.y >= 0 && c.y < map_h;
}

static inline int coord_index(Coord c)
{
    return c.y * map_w + c.x;
}

void add_mines(const Coord *exclude);
void fill_map();
extern Cell *map;

#define MAP_AT(coord) (map[coord_index(coord)])

void print_map();

#endif // MINESWEEPER_MAP_H