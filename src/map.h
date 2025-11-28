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

#define COORD_IDX(c, w) ((c).y * (w) + (c).x)

// Helperfuncties voor het object Coord
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

// Deze functies maken het gemakkelijker om de state van een cell te veranderen/op te vragen.
static inline bool cell_is_uncovered(Coord c)
{
    return MAP_AT(c).uncovered;
}

static inline void cell_set_uncovered(Coord c, bool val)
{
    MAP_AT(c).uncovered = val;
}

static inline bool cell_is_flagged(Coord c)
{
    return MAP_AT(c).flagged;
}

static inline void cell_set_flagged(Coord c, bool val)
{
    MAP_AT(c).flagged = val;
}

static inline bool cell_is_removed(Coord c)
{
    return MAP_AT(c).removed;
}

static inline void cell_set_removed(Coord c, bool val)
{
    MAP_AT(c).removed = val;
}

static inline bool cell_is_saved_uncovered(Coord c)
{
    return MAP_AT(c).saved_uncovered;
}

static inline void cell_set_saved_uncovered(Coord c, bool val)
{
    MAP_AT(c).saved_uncovered = val;
}

void print_map();

#endif // MINESWEEPER_MAP_H