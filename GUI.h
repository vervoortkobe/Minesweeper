#ifndef MINESWEEPER_GUI_H
#define MINESWEEPER_GUI_H

#include <stdio.h>
#include <stdlib.h>

/*
 * Importeer de benodigde functiedeclaraties uit SDL2.
 */
#include <SDL2/SDL.h>

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
#define IMAGE_HEIGHT 50
#define IMAGE_WIDTH 50

void initialize_gui(int window_width, int window_height);
void free_gui();
void draw_window();
void read_input();

#endif //MINESWEEPER_GUI_H