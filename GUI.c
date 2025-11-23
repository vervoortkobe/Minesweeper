#include <stdlib.h>
#include "GUI.h"
#include <SDL2/SDL.h>
#include <stdio.h>
#include <string.h>
#include "map.h"
#include <stdbool.h>
#include "filehandler.h"

/*
 * Deze renderer wordt gebruikt om figuren in het venster te tekenen. De renderer
 * wordt geïnitialiseerd in de initialize_window-functie.
 */
static SDL_Renderer *renderer;

// Instantieer de variabelen voor het bijhouden van de textures.
static SDL_Texture *digit_textures[9] = { NULL };
static SDL_Texture *digit_covered_texture = NULL;
static SDL_Texture *digit_flagged_texture = NULL;
static SDL_Texture *digit_mine_texture = NULL;

// Current window dimensions (set by initialize_window)
static int current_window_w = WINDOW_WIDTH;
static int current_window_h = WINDOW_HEIGHT;


static SDL_DisplayMode dm;
// Try candidate sizes then fall back to a computed size that fits the desktop.
int determine_img_win_size(int cols, int rows, int *out_image_size, int *out_window_w, int *out_window_h) {
    if (!out_image_size || !out_window_w || !out_window_h) return -1;
    if (cols <= 0 || rows <= 0) return -1;

    // Ensure SDL video subsystem is available to query desktop mode.
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        // SDL init failed; fall back to defaults
        *out_image_size = DEFAULT_IMAGE_SIZE;
        *out_window_w = cols * (*out_image_size);
        *out_window_h = rows * (*out_image_size);
        return 0;
    }

/*
* Deze functie berekent de display size van het gebruikte scherm.
* Op deze manier kunnen we de GUI in het midden van het scherm zetten bij het opstarten van het spel.
* Zie SDL2 documentatie: https://wiki.libsdl.org/SDL2/SDL_GetDesktopDisplayMode
*/
    if (SDL_GetDesktopDisplayMode(0, &dm) != 0) {
        *out_image_size = DEFAULT_IMAGE_SIZE;
        *out_window_w = cols * (*out_image_size);
        *out_window_h = rows * (*out_image_size);
        return 0;
    }

    int pref_sizes[] = {100, 75, 50};
    int chosen = -1;
    for (int i = 0; i < 3; ++i) {
        int s = pref_sizes[i];
        int ww = cols * s;
        int hh = rows * s;
        if (ww <= dm.w && hh <= dm.h) { chosen = s; break; }
    }
    if (chosen == -1) {
        // none of the presets fit; compute the max possible size that fits
        int max_w = dm.w / cols;
        int max_h = dm.h / rows;
        int s = max_w < max_h ? max_w : max_h;
        if (s < 20) s = 20; // enforce a small minimum
        chosen = s;
    }

    *out_image_size = chosen;
    *out_window_w = cols * chosen;
    *out_window_h = rows * chosen;

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

// Per-cell state stored in a linear Field array (see GUI.h)
static Field *fields = NULL;
static bool mines_placed = false; // of de mijnen reeds geplaatst zijn via add_mines
static bool show_mines = false; // via 'b' key
static bool game_lost = false;
static int losing_x = -1;
static int losing_y = -1;
static uint32_t lose_start_time = 0;
static bool game_won = false;
static uint32_t win_start_time = 0;
static uint32_t win_last_remove = 0;
static int win_remaining = 0;
static bool show_all = false; // via 'p' key

/* Field access macros (use COORD_IDX/map_w as width) */
#define FIELD_AT(c, r) (fields[(r) * map_w + (c)])
#define UNC(c, r) (FIELD_AT(c, r).uncovered)
#define FLAG(c, r) (FIELD_AT(c, r).flagged)
#define REMOVED(c, r) (FIELD_AT(c, r).removed)
#define SAVEDUNC(c, r) (FIELD_AT(c, r).saved_uncovered)

int alloc_game_states(void) {
    int cells = (int)map_w * (int)map_h;
    // maak de state buffers leeg als ze al bestaan
    free_game_states();
    fields = (Field*)calloc(cells, sizeof(Field));
    if (!fields) return -1;
    return 0;
}

// free als ze al bestaan
void free_game_states(void) {
    free(fields);
    fields = NULL;
}

// Bij elke interactie, wordt het speeldveld in de console geprint.
static void print_view() {
    for (int y = 0; y < map_h; ++y) {
        for (int x = 0; x < map_w; ++x) {
            if (UNC(x, y)) {
                putchar(MAP(x, y));
            } else if (FLAG(x, y)) {
                putchar('F');
            } else {
                putchar('#');
            }
            putchar(' ');
        }
        putchar('\n');
    }
    putchar('\n');
}

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

