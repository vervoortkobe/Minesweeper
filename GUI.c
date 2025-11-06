#include <stdlib.h>
#include "GUI.h"
#include <SDL2/SDL.h>
#include <stdio.h>
#include <string.h>
#include "map.h"
#include <dirent.h>
#include <stdbool.h>

/*
 * Deze renderer wordt gebruikt om figuren in het venster te tekenen. De renderer
 * wordt geïnitialiseerd in de initialize_window-functie.
 */
static SDL_Renderer *renderer;

/* De afbeelding die een vakje met een "1" in voorstelt. */
static SDL_Texture *digit_textures[9] = { NULL };
static SDL_Texture *digit_covered_texture = NULL;
static SDL_Texture *digit_flagged_texture = NULL;
static SDL_Texture *digit_mine_texture = NULL;

static SDL_DisplayMode dm;
int calcDisplaySize() {
    if (SDL_GetDesktopDisplayMode(0, &dm) != 0)
    {
        SDL_Log("SDL_GetDesktopDisplayMode failed: %s", SDL_GetError());
        return 1;
    }

    int w, h;
    w = dm.w;
    h = dm.h;
    return 0;
}

/*
 * Onderstaande twee lijnen maken deel uit van de minimalistische voorbeeldapplicatie:
 * ze houden de laatste positie bij waar de gebruiker geklikt heeft.
 */
int mouse_x = 0;
int mouse_y = 0;

/*
 * Geeft aan of de applicatie moet verdergaan.
 * Dit is waar zolang de gebruiker de applicatie niet wilt afsluiten door op het kruisje te klikken.
 */
int should_continue = 1;

/*
 * Dit is het venster dat getoond zal worden en waarin het speelveld weergegeven wordt.
 * Dit venster wordt aangemaakt bij het initialiseren van de GUI en wordt weer afgebroken
 * wanneer het spel ten einde komt.
 */
static SDL_Window *window;

// Game state for GUI: covered/uncovered/flagged
static bool uncovered[10][10];
static bool flagged[10][10];
static bool mines_placed = false; // whether add_mines_excluding has been called
static bool show_bombs = false; // toggled with key 'b'

/*
 * Controleert of het gegeven event "relevant" is voor dit spel.
 * We gebruiken in deze GUI enkel muiskliks, toetsdrukken, en de "Quit"
 * van het venster, dus enkel deze soorten events zijn "relevant".
 */
static int is_relevant_event(SDL_Event *event) {
    if (event == NULL) {
        return 0;
    }
    return (event->type == SDL_MOUSEBUTTONDOWN) ||
           (event->type == SDL_KEYDOWN) ||
           (event->type == SDL_QUIT);
}

/*
 * Vangt de input uit de GUI op. Deze functie is al deels geïmplementeerd, maar je moet die
 * zelf nog afwerken.
 * Je mag natuurlijk alles aanpassen aan deze functie, inclusies return-type en argumenten.
 */
