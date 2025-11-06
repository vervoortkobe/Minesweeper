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
static bool game_lost = false;
static int losing_x = -1;
static int losing_y = -1;
static uint32_t lose_start_time = 0;
static bool game_won = false;
static bool removed_cells[10][10];
static uint32_t win_start_time = 0;
static uint32_t win_last_remove = 0;
static int win_remaining = 0;
static const uint32_t WIN_REMOVE_INTERVAL_MS = 30; // remove one box every 30ms (faster)

// Print the current visible view of the board to stdout:
// '#' = covered, 'F' = flagged, numbers or 'M' for uncovered
static void print_view() {
    for (int y = 0; y < map_h; ++y) {
        for (int x = 0; x < map_w; ++x) {
            if (uncovered[y][x]) {
                putchar(map[y][x]);
            } else if (flagged[y][x]) {
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

/*
 * Vangt de input uit de GUI op. Deze functie is al deels geïmplementeerd, maar je moet die
 * zelf nog afwerken.
 * Je mag natuurlijk alles aanpassen aan deze functie, inclusies return-type en argumenten.
 */
void read_input() {
    SDL_Event event;
    bool changed = false;
    int grid_rows = 10, grid_cols = 10;
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
            printf("P pressed\n");
        } else if (event.key.keysym.sym == SDLK_b) {
            show_bombs = !show_bombs;
            printf("Toggle show bombs: %d\n", show_bombs);
            // mark view changed so we print once after handling the event
            changed = true;
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
            // mark view changed so we print once after handling the event
            changed = true;
            // check win: all mines flagged and no incorrect flags
            int correct_flags = 0;
            int total_flags = 0;
            for (int y = 0; y < map_h; ++y) {
                for (int x = 0; x < map_w; ++x) {
                    if (flagged[y][x]) {
                        total_flags++;
                        if (map[y][x] == 'M') correct_flags++;
                    }
                }
            }
            if (total_flags == map_mines && correct_flags == map_mines) {
                printf("All bombs flagged — you win!\n");
                // start win animation instead of immediately closing
                game_won = true;
                win_start_time = SDL_GetTicks();
                win_last_remove = win_start_time;
                // initialize removed map and remaining count
                win_remaining = map_w * map_h;
                for (int y = 0; y < map_h; ++y) {
                    for (int x = 0; x < map_w; ++x) {
                        removed_cells[y][x] = false;
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
            if (map[clicked_row][clicked_col] == 'M') {
                // player clicked a mine -> lose
                if (!game_lost) {
                    game_lost = true;
                    losing_x = clicked_col;
                    losing_y = clicked_row;
                    lose_start_time = SDL_GetTicks();
                    // reveal all mines
                    for (int y = 0; y < map_h; ++y) {
                        for (int x = 0; x < map_w; ++x) {
                            if (map[y][x] == 'M') uncovered[y][x] = true;
                        }
                    }
                    // ensure losing mine is uncovered
                    uncovered[losing_y][losing_x] = true;
                    printf("Boom! You clicked a mine at (%d,%d) — you lose.\n", losing_y, losing_x);
                    changed = true;
                }
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
                    changed = true;
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
                if (!uncovered[clicked_row][clicked_col]) {
                    uncovered[clicked_row][clicked_col] = true;
                    changed = true;
                }
            }
            /* view changes are tracked while handling the click; print once below if changed */
        }
        break;
    }

        return;
    }

void draw_window() {
    // Draw a translucent marker on the cell under the mouse (only when not in end-state)
    int grid_rows = 10, grid_cols = 10;
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
            if (game_won && removed_cells[row][col]) {
                continue; // disappeared
            }

            // If user asked to show bombs, draw them regardless of uncovered state
            if (show_bombs && map[row][col] == 'M') {
                // if win animation is active and cell has been removed, skip drawing
                if (game_won && removed_cells[row][col]) continue;
                SDL_RenderCopy(renderer, digit_mine_texture, NULL, &rect);
                continue;
            }

            if (uncovered[row][col]) {
                char c = map[row][col];
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
                if (flagged[row][col] && digit_flagged_texture) {
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
                if (!removed_cells[ry][rx]) {
                    removed_cells[ry][rx] = true;
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