// forward declaration so read_input can call it
static void save_field_with_increment(void);

/*
 * Vangt de input uit de GUI op. Deze functie is al deels geïmplementeerd, maar je moet die
 * zelf nog afwerken.
 * Je mag natuurlijk alles aanpassen aan deze functie, inclusies return-type en argumenten.
 */
void read_input() {
    SDL_Event event;
    bool changed = false;
    int grid_rows = map_h, grid_cols = map_w;
    int cell_w = current_window_w / grid_cols;
    int cell_h = current_window_h / grid_rows;

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

    // Wanneer een game al gespeeld is (speler heeft al gewonnen/verloren), dan negeren we alle input en sluiten we het spel af.
    if ((game_lost || game_won) && event.type != SDL_QUIT) {
        return;
    }

    switch (event.type) {
    case SDL_KEYDOWN:
        if (event.key.keysym.sym == SDLK_p) {
            // Tijdelijk uncover alles via 'p' key
            show_all = !show_all;
            if (show_all) {
                // save previous uncovered state
                for (int y = 0; y < map_h; ++y) for (int x = 0; x < map_w; ++x) SAVEDUNC(x, y) = UNC(x, y);
                if (!mines_placed) {
                        // place mines without excluding any specific cell
                        add_mines(NULL);
                        print_map();
                        mines_placed = true;
                }
                for (int y = 0; y < map_h; ++y) {
                    for (int x = 0; x < map_w; ++x) {
                        UNC(x, y) = true;
                    }
                }
                printf("Show-all enabled (P pressed)\n");
            } else {
                // restore previous uncovered state
                for (int y = 0; y < map_h; ++y) for (int x = 0; x < map_w; ++x) UNC(x, y) = SAVEDUNC(x, y);
                printf("Show-all disabled (P pressed)\n");
            }
            changed = true;
        } else if (event.key.keysym.sym == SDLK_b) {
            show_mines = !show_mines;
            printf("Toggle show mines: %d\n", show_mines);
            // mark view changed so we print once after handling the event
            changed = true;
        } else if (event.key.keysym.sym == SDLK_s) {
            // Save the current generated field to an incrementally numbered file
            save_field_with_increment();
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

    int clicked_col = mouse_x / cell_w;
    int clicked_row = mouse_y / cell_h;

        if (clicked_col < 0) clicked_col = 0;
        if (clicked_col >= grid_cols) clicked_col = grid_cols - 1;
        if (clicked_row < 0) clicked_row = 0;
        if (clicked_row >= grid_rows) clicked_row = grid_rows - 1;

        if (event.button.button == SDL_BUTTON_RIGHT) {
            FLAG(clicked_col, clicked_row) = !FLAG(clicked_col, clicked_row);
            printf("Right-click at (%d, %d) -> cell (%d, %d) flag=%d\n", mouse_x, mouse_y, clicked_row, clicked_col, (int)FLAG(clicked_col, clicked_row));
            // mark view changed so we print once after handling the event
            changed = true;
            // check win: all mines flagged and no incorrect flags
            int correct_flags = 0;
            int total_flags = 0;
            for (int y = 0; y < map_h; ++y) {
                for (int x = 0; x < map_w; ++x) {
                    if (FLAG(x, y)) {
                        total_flags++;
                        if (MAP(x, y) == 'M') correct_flags++;
                    }
                }
            }
            if (total_flags == map_mines && correct_flags == map_mines) {
                printf("All mines flagged - you win!\n");
                // start win animation instead of immediately closing
                game_won = true;
                win_start_time = SDL_GetTicks();
                win_last_remove = win_start_time;
                // initialize removed map and remaining count
                win_remaining = map_w * map_h;
                for (int y = 0; y < map_h; ++y) {
                    for (int x = 0; x < map_w; ++x) {
                        REMOVED(x, y) = false;
                    }
                }
                // seed rand for the removal ordering
                srand((unsigned int)SDL_GetTicks());
                changed = true;
            }
        } else {
            printf("Clicked at (%d, %d) -> cell (%d, %d)\n", mouse_x, mouse_y, clicked_row, clicked_col);
            if (!mines_placed) {
                Coord exclude = { .x = clicked_col, .y = clicked_row };
                add_mines(&exclude);
                print_map();
                printf("\n");
                mines_placed = true;
                changed = true;
            }
            if (MAP(clicked_col, clicked_row) == 'M') {
                // speler klikte op een mijn -> game over
                if (!game_lost) {
                    game_lost = true;
                    losing_x = clicked_col;
                    losing_y = clicked_row;
                    lose_start_time = SDL_GetTicks();
                    // toon alle mijnen
                    for (int y = 0; y < map_h; ++y) {
                        for (int x = 0; x < map_w; ++x) {
                            if (MAP(x, y) == 'M') UNC(x, y) = true;
                        }
                    }
                    // uncover ook de mijn waarop geklikt werd
                    UNC(losing_x, losing_y) = true;
                    printf("Boom! You clicked a mine at (%d, %d) - you lose.\n", losing_x, losing_y);
                    changed = true;
                }
            } else if (MAP(clicked_col, clicked_row) == '0') {
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
                    if (UNC(cx, cy)) continue;
                    UNC(cx, cy) = true;
                    changed = true;
                    if (MAP(cx, cy) == '0') {
                        for (int dy = -1; dy <= 1; dy++) {
                            for (int dx = -1; dx <= 1; dx++) {
                                if (dx == 0 && dy == 0) continue;
                                int nx = cx + dx;
                                int ny = cy + dy;
                                if (nx < 0 || nx >= map_w || ny < 0 || ny >= map_h) continue;
                                if (!UNC(nx, ny) && !FLAG(nx, ny)) {
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
                if (!UNC(clicked_col, clicked_row)) {
                    UNC(clicked_col, clicked_row) = true;
                    changed = true;
                }
            }
        }
        break;
    }

    if (changed) {
        print_view();
    }

    return;
}

void draw_window() {
    int grid_rows = map_h, grid_cols = map_w;
    int cell_w = current_window_w / grid_cols;
    int cell_h = current_window_h / grid_rows;

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderClear(renderer);

    if (!game_won && !game_lost) {
        int marker_col = mouse_x / cell_w;
        int marker_row = mouse_y / cell_h;
        if (marker_col >= 0 && marker_col < grid_cols && marker_row >= 0 && marker_row < grid_rows) {
            SDL_Rect marker_rect = { marker_col * cell_w, marker_row * cell_h, cell_w, cell_h };
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer, 255, 255, 0, 100);
            SDL_RenderFillRect(renderer, &marker_rect);
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        }
    }

    for (int row = 0; row < grid_rows; ++row) {
        for (int col = 0; col < grid_cols; ++col) {
            SDL_Rect rect = { col * cell_w, row * cell_h, cell_w, cell_h };
            // During win animation, removed cells disappear; otherwise render normally
                if (game_won && REMOVED(col, row)) {
                continue; // disappeared
            }

            // If user asked to show mines, draw them regardless of uncovered state
            if (show_mines && MAP(col, row) == 'M') {
                // if win animation is active and cell has been removed, skip drawing
                    if (game_won && REMOVED(col, row)) continue;
                SDL_RenderCopy(renderer, digit_mine_texture, NULL, &rect);
                continue;
            }

            if (UNC(col, row)) {
                char c = MAP(col, row);
                if (c == 'M') {
                    // If player lost and this is the losing mine, make it blink red every second
                    if (game_lost && row == losing_y && col == losing_x) {
                        uint32_t t = SDL_GetTicks();
                        uint32_t elapsed = t - lose_start_time;
                        int visible = ((elapsed / 1000) % 2) == 0; // blink every second relative to loss time
                        if (visible) {
                            // tint red for the losing mine
                            SDL_SetTextureColorMod(digit_mine_texture, 255, 0, 0);
                            SDL_RenderCopy(renderer, digit_mine_texture, NULL, &rect);
                            // reset tint
                            SDL_SetTextureColorMod(digit_mine_texture, 255, 255, 255);
                        } else {
                            // when not visible, draw the covered texture so the marker or previous frames are hidden
                            if (digit_covered_texture) SDL_RenderCopy(renderer, digit_covered_texture, NULL, &rect);
                        }
                    } else {
                        SDL_RenderCopy(renderer, digit_mine_texture, NULL, &rect);
                    }
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
                    if (FLAG(col, row) && digit_flagged_texture) {
                    SDL_RenderCopy(renderer, digit_flagged_texture, NULL, &rect);
                } else if (digit_covered_texture) {
                    SDL_RenderCopy(renderer, digit_covered_texture, NULL, &rect);
                }
            }
        }
    }

    // If win animation running, remove a random non-removed cell periodically
    if (game_won && win_remaining > 0) {
        uint32_t now = SDL_GetTicks();
        if (now - win_last_remove >= 20) {
            // pick a random remaining cell
            int tries = 0;
            while (tries < 1000 && win_remaining > 0) {
                int rx = rand() % map_w;
                int ry = rand() % map_h;
                if (!REMOVED(rx, ry)) {
                    REMOVED(rx, ry) = true;
                    win_remaining--;
                    break;
                }
                tries++;
            }
            win_last_remove = now;
        }
        // when all removed, close the game
        if (win_remaining <= 0) {
            should_continue = 0;
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
        /* store current window size for layout calculations */
        current_window_w = window_width;
        current_window_h = window_height;
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

// Save current map to an incrementally numbered file: field_<width>x<height>_<n>.txt
static void save_field_with_increment() {
    char filename[256];
    int n = 1;
    while (1) {
        snprintf(filename, sizeof(filename), "field_%dx%d_%d.txt", map_w, map_h, n);
        FILE *f = fopen(filename, "r");
        if (f) {
            fclose(f);
            n++;
            continue;
        }
        break;
    }
    char filenamebuf[256];
    snprintf(filenamebuf, sizeof(filenamebuf), "field_%dx%d_%d.txt", map_w, map_h, n);
    /* build temporary flagged/uncovered arrays as unsigned char arrays for filehandler API */
    int cells = (int)map_w * (int)map_h;
    unsigned char *f_arr = (unsigned char*)malloc(cells);
    unsigned char *u_arr = (unsigned char*)malloc(cells);
    if (!f_arr || !u_arr) {
        if (f_arr) free(f_arr);
        if (u_arr) free(u_arr);
        perror("Out of memory while saving");
        return;
    }
    for (int y = 0; y < map_h; ++y) for (int x = 0; x < map_w; ++x) {
        int idx = y * map_w + x;
        f_arr[idx] = FLAG(x, y) ? 1 : 0;
        u_arr[idx] = UNC(x, y) ? 1 : 0;
    }
    if (save_field(filenamebuf, map_w, map_h, map, f_arr, u_arr) != 0) {
        fprintf(stderr, "Error saving field to %s\n", filenamebuf);
    } else {
        printf("Saved field and play state to %s\n", filenamebuf);
    }
    free(f_arr);
    free(u_arr);
}

// forward declaration for save helper used in read_input
static void save_field_with_increment(void);

// Load a game file which may contain an optional state block separated by a blank line.
//  - map + state: same as above, then a blank line, then rows of state tokens per cell: U=uncovered, F=flagged, #=covered
int load_file(const char *filename) {
    char **lines = NULL;
    int rows = 0;
    if (read_lines(filename, &lines, &rows) != 0) return -1;
    if (rows == 0) { free_lines(lines, rows); return -1; }
    int sep = -1;
    for (int i = 0; i < rows; ++i) if (lines[i][0] == '\0') { sep = i; break; }
    int map_rows = (sep == -1) ? rows : sep;
    int cols = 0;
    char *tmp = strdup(lines[0]);
    char *p = strtok(tmp, " \t");
    while (p) { cols++; p = strtok(NULL, " \t"); }
    free(tmp);
    if (cols <= 0) { free_lines(lines, rows); return -1; }
    if (init_map(cols, map_rows, 0) != 0) { free_lines(lines, rows); return -1; }
    int mines = 0;
    for (int y = 0; y < map_rows; ++y) {
        int c = 0;
        char *tok = strtok(lines[y], " \t");
        while (tok && c < cols) {
            MAP(c, y) = tok[0];
            if (MAP(c, y) == 'M') mines++;
            c++;
            tok = strtok(NULL, " \t");
        }
    }
    map_mines = mines;
    if (alloc_game_states() != 0) { free_lines(lines, rows); free_map(); return -1; }
    for (int y = 0; y < map_h; ++y) for (int x = 0; x < map_w; ++x) { UNC(x, y) = false; FLAG(x, y) = false; }
    if (sep != -1) {
        int state_rows = rows - (sep + 1);
        int use_rows = state_rows < map_rows ? state_rows : map_rows;
        for (int y = 0; y < use_rows; ++y) {
            int c = 0;
            char *tok = strtok(lines[sep + 1 + y], " \t");
            while (tok && c < cols) {
                if (tok[0] == 'F') FLAG(c, y) = true;
                else if (tok[0] == 'U') UNC(c, y) = true;
                c++;
                tok = strtok(NULL, " \t");
            }
        }
    }
    free_lines(lines, rows);
    mines_placed = true;
    return 0;
}