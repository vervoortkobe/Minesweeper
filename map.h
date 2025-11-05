//
// Created by vervo on 14/10/2025.
//

#ifndef MINESWEEPER_MAP_H
#define MINESWEEPER_MAP_H

extern int map_w;
extern int map_h;
extern int map_mines;
void create_map();
void export_map();
void add_mines_excluding(int exclude_x, int exclude_y);
void fill_numbers();
extern char map[10][10];
void print_map();

#endif //MINESWEEPER_MAP_H