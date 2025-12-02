#ifndef MINESWEEPER_map_height
#define MINESWEEPER_map_height

#include <stdbool.h>

// We declareren globale/externe variabelen voor de map dimensies en het aantal mijnen.
extern int map_width;
extern int map_height;
extern int map_mines;
int init_map(int w, int h, int mines);
void create_map();
void free_map();

// We declareren een struct om de waarden/eigenschappen van een cell mee op te slaan.
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
// We declareren een globale/externe 2D array van Cell structs om het speelveld in op te slaan.
extern Cell **map;
void print_map();

#endif // MINESWEEPER_map_height