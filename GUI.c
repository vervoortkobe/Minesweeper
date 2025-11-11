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

// Game state for GUI: covered/uncovered/flagged (dynamic, sized to map_w x map_h)
static bool *uncovered = NULL;
static bool *flagged = NULL;
static bool mines_placed = false; // whether add_mines_excluding has been called
static bool show_bombs = false; // toggled with key 'b'
static bool game_lost = false;
static int losing_x = -1;
static int losing_y = -1;
static uint32_t lose_start_time = 0;
static bool game_won = false;
static bool *removed_cells = NULL;
static uint32_t win_start_time = 0;
static uint32_t win_last_remove = 0;
static int win_remaining = 0;
static const uint32_t WIN_REMOVE_INTERVAL_MS = 30; // remove one box every 30ms (faster)
static bool show_all = false; // toggled with key 'p'
static bool *saved_uncovered = NULL;

/* access helpers */
#define UNC(r,c) (uncovered[(r) * map_w + (c)])
#define FLAG(r,c) (flagged[(r) * map_w + (c)])
#define REMOVED(r,c) (removed_cells[(r) * map_w + (c)])
#define SAVEDUNC(r,c) (saved_uncovered[(r) * map_w + (c)])

static int alloc_state_buffers(void) {
    size_t cells = (size_t)map_w * (size_t)map_h;
    // free existing
    if (uncovered) free(uncovered);
    if (flagged) free(flagged);
    if (removed_cells) free(removed_cells);
    if (saved_uncovered) free(saved_uncovered);
    uncovered = (bool*)calloc(cells, sizeof(bool));
    flagged = (bool*)calloc(cells, sizeof(bool));
    removed_cells = (bool*)calloc(cells, sizeof(bool));
    saved_uncovered = (bool*)calloc(cells, sizeof(bool));
    if (!uncovered || !flagged || !removed_cells || !saved_uncovered) return -1;
    return 0;
}

static void free_state_buffers(void) {
    if (uncovered) { free(uncovered); uncovered = NULL; }
    if (flagged) { free(flagged); flagged = NULL; }
    if (removed_cells) { free(removed_cells); removed_cells = NULL; }
    if (saved_uncovered) { free(saved_uncovered); saved_uncovered = NULL; }
}

