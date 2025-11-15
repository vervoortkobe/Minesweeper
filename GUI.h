#ifndef MINESWEEPER_GUI_H
#define MINESWEEPER_GUI_H

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

// Choose a sensible image/window size for a given field. Tries sizes {100,75,50}
// and falls back to a computed size that fits the desktop if none of the
// presets fit. Returns 0 on success. The chosen image size is written to
// `out_image_size` and the corresponding window width/height to `out_window_w`/`out_window_h`.
int choose_image_and_window_size(int cols, int rows, int *out_image_size, int *out_window_w, int *out_window_h);

void initialize_gui(int window_width, int window_height);
void free_gui();
void draw_window();
void read_input();

// Alloceer/free de uncovered/flagged buffers met map data
int alloc_state_buffers(void);
void free_state_buffers(void);

// Nodig door opsplitsen van files. Hierdoor weet main ook wanneer er gestopt moet worden.
// Deze staat altijd op true, maar wordt enkel op false gezet wanneer de winanimatie op het einde gedaan is of wanneer de speler op het kruisje heeft gedrukt.
extern int should_continue;

// Een speelveld inladen via een opgeslagen field bestand
int load_game_file(const char *filename);

#endif //MINESWEEPER_GUI_H