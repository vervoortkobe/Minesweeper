#ifndef MINESWEEPER_MAP_H
#define MINESWEEPER_MAP_H

extern int map_w;
extern int map_h;
extern int map_mines;
int init_map(int w, int h, int mines);
void create_map();
void free_map();
typedef struct { int x; int y; } Coord;

#define COORD_IDX(c, w) ((c).y * (w) + (c).x)

void add_mines(const Coord *exclude);
void fill_map();
extern char *map;

#define MAP(c, r) (map[(r) * map_w + (c)])
void print_map();

#endif //MINESWEEPER_MAP_H