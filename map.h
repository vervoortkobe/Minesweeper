#ifndef MINESWEEPER_MAP_H
#define MINESWEEPER_MAP_H

extern int map_w;
extern int map_h;
extern int map_mines;
int init_map(int w, int h, int mines);
void create_map();
int load_map_from_file(const char *filename);
void free_map();
void add_mines_excluding(int exclude_x, int exclude_y);
void fill_numbers();
extern char *map;
#define MAP(r, c) (map[(r) * map_w + (c)])
void print_map();

#endif //MINESWEEPER_MAP_H