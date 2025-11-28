#ifndef MINESWEEPER_UTILS_H
#define MINESWEEPER_UTILS_H

#include <stdbool.h>
#include "map.h"

// Helperfuncties voor het object Coord
Coord coord_make(int x, int y);
bool coord_equals(Coord a, Coord b);
bool coord_valid(Coord c);
int coord_index(Coord c);

// Helperfuncties voor Cell state
bool cell_is_uncovered(Coord c);
void cell_set_uncovered(Coord c, bool val);
bool cell_is_flagged(Coord c);
void cell_set_flagged(Coord c, bool val);
bool cell_is_removed(Coord c);
void cell_set_removed(Coord c, bool val);
bool cell_is_saved_uncovered(Coord c);
void cell_set_saved_uncovered(Coord c, bool val);

#endif // MINESWEEPER_UTILS_H