void read_input() {
    SDL_Event event;

    /*
     * Handelt alle input uit de GUI af.
     * Telkens de speler een input in de GUI geeft (bv. een muisklik, muis bewegen, toetsindrukken
     * enz.) wordt er een 'event' (van het type SDL_Event) gegenereerd dat hier wordt afgehandeld.
     *
     * Niet al deze events zijn relevant voor jou: als de muis bv. gewoon wordt bewogen, hoef
     * je niet te reageren op dit event.
     * We gebruiken daarom de is_relevant_event-functie om niet-gebruikte events weg te
     * filteren, zonder dat ze de applicatie vertragen of de GUI minder responsief maken.
     *
     * Zie ook https://wiki.libsdl.org/SDL_PollEvent
     */
    while (1) {
        int event_polled = SDL_PollEvent(&event);
        if (event_polled == 0) {
            return;
        } else if (is_relevant_event(&event)) {
            break;
        }
    }

    switch (event.type) {
    case SDL_KEYDOWN:
        if (event.key.keysym.sym == SDLK_p) {
            printf("P pressed\n");
        } else if (event.key.keysym.sym == SDLK_b) {
            show_bombs = !show_bombs;
            printf("Toggle show bombs: %d\n", show_bombs);
        }
        break;
    case SDL_QUIT:
        // De gebruiker heeft op het kruisje van het venster geklikt om de applicatie te stoppen.
        should_continue = 0;
        break;
    case SDL_MOUSEBUTTONDOWN:
        /*
         * De speler heeft met de muis geklikt: met de onderstaande lijn worden de coördinaten in het
         * het speelveld waar de speler geklikt heeft bewaard in de variabelen mouse_x en mouse_y.
         */
        mouse_x = event.button.x;
        mouse_y = event.button.y;

        int grid_rows = 10, grid_cols = 10;
        int cell_w = WINDOW_WIDTH / grid_cols;
        int cell_h = WINDOW_HEIGHT / grid_rows;

        int clicked_col = mouse_x / cell_w;
        int clicked_row = mouse_y / cell_h;

        if (clicked_col < 0) clicked_col = 0;
        if (clicked_col >= grid_cols) clicked_col = grid_cols - 1;
        if (clicked_row < 0) clicked_row = 0;
        if (clicked_row >= grid_rows) clicked_row = grid_rows - 1;

        if (event.button.button == SDL_BUTTON_RIGHT) {
            flagged[clicked_row][clicked_col] = !flagged[clicked_row][clicked_col];
            printf("Right-click at (%d,%d) -> cell (%d,%d) flag=%d\n", mouse_x, mouse_y, clicked_row, clicked_col, flagged[clicked_row][clicked_col]);
        } else {
            printf("Clicked at (%d,%d) -> cell (%d,%d)\n", mouse_x, mouse_y, clicked_row, clicked_col);
            if (!mines_placed) {
                add_mines_excluding(clicked_col, clicked_row);
                print_map();
                export_map();
                mines_placed = true;
            }
            if (map[clicked_row][clicked_col] == 'M') {
                uncovered[clicked_row][clicked_col] = true;
            } else if (map[clicked_row][clicked_col] == '0') {
                int stack_size = map_w * map_h;
                int stack_x[1000];
                int stack_y[1000];
                int sp = 0;
                // push
                stack_x[sp] = clicked_col;
                stack_y[sp] = clicked_row;
                sp++;
                while (sp > 0) {
                    sp--;
                    int cx = stack_x[sp];
                    int cy = stack_y[sp];
                    if (cx < 0 || cx >= map_w || cy < 0 || cy >= map_h) continue;
                    if (uncovered[cy][cx]) continue;
                    uncovered[cy][cx] = true;
                    if (map[cy][cx] == '0') {
                        for (int dy = -1; dy <= 1; dy++) {
                            for (int dx = -1; dx <= 1; dx++) {
                                if (dx == 0 && dy == 0) continue;
                                int nx = cx + dx;
                                int ny = cy + dy;
                                if (nx < 0 || nx >= map_w || ny < 0 || ny >= map_h) continue;
                                if (!uncovered[ny][nx] && !flagged[ny][nx]) {
                                    stack_x[sp] = nx;
                                    stack_y[sp] = ny;
                                    sp++;
                                    if (sp >= 1000) sp = 999;
                                }
                            }
                        }
                    }
                }
            } else {
                uncovered[clicked_row][clicked_col] = true;
            }
        }
        break;
    }
}

void draw_window() {
        /*
         * Maakt het venster blanco.
         */
        SDL_RenderClear(renderer);

        /*
         * Bereken de plaats (t.t.z., de rechthoek) waar een afbeelding moet getekend worden.
         * Dit is op de plaats waar de gebruiker het laatst geklikt heeft.
         */
        SDL_Rect rectangle = { mouse_x, mouse_y, IMAGE_WIDTH, IMAGE_HEIGHT };
    /* Tekent de afbeelding op die plaats. */
    // Use the '1' texture as marker for last click, fall back if not loaded
    if (digit_textures[1]) SDL_RenderCopy(renderer, digit_textures[1], NULL, &rectangle);
    else if (digit_covered_texture) SDL_RenderCopy(renderer, digit_covered_texture, NULL, &rectangle);

        int grid_rows = 10, grid_cols = 10;
        int cell_w = WINDOW_WIDTH / grid_cols;
        int cell_h = WINDOW_HEIGHT / grid_rows;

        for (int row = 0; row < grid_rows; ++row) {
            for (int col = 0; col < grid_cols; ++col) {
                SDL_Rect rect = { col * cell_w, row * cell_h, cell_w, cell_h };
                // If user asked to show bombs, draw them regardless of uncovered state
                if (show_bombs && map[row][col] == 'M') {
                    SDL_RenderCopy(renderer, digit_mine_texture, NULL, &rect);
                    continue;
                }

                if (uncovered[row][col]) {
                    char c = map[row][col];
                    if (c == 'M') {
                        SDL_RenderCopy(renderer, digit_mine_texture, NULL, &rect);
                    } else if (c >= '0' && c <= '8') {
                        int idx = c - '0';
                        if (digit_textures[idx]) {
                            SDL_RenderCopy(renderer, digit_textures[idx], NULL, &rect);
                        } else {
                            SDL_RenderCopy(renderer, digit_covered_texture, NULL, &rect);
                        }
                    } else {
                        SDL_RenderCopy(renderer, digit_covered_texture, NULL, &rect);
                    }
                } else {
                    if (flagged[row][col] && digit_flagged_texture) {
                        SDL_RenderCopy(renderer, digit_flagged_texture, NULL, &rect);
                    } else if (digit_covered_texture) {
                        SDL_RenderCopy(renderer, digit_covered_texture, NULL, &rect);
                    }
                }
            }
        }

    /*
     * De volgende lijn moet zeker uitgevoerd worden op het einde van de functie.
     * Wanneer aan de renderer gevraagd wordt om iets te tekenen, wordt het venster pas aangepast
     * wanneer de SDL_RenderPresent-functie wordt aangeroepen.
     */
        SDL_RenderPresent(renderer);
}

