//
// Created by vervo on 14/10/2025.
//

#ifndef MINESWEEPER_MAP_H
#define MINESWEEPER_MAP_H

extern int map_w;
extern int map_h;
extern int map_mines;
int init_map(int w, int h, int mines);
void create_map();
void export_map();
int load_map_from_file(const char *filename);
void free_map();
void add_mines_excluding(int exclude_x, int exclude_y);
void fill_numbers();
extern char *map; /* linear buffer of size map_w * map_h */
/* macro to access map as 2D: MAP(row, col) */
#define MAP(r,c) (map[(r) * map_w + (c)])
void print_map();

#endif //MINESWEEPER_MAP_H