// Print the current visible view of the board to stdout:
// '#' = covered, 'F' = flagged, numbers or 'M' for uncovered
static void print_view() {
    for (int y = 0; y < map_h; ++y) {
        for (int x = 0; x < map_w; ++x) {
            if (UNC(y,x)) {
                putchar(MAP(y,x));
            } else if (FLAG(y,x)) {
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
// forward declaration for loader that restores playing state
static int load_game_file(const char *filename);


/*
 * Vangt de input uit de GUI op. Deze functie is al deels geïmplementeerd, maar je moet die
 * zelf nog afwerken.
 * Je mag natuurlijk alles aanpassen aan deze functie, inclusies return-type en argumenten.
 */
void read_input() {
    SDL_Event event;
    bool changed = false;
    int grid_rows = map_h, grid_cols = map_w;
    int cell_w = WINDOW_WIDTH / grid_cols;
    int cell_h = WINDOW_HEIGHT / grid_rows;

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

    // If player already lost or won, ignore all inputs except quitting so animations can continue
    if ((game_lost || game_won) && event.type != SDL_QUIT) {
        return;
    }

    switch (event.type) {
    case SDL_KEYDOWN:
        if (event.key.keysym.sym == SDLK_p) {
            // Toggle reveal-all on the board (temporary reveal)
            show_all = !show_all;
            if (show_all) {
                // save previous uncovered state
                for (int y = 0; y < map_h; ++y) for (int x = 0; x < map_w; ++x) SAVEDUNC(y,x) = UNC(y,x);
                if (!mines_placed) {
                    // place mines without excluding any specific cell
                    add_mines_excluding(-1, -1);
                    print_map();
                    export_map();
                    mines_placed = true;
                }
                for (int y = 0; y < map_h; ++y) {
                    for (int x = 0; x < map_w; ++x) {
                        UNC(y,x) = true;
                    }
                }
                printf("Show-all enabled (P pressed)\n");
            } else {
                // restore previous uncovered state
                for (int y = 0; y < map_h; ++y) for (int x = 0; x < map_w; ++x) UNC(y,x) = SAVEDUNC(y,x);
                printf("Show-all disabled (P pressed)\n");
            }
            changed = true;
        } else if (event.key.keysym.sym == SDLK_b) {
            show_bombs = !show_bombs;
            printf("Toggle show bombs: %d\n", show_bombs);
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

    int grid_rows = map_h, grid_cols = map_w;
    int cell_w = WINDOW_WIDTH / grid_cols;
    int cell_h = WINDOW_HEIGHT / grid_rows;

        int clicked_col = mouse_x / cell_w;
        int clicked_row = mouse_y / cell_h;

        if (clicked_col < 0) clicked_col = 0;
        if (clicked_col >= grid_cols) clicked_col = grid_cols - 1;
        if (clicked_row < 0) clicked_row = 0;
        if (clicked_row >= grid_rows) clicked_row = grid_rows - 1;

        if (event.button.button == SDL_BUTTON_RIGHT) {
            FLAG(clicked_row,clicked_col) = !FLAG(clicked_row,clicked_col);
            printf("Right-click at (%d,%d) -> cell (%d,%d) flag=%d\n", mouse_x, mouse_y, clicked_row, clicked_col, (int)FLAG(clicked_row,clicked_col));
            // mark view changed so we print once after handling the event
            changed = true;
            // check win: all mines flagged and no incorrect flags
            int correct_flags = 0;
            int total_flags = 0;
            for (int y = 0; y < map_h; ++y) {
                for (int x = 0; x < map_w; ++x) {
                    if (FLAG(y,x)) {
                        total_flags++;
                        if (MAP(y,x) == 'M') correct_flags++;
                    }
                }
            }
            if (total_flags == map_mines && correct_flags == map_mines) {
                printf("All bombs flagged - you win!\n");
                // start win animation instead of immediately closing
                game_won = true;
                win_start_time = SDL_GetTicks();
                win_last_remove = win_start_time;
                // initialize removed map and remaining count
                win_remaining = map_w * map_h;
                for (int y = 0; y < map_h; ++y) {
                    for (int x = 0; x < map_w; ++x) {
                        REMOVED(y,x) = false;
                    }
                }
                // seed rand for the removal ordering
                srand((unsigned int)SDL_GetTicks());
                changed = true;
            }
        } else {
            printf("Clicked at (%d,%d) -> cell (%d,%d)\n", mouse_x, mouse_y, clicked_row, clicked_col);
            if (!mines_placed) {
                add_mines_excluding(clicked_col, clicked_row);
                print_map();
                export_map();
                mines_placed = true;
                changed = true;
            }
            if (MAP(clicked_row,clicked_col) == 'M') {
                // player clicked a mine -> lose
                if (!game_lost) {
                    game_lost = true;
                    losing_x = clicked_col;
                    losing_y = clicked_row;
                    lose_start_time = SDL_GetTicks();
                    // reveal all mines
                    for (int y = 0; y < map_h; ++y) {
                        for (int x = 0; x < map_w; ++x) {
                            if (MAP(y,x) == 'M') UNC(y,x) = true;
                        }
                    }
                    // ensure losing mine is uncovered
                    UNC(losing_y,losing_x) = true;
                    printf("Boom! You clicked a mine at (%d,%d) - you lose.\n", losing_y, losing_x);
                    changed = true;
                }
            } else if (MAP(clicked_row,clicked_col) == '0') {
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
                    if (UNC(cy,cx)) continue;
                    UNC(cy,cx) = true;
                    changed = true;
                    if (MAP(cy,cx) == '0') {
                        for (int dy = -1; dy <= 1; dy++) {
                            for (int dx = -1; dx <= 1; dx++) {
                                if (dx == 0 && dy == 0) continue;
                                int nx = cx + dx;
                                int ny = cy + dy;
                                if (nx < 0 || nx >= map_w || ny < 0 || ny >= map_h) continue;
                                if (!UNC(ny,nx) && !FLAG(ny,nx)) {
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
                if (!UNC(clicked_row,clicked_col)) {
                    UNC(clicked_row,clicked_col) = true;
                    changed = true;
                }
            }
            /* view changes are tracked while handling the click; print once below if changed */
        }
        break;
    }

    if (changed) {
        print_view();
    }

    return;
}

void draw_window() {
    // Draw a translucent marker on the cell under the mouse (only when not in end-state)
    int grid_rows = map_h, grid_cols = map_w;
    int cell_w = WINDOW_WIDTH / grid_cols;
    int cell_h = WINDOW_HEIGHT / grid_rows;

    // Clear background
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
                if (game_won && REMOVED(row,col)) {
                continue; // disappeared
            }

            // If user asked to show bombs, draw them regardless of uncovered state
            if (show_bombs && MAP(row,col) == 'M') {
                // if win animation is active and cell has been removed, skip drawing
                    if (game_won && REMOVED(row,col)) continue;
                SDL_RenderCopy(renderer, digit_mine_texture, NULL, &rect);
                continue;
            }

            if (UNC(row,col)) {
                char c = MAP(row,col);
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
                    if (FLAG(row,col) && digit_flagged_texture) {
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
        if (now - win_last_remove >= WIN_REMOVE_INTERVAL_MS) {
            // pick a random remaining cell
            int tries = 0;
            while (tries < 1000 && win_remaining > 0) {
                int rx = rand() % map_w;
                int ry = rand() % map_h;
                if (!REMOVED(ry,rx)) {
                    REMOVED(ry,rx) = true;
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
    /* Command-line modes supported:
     * -f <file>         Load saved map from file and continue
     * -w <width> -h <height> -m <mines>
     *                   Create a new map with given width/height/mines (flags may appear in any order)
     */
    const char *load_file = NULL;
    int w = -1, h = -1, m = -1;

    for (int i = 1; i < argc; ++i) {
        char *arg = argv[i];
        if (arg[0] != '-') {
            fprintf(stderr, "Unknown argument: %s\n", arg);
            return 1;
        }
        switch (arg[1]) {
            case 'f':
                if (strcmp(arg, "-f") != 0) { fprintf(stderr, "Unknown argument: %s\n", arg); return 1; }
                if (i + 1 < argc) { load_file = argv[++i]; } else { fprintf(stderr, "Missing filename after -f\n"); return 1; }
                break;
            case 'w':
                if (strcmp(arg, "-w") != 0) { fprintf(stderr, "Unknown argument: %s\n", arg); return 1; }
                if (i + 1 < argc) { w = atoi(argv[++i]); } else { fprintf(stderr, "Missing width after -w\n"); return 1; }
                break;
            case 'h':
                if (strcmp(arg, "-h") != 0) { fprintf(stderr, "Unknown argument: %s\n", arg); return 1; }
                if (i + 1 < argc) { h = atoi(argv[++i]); } else { fprintf(stderr, "Missing height after -h\n"); return 1; }
                break;
            case 'm':
                if (strcmp(arg, "-m") != 0) { fprintf(stderr, "Unknown argument: %s\n", arg); return 1; }
                if (i + 1 < argc) { m = atoi(argv[++i]); } else { fprintf(stderr, "Missing mines after -m\n"); return 1; }
                break;
            default:
                fprintf(stderr, "Unknown argument: %s\n", arg);
                return 1;
        }
    }

    /* disallow combining -f with new-map flags */
    if (load_file && (w != -1 || h != -1 || m != -1)) {
        fprintf(stderr, "Cannot combine -f with -w/-h/-m options\n");
        return 1;
    }

    if (load_file) {
        if (load_game_file(load_file) != 0) {
            fprintf(stderr, "Failed to load map from %s\n", load_file);
            return 1;
        }
    } else if (w > 0 && h > 0 && m >= 0) {
        if (init_map(w, h, m) != 0) {
            fprintf(stderr, "Failed to initialize map %dx%d\n", w, h);
            return 1;
        }
        create_map();
        /* allocate GUI state buffers for the requested dimensions */
        if (alloc_state_buffers() != 0) {
            fprintf(stderr, "Failed to allocate GUI state buffers\n");
            return 1;
        }
    } else {
        // default
        create_map();
        /* ensure state buffers exist for the default map */
        if (alloc_state_buffers() != 0) {
            fprintf(stderr, "Failed to allocate GUI state buffers\n");
            return 1;
        }
    }

    initialize_gui(WINDOW_WIDTH,WINDOW_HEIGHT);
    while (should_continue) {
        draw_window();
        read_input();
    }
    // Dealloceer al het geheugen dat werd aangemaakt door SDL zelf.
    free_gui();
    free_map();
    return 0;
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
    /* Write a single file: field_<w>x<h>_<n>.txt
     * Format: map tokens (rows), blank line, state tokens per cell: U=uncovered, F=flagged, #=covered
     */
    char filenamebuf[256];
    snprintf(filenamebuf, sizeof(filenamebuf), "field_%dx%d_%d.txt", map_w, map_h, n);

    FILE *out = fopen(filenamebuf, "w");
    if (!out) {
        perror("Error creating save file");
        return;
    }

    for (int y = 0; y < map_h; ++y) {
        for (int x = 0; x < map_w; ++x) {
            fprintf(out, "%c ", MAP(y,x));
        }
        fprintf(out, "\n");
    }
    /* separator between map and state */
    fprintf(out, "\n");
    for (int y = 0; y < map_h; ++y) {
        for (int x = 0; x < map_w; ++x) {
            if (FLAG(y,x)) fprintf(out, "F ");
            else if (UNC(y,x)) fprintf(out, "U ");
            else fprintf(out, "# ");
        }
        fprintf(out, "\n");
    }

    fclose(out);
    printf("Saved field and play state to %s\n", filenamebuf);
}

// forward declaration for save helper used in read_input
static void save_field_with_increment(void);
// forward declaration for loader that restores playing state
static int load_game_file(const char *filename);

// Load a game file which may contain an optional state block separated by a blank line.
// Format supported:
//  - map only: rows of tokens (M/0..8) separated by spaces
//  - map + state: same as above, then a blank line, then rows of state tokens per cell: U=uncovered, F=flagged, #=covered
static int load_game_file(const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) return -1;
    char line[4096];
    char *lines[4096];
    int rows = 0;
    while (fgets(line, sizeof(line), f)) {
        size_t len = strlen(line);
        while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r')) { line[--len] = '\0'; }
        lines[rows] = strdup(line);
        rows++;
        if (rows >= 4096) break;
    }
    fclose(f);
    if (rows == 0) return -1;
    // find separator (first empty line)
    int sep = -1;
    for (int i = 0; i < rows; ++i) {
        if (lines[i][0] == '\0') { sep = i; break; }
    }
    int map_rows = (sep == -1) ? rows : sep;
    // determine columns by splitting first non-empty line
    int cols = 0;
    char *tmp = strdup(lines[0]);
    char *p = strtok(tmp, " \t");
    while (p) { cols++; p = strtok(NULL, " \t"); }
    free(tmp);
    if (cols <= 0) goto fail;
    if (init_map(cols, map_rows, 0) != 0) goto fail;
    // fill map and count mines
    int mines = 0;
    for (int y = 0; y < map_rows; ++y) {
        int c = 0;
        char *token = strtok(lines[y], " \t");
        while (token && c < cols) {
            MAP(y,c) = token[0];
            if (MAP(y,c) == 'M') mines++;
            c++;
            token = strtok(NULL, " \t");
        }
    }
    map_mines = mines;
    // allocate GUI state buffers
    if (alloc_state_buffers() != 0) goto fail;
    // default: all covered and unflagged
    for (int y = 0; y < map_h; ++y) for (int x = 0; x < map_w; ++x) { UNC(y,x) = false; FLAG(y,x) = false; }
    // if there's a state block, parse it
    if (sep != -1) {
        int state_rows = rows - (sep + 1);
        int use_rows = state_rows < map_rows ? state_rows : map_rows;
        for (int y = 0; y < use_rows; ++y) {
            int c = 0;
            char *token = strtok(lines[sep + 1 + y], " \t");
            while (token && c < cols) {
                if (token[0] == 'F') FLAG(y,c) = true;
                else if (token[0] == 'U') UNC(y,c) = true;
                else { /* covered */ }
                c++;
                token = strtok(NULL, " \t");
            }
        }
    }
    // clean up lines
    for (int i = 0; i < rows; ++i) if (lines[i]) free(lines[i]);
    mines_placed = true;
    return 0;
fail:
    for (int i = 0; i < rows; ++i) if (lines[i]) free(lines[i]);
    return -1;
}