/*
 * Initialiseert het venster en alle extra structuren die nodig zijn om het venster te manipuleren.
 */
void initialize_window(const char *title, int window_width, int window_height) {
        /*
         * Code o.a. gebaseerd op:
         * http://lazyfoo.net/tutorials/SDL/02_getting_an_image_on_the_screen/index.php
         */
        if (SDL_Init(SDL_INIT_VIDEO) < 0) {
                printf("Could not initialize SDL: %s\n", SDL_GetError());
                exit(1);
        }

        calcDisplaySize();

        /* Maak het venster aan met de gegeven dimensies en de gegeven titel. */
        window = SDL_CreateWindow(title, (dm.w - window_width) / 2, (dm.h - window_height) / 2, window_width, window_height, SDL_WINDOW_SHOWN);

        if (window == NULL) {
                /* Er ging iets verkeerd bij het initialiseren. */
                printf("Couldn't set screen mode to required dimensions: %s\n", SDL_GetError());
                exit(1);
        }

        /* Initialiseert de renderer. */
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);
        /* Laat de default-kleur die de renderer in het venster tekent wit zijn. */
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
}

/*
 * Laadt alle afbeeldingen die getoond moeten worden in.
 */
void initialize_textures() {
        /*
         * Laadt de afbeeldingen in.
         * Indien een afbeelding niet kon geladen worden (bv. omdat het pad naar de afbeelding verkeerd is),
         * geeft SDL_LoadBMP een NULL-pointer terug.
         */
        const char *num_files[9] = {
            "Images/0.bmp","Images/1.bmp","Images/2.bmp",
            "Images/3.bmp","Images/4.bmp","Images/5.bmp",
            "Images/6.bmp","Images/7.bmp","Images/8.bmp"
        };
        for (int i = 0; i < 9; ++i) {
            SDL_Surface *s = SDL_LoadBMP(num_files[i]);
            if (s) {
                digit_textures[i] = SDL_CreateTextureFromSurface(renderer, s);
                SDL_FreeSurface(s);
            } else {
                digit_textures[i] = NULL;
            }
        }

        SDL_Surface *digit_covered_surface = SDL_LoadBMP("Images/covered.bmp");
        SDL_Surface *digit_flagged_surface = SDL_LoadBMP("Images/flagged.bmp");
        SDL_Surface *digit_mine_surface = SDL_LoadBMP("Images/mine.bmp");

        digit_covered_texture = SDL_CreateTextureFromSurface(renderer, digit_covered_surface);
        digit_flagged_texture = SDL_CreateTextureFromSurface(renderer, digit_flagged_surface);
        digit_mine_texture = SDL_CreateTextureFromSurface(renderer, digit_mine_surface);
        /* Dealloceer de tijdelijke SDL_Surfaces die werden aangemaakt. */
        SDL_FreeSurface(digit_covered_surface);
        SDL_FreeSurface(digit_flagged_surface);
        SDL_FreeSurface(digit_mine_surface);
}

/*
 * Initialiseert onder het venster waarin het speelveld getoond zal worden, en de texture van de afbeelding die getoond zal worden.
 * Deze functie moet aangeroepen worden aan het begin van het spel, vooraleer je de spelwereld begint te tekenen.
 */
void initialize_gui(int window_width, int window_height) {
        initialize_window("Minesweeper", window_width, window_height);
        initialize_textures();
    // Maakt van wit de standaard, blanco achtergrondkleur.
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
}

/*
 * Dealloceert alle SDL structuren die geïnitialiseerd werden.
 */
void free_gui() {
    // Dealloceert de afbeeldingen.
    for (int i = 0; i < 9; ++i) {
        if (digit_textures[i]) SDL_DestroyTexture(digit_textures[i]);
    }
    SDL_DestroyTexture(digit_covered_texture);
    SDL_DestroyTexture(digit_flagged_texture);
    SDL_DestroyTexture(digit_mine_texture);
    // Dealloceert de renderer.
    SDL_DestroyRenderer(renderer);
    // Dealloceert het venster.
    SDL_DestroyWindow(window);

    // Sluit SDL af.
    SDL_Quit();
}

int main(int argc, char *argv[]) {
    /*if (argc == 2 && strcmp(argv[1], "-f") == 0) {
        printf("Usage: %s\n", argv[0]);
        return 0;
    }
    else if (strcmp(argv[1], "-w") == 0 && strcmp(argv[3], "-h") == 0 && strcmp(argv[5], "-m") == 0) {
        return 1;
    }*/
    create_map();
    initialize_gui(WINDOW_WIDTH,WINDOW_HEIGHT);
    while (should_continue) {
        draw_window();
        read_input();
    }
    // Dealloceer al het geheugen dat werd aangemaakt door SDL zelf.
    free_gui();
    return 0;
}