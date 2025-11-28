#include "utils.h"
#include "map.h"

// Helperfuncties voor het object Coord
Coord coord_make(int x, int y)
{
    return (Coord){x, y};
}

bool coord_equals(Coord a, Coord b)
{
    return a.x == b.x && a.y == b.y;
}

bool coord_valid(Coord c)
{
    return c.x >= 0 && c.x < map_w && c.y >= 0 && c.y < map_h;
}

int coord_index(Coord c)
{
    return c.y * map_w + c.x;
}

// Helperfuncties voor Cell state
bool cell_is_uncovered(Coord c)
{
    return MAP_AT(c).uncovered;
}

void cell_set_uncovered(Coord c, bool val)
{
    MAP_AT(c).uncovered = val;
}

bool cell_is_flagged(Coord c)
{
    return MAP_AT(c).flagged;
}

void cell_set_flagged(Coord c, bool val)
{
    MAP_AT(c).flagged = val;
}

bool cell_is_removed(Coord c)
{
    return MAP_AT(c).removed;
}

void cell_set_removed(Coord c, bool val)
{
    MAP_AT(c).removed = val;
}

bool cell_is_saved_uncovered(Coord c)
{
    return MAP_AT(c).saved_uncovered;
}

void cell_set_saved_uncovered(Coord c, bool val)
{
    MAP_AT(c).saved_uncovered = val;
}
