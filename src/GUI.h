#ifndef MINESWEEPER_GUI_H
#define MINESWEEPER_GUI_H

#include <stdbool.h>
#include "map.h"

/*
 * De hoogte en breedte van het venster (in pixels).
 * Deze dimensies zijn arbitrair gekozen. Deze dimensies hangen mogelijk af van de grootte van het speelveld.
 */
#define WINDOW_HEIGHT 500
#define WINDOW_WIDTH 600

/*
 * De hoogte en breedte (in pixels) van de afbeeldingen voor de vakjes in het speelveld die getoond worden.
 * Als je andere afbeelding wil gebruiken in je GUI, zorg er dan voor dat deze
 * dimensies ook aangepast worden.
 */
#define DEFAULT_IMAGE_SIZE 50
int determine_img_win_size(int cols, int rows, int *out_image_size, int *out_window_w, int *out_window_h);
void initialize_gui(int window_width, int window_height);
void free_gui();
void draw_window();
void read_input();
int alloc_game_states(void);
void free_game_states(void);
extern int should_continue;
int load_file(const char *filename);
void save_game();

#endif // MINESWEEPER_GUI_H
