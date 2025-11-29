#ifndef MINESWEEPER_map_height
#define MINESWEEPER_map_height

#include <stdbool.h>

extern int map_width;
extern int map_height;
extern int map_mines;
int init_map(int w, int h, int mines);
void create_map();
void free_map();

typedef struct
{
    bool is_mine;
    int neighbour_mines;
    bool uncovered;
    bool flagged;
    bool removed;
    bool saved_uncovered;
} Cell;

void add_mines(int exclude_x, int exclude_y);
void fill_map();
extern Cell **map;
void print_map();

#endif // MINESWEEPER_map_height