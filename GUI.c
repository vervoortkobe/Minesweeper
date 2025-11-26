#include <stdlib.h>
#include "GUI.h"
#include <SDL2/SDL.h>
#include <stdio.h>
#include <string.h>
#include "map.h"
#include <stdbool.h>
#include "filehandler.h"

/*
 * Deze renderer wordt gebruikt om figuren in het venster te tekenen.
 * De renderer wordt geïnitialiseerd in de initialize_window-functie.
 */
static SDL_Renderer *renderer;

// Instantieer de variabelen voor het bijhouden van de textures.
static SDL_Texture *digit_textures[9] = {NULL};
static SDL_Texture *digit_covered_texture = NULL;
static SDL_Texture *digit_flagged_texture = NULL;
static SDL_Texture *digit_mine_texture = NULL;

// Instantieer de variabele voor het bijhouden van de display mode.
static SDL_DisplayMode dm;

// Standaard window afmetingen
static int curr_win_w = WINDOW_WIDTH;
static int curr_win_h = WINDOW_HEIGHT;

/*
 * Deze functie bepaalt de beste window en image size op basis van het aantal kolommen en rijen.
 * Het resultaat wordt via de out-parameters teruggegeven.
 */
int determine_img_win_size(int cols, int rows, int *out_img_size, int *out_win_w, int *out_win_h)
{
    if (!out_img_size || !out_win_w || !out_win_h)
        return -1;
    if (cols <= 0 || rows <= 0)
        return -1;

    /*
     * Code o.a. gebaseerd op:
     * http://lazyfoo.net/tutorials/SDL/02_getting_an_image_on_the_screen/index.php
     */
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        printf("Could not initialize SDL: %s\n", SDL_GetError());
        return 1;
    }

    if (SDL_GetDesktopDisplayMode(0, &dm) != 0)
    {
        *out_img_size = DEFAULT_IMAGE_SIZE;
        *out_win_w = cols * (*out_img_size);
        *out_win_h = rows * (*out_img_size);
        return 0;
    }

    // voorgestelde afmetingen voor afbeeldingen
    int pref_sizes[] = {100, 75, 50};
    int chosen = -1;
    for (int i = 0; i < 3; ++i)
    {
        int size = pref_sizes[i];
        // window afmetingen worden berekend d.m.v. aantal kolommen/rijen * image size
        int win_width = cols * size;
        int win_height = rows * size;
        if (win_width <= dm.w && win_height <= dm.h)
        {
            chosen = size;
            break;
        }
    }
    if (chosen == -1)
    {
        // Bereken de maximale window grootte, afhankelijk van het scherm
        int max_w = dm.w / cols;
        int max_h = dm.h / rows;
        int size = max_w < max_h ? max_w : max_h;
        if (size < 20)
            size = 20; // minimale grootte
        chosen = size;
    }

    *out_img_size = chosen;
    *out_win_w = cols * chosen;
    *out_win_h = rows * chosen;

    return 0;
}

/*
 * Onderstaande twee lijnen maken deel uit van de minimalistische voorbeeldapplicatie:
 * ze houden de laatste positie bij waar de gebruiker geklikt heeft.
 */
int mouse_x = 0;
int mouse_y = 0;

/*
 * Deze variabele geeft aan of de applicatie moet verdergaan.
 * Dit is true/1 zolang de gebruiker de applicatie niet wilt afsluiten door op het kruisje te klikken.
 */
int should_continue = 1;

/*
 * Dit is het venster dat getoond zal worden en waarin het speelveld weergegeven wordt.
 * Dit venster wordt aangemaakt bij het initialiseren van de GUI en wordt weer afgebroken wanneer het spel ten einde komt.
 */
static SDL_Window *window;

// benodigde game state variabelen
bool mines_placed = false;      // of de mijnen reeds geplaatst zijn via add_mines
static bool show_mines = false; // via 'b' key
static bool game_lost = false;
static Coord losing_coord = {-1, -1};
static uint32_t lose_start_time = 0;
static bool game_won = false;
static int win_remaining = 0;
static uint32_t win_last_remove = 0;
static bool show_all = false; // via 'p' key

int alloc_game_states(void)
{
    // Initialize all Cell state fields to false
    for (int y = 0; y < map_h; ++y)
    {
        for (int x = 0; x < map_w; ++x)
        {
            Coord c = coord_make(x, y);
            cell_set_uncovered(c, false);
            cell_set_flagged(c, false);
            cell_set_removed(c, false);
            cell_set_saved_uncovered(c, false);
        }
    }
    return 0;
}

// Game states are now part of Cell, so nothing to free
void free_game_states(void)
{
    // Cell states are freed when map is freed
}

// Bij elke interactie, wordt het speeldveld in de console geprint.
static void print_view()
{
    for (int y = 0; y < map_h; ++y)
    {
        for (int x = 0; x < map_w; ++x)
        {
            Coord c = coord_make(x, y);
            if (cell_is_uncovered(c))
            {
                if (MAP_AT(c).is_mine)
                    putchar('M');
                else
                    putchar('0' + MAP_AT(c).neighbour_mines);
            }
            else if (cell_is_flagged(c))
            {
                putchar('F');
            }
            else if (show_mines && MAP_AT(c).is_mine)
            {
                putchar('M');
            }
            else
            {
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
 * We gebruiken in deze GUI enkel muiskliks, toetsdrukken, en de "Quit" van het venster, dus enkel deze soorten events zijn "relevant".
 */
static int is_relevant_event(SDL_Event *event)
{
    if (event == NULL)
    {
        return 0;
    }
    return (event->type == SDL_MOUSEBUTTONDOWN) ||
           (event->type == SDL_KEYDOWN) ||
           (event->type == SDL_QUIT);
}

// Deze functie vangt de input uit de GUI op (muiskliks en het indrukken van toetsen).
void read_input()
{
    SDL_Event event;
    bool changed = false;
    int grid_rows = map_h, grid_cols = map_w;
    int cell_w = curr_win_w / grid_cols;
    int cell_h = curr_win_h / grid_rows;

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
    while (1)
    {
        int event_polled = SDL_PollEvent(&event);
        if (event_polled == 0)
        {
            return;
        }
        else if (is_relevant_event(&event))
        {
            break;
        }
    }

    // Wanneer een game al gespeeld is (speler heeft al gewonnen/verloren), dan negeren we alle input en sluiten we het spel af.
    if ((game_lost || game_won) && event.type != SDL_QUIT)
    {
        return;
    }

    switch (event.type)
    {
    case SDL_KEYDOWN:
        if (event.key.keysym.sym == SDLK_p)
        {
            // Tijdelijk uncover alles via 'p' key
            show_all = !show_all;
            printf("Toggle show_all: %d\n", show_all);
            if (show_all)
            {
                // sla tijdelijk vorige uncovered state op
                for (int x = 0; x < map_w; ++x)
                    for (int y = 0; y < map_h; ++y)
                    {
                        Coord c = coord_make(x, y);
                        cell_set_saved_uncovered(c, cell_is_uncovered(c));
                    }
                // Voor de eerste muisklik, zijn er nog geen mijnen geplaatst.
                // Voordat we alles uncoveren, moeten we dus eerst de mijnen plaatsen.
                if (!mines_placed)
                {
                    create_map();
                    alloc_game_states();
                    add_mines(NULL);
                    mines_placed = true;
                }
                // Uncover alles tijdelijk
                for (int x = 0; x < map_w; ++x)
                    for (int y = 0; y < map_h; ++y)
                    {
                        Coord c = coord_make(x, y);
                        cell_set_uncovered(c, true);
                    }
                print_view();
            }
            else
            {
                // terugzetten van vorige uncovered state
                for (int x = 0; x < map_w; ++x)
                    for (int y = 0; y < map_h; ++y)
                    {
                        Coord c = coord_make(x, y);
                        cell_set_uncovered(c, cell_is_saved_uncovered(c));
                    }
            }
            changed = true;
        }
        else if (event.key.keysym.sym == SDLK_b)
        {
            // Ensure map is generated before showing mines
            if (!mines_placed)
            {
                create_map();
                alloc_game_states();
                add_mines(NULL);
                mines_placed = true;
            }
            show_mines = !show_mines;
            printf("Toggle show_mines: %d\n", show_mines);
            changed = true;
        }
        else if (event.key.keysym.sym == SDLK_s)
        {
            save_game();
        }
        break;
    case SDL_QUIT:
        // De gebruiker heeft op het kruisje van het venster geklikt om de applicatie te stoppen.
        should_continue = 0;
        break;
    case SDL_MOUSEBUTTONDOWN:
        /*
         * De speler heeft met de muis geklikt:
         * We slaan de coördinaten van de muisklik op in de variabelen mouse_x en mouse_y.
         */
        mouse_x = event.button.x;
        mouse_y = event.button.y;

        // Bereken de coördinaten van de geklikte cel.
        int clicked_col = mouse_x / cell_w;
        int clicked_row = mouse_y / cell_h;

        if (event.button.button == SDL_BUTTON_RIGHT)
        {
            /*
             * Rechter muisknop: toggle een vlag op de cel.
             * Maar alleen als we niet al het maximale aantal vlaggen hebben geplaatst.
             */
            // Ensure map is generated before placing flags
            if (!mines_placed)
            {
                create_map();
                alloc_game_states();
                add_mines(NULL);
                mines_placed = true;
            }

            Coord clicked = coord_make(clicked_col, clicked_row);
            bool currently_flagged = cell_is_flagged(clicked);

            // Tellen hoeveel vlaggen er al zijn geplaatst
            int total_flags = 0;
            for (int y = 0; y < map_h; ++y)
            {
                for (int x = 0; x < map_w; ++x)
                {
                    Coord c = coord_make(x, y);
                    if (cell_is_flagged(c))
                        total_flags++;
                }
            }

            // Als we een vlag willen plaatsen en al het max bereikt hebben, skip
            if (!currently_flagged && total_flags >= map_mines)
            {
                printf("Cannot place more flags (max: %d)\n", map_mines);
            }
            else
            {
                cell_set_flagged(clicked, !currently_flagged);
                printf("Right click at (%d, %d) -> cell (col=%d, row=%d) flag=%d\n", mouse_x, mouse_y, clicked_col, clicked_row, (int)cell_is_flagged(clicked));
                changed = true;
            }

            /*
             * Controleer of de speler gewonnen heeft door alle mijnen correct te vlaggen.
             * We tellen het totaal aan vlaggen en hoeveel daarvan op een mijn staan.
             */
            int correct_flags = 0;
            int flagged_count = 0;
            for (int y = 0; y < map_h; ++y)
            {
                for (int x = 0; x < map_w; ++x)
                {
                    Coord c = coord_make(x, y);
                    if (cell_is_flagged(c))
                    {
                        flagged_count++;
                        if (MAP_AT(c).is_mine)
                            correct_flags++;
                    }
                }
            }
            // Als het aantal vlaggen gelijk is aan het aantal mijnen en alle mijnen correct geflagd zijn -> win
            if (correct_flags == map_mines && flagged_count == map_mines)
            {
                printf("All mines flagged - you win!\n");
                // Start win-animatie: markeer game_won en initialiseer verwijder-lijst
                game_won = true;
                win_remaining = map_w * map_h;
                for (int y = 0; y < map_h; ++y)
                {
                    for (int x = 0; x < map_w; ++x)
                    {
                        Coord c = coord_make(x, y);
                        cell_set_removed(c, false); // nog niet verwijderd tijdens animatie
                    }
                }
                // Seed de RNG met huidige ticks zodat de verwijdervolgorde random is
                srand((unsigned int)SDL_GetTicks());
                changed = true;
            }
        }
        else
        {
            /*
             * Linker muisknop: ontdek cel.
             * Bij de eerste klik plaatsen we eerst de mijnen (exclusief de aangeklikte cel).
             */
            printf("Left click at (%d, %d) -> cell (col=%d, row=%d)\n", mouse_x, mouse_y, clicked_col, clicked_row);
            Coord clicked = coord_make(clicked_col, clicked_row);

            if (!mines_placed)
            {
                // dit coördinaat uitsluiten bij mijnplaatsing, omdat de speler hier net geklikt heeft
                add_mines(&clicked);
                print_map();
                printf("\n");
                mines_placed = true;
                changed = true;
            }
            if (MAP_AT(clicked).is_mine)
            {
                // speler klikte op een mijn -> game over
                if (!game_lost)
                {
                    game_lost = true;
                    losing_coord = clicked;
                    lose_start_time = SDL_GetTicks();
                    // toon alle mijnen
                    for (int x = 0; x < map_w; ++x)
                    {
                        for (int y = 0; y < map_h; ++y)
                        {
                            Coord c = coord_make(x, y);
                            if (MAP_AT(c).is_mine)
                                cell_set_uncovered(c, true);
                        }
                    }
                    // uncover ook de mijn waarop geklikt werd
                    cell_set_uncovered(losing_coord, true);
                    printf("You clicked a mine at (col=%d, row=%d) - you lose.\n", losing_coord.x, losing_coord.y);
                    changed = true;
                }
            }
            else if (MAP_AT(clicked).neighbour_mines == 0)
            {
                // Wanneer een lege cel (0 neighbour mines) wordt aangeklikt, worden de naburige cellen ook automatisch ontdekt.
                // We doen dit d.m.v. een array die als stack fungeren.
                int size = (int)map_w * (int)map_h;
                Coord stack[size];
                int i = 0;
                // We push de startcel op de stack.
                stack[i] = clicked;
                i++;

                while (i > 0)
                {
                    // iterateer zolang er cellen in de stack zitten
                    i--;
                    Coord current = stack[i];

                    // bounds check
                    if (!coord_valid(current))
                        continue;
                    // als de cel al uncovered is, slaan we ze over
                    if (cell_is_uncovered(current))
                        continue;
                    // uncover de cel
                    cell_set_uncovered(current, true);
                    changed = true;
                    // als de cel ook 0 aangrenzende mijnen heeft, push alle niet-onthulde en niet-gevlagde buren
                    if (MAP_AT(current).neighbour_mines == 0)
                    {
                        // push alle 8 buren
                        for (int diag_x = -1; diag_x <= 1; diag_x++)
                        {
                            for (int diag_y = -1; diag_y <= 1; diag_y++)
                            {
                                if (diag_x == 0 && diag_y == 0)
                                    continue;
                                Coord neighbor = coord_make(current.x + diag_x, current.y + diag_y);
                                if (!coord_valid(neighbor))
                                    continue;
                                if (!cell_is_uncovered(neighbor) && !cell_is_flagged(neighbor))
                                {
                                    // push de buurcel
                                    stack[i] = neighbor;
                                    i++;
                                }
                            }
                        }
                    }
                }
            }
            else
            {
                // uncover een normale nummer cel
                if (!cell_is_uncovered(clicked))
                {
                    cell_set_uncovered(clicked, true);
                    changed = true;
                }
            }
        }
        break;
    }

    if (changed)
    {
        print_view();
    }
    return;
}

void draw_window()
{
    int grid_rows = map_h, grid_cols = map_w;
    int cell_w = curr_win_w / grid_cols;
    int cell_h = curr_win_h / grid_rows;

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderClear(renderer);

    if (!game_won && !game_lost)
    {
        int marker_col = mouse_x / cell_w;
        int marker_row = mouse_y / cell_h;
        if (marker_col >= 0 && marker_col < grid_cols && marker_row >= 0 && marker_row < grid_rows)
        {
            SDL_Rect marker_rect = {marker_col * cell_w, marker_row * cell_h, cell_w, cell_h};
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer, 255, 255, 0, 100);
            SDL_RenderFillRect(renderer, &marker_rect);
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        }
    }

    for (int row = 0; row < grid_rows; ++row)
    {
        for (int col = 0; col < grid_cols; ++col)
        {
            Coord c = coord_make(col, row);
            SDL_Rect rect = {col * cell_w, row * cell_h, cell_w, cell_h};
            // During win animation, removed cells disappear; otherwise render normally
            if (game_won && cell_is_removed(c))
            {
                continue; // disappeared
            }

            // If user asked to show mines, draw them regardless of uncovered state
            if (show_mines && MAP_AT(c).is_mine)
            {
                // if win animation is active and cell has been removed, skip drawing
                if (game_won && cell_is_removed(c))
                    continue;
                SDL_RenderCopy(renderer, digit_mine_texture, NULL, &rect);
                continue;
            }

            if (cell_is_uncovered(c))
            {
                if (MAP_AT(c).is_mine)
                {
                    // If player lost and this is the losing mine, make it blink red every second
                    if (game_lost && coord_equals(c, losing_coord))
                    {
                        uint32_t t = SDL_GetTicks();
                        uint32_t elapsed = t - lose_start_time;
                        int visible = ((elapsed / 1000) % 2) == 0; // blink every second relative to loss time
                        if (visible)
                        {
                            // tint red for the losing mine
                            SDL_SetTextureColorMod(digit_mine_texture, 255, 0, 0);
                            SDL_RenderCopy(renderer, digit_mine_texture, NULL, &rect);
                            // reset tint
                            SDL_SetTextureColorMod(digit_mine_texture, 255, 255, 255);
                        }
                        else
                        {
                            // when not visible, draw the covered texture so the marker or previous frames are hidden
                            if (digit_covered_texture)
                                SDL_RenderCopy(renderer, digit_covered_texture, NULL, &rect);
                        }
                    }
                    else
                    {
                        SDL_RenderCopy(renderer, digit_mine_texture, NULL, &rect);
                    }
                }
                else
                {
                    int idx = MAP_AT(c).neighbour_mines;
                    if (idx >= 0 && idx <= 8 && digit_textures[idx])
                    {
                        SDL_RenderCopy(renderer, digit_textures[idx], NULL, &rect);
                    }
                    else
                    {
                        SDL_RenderCopy(renderer, digit_covered_texture, NULL, &rect);
                    }
                }
            }
            else
            {
                if (cell_is_flagged(c) && digit_flagged_texture)
                {
                    SDL_RenderCopy(renderer, digit_flagged_texture, NULL, &rect);
                }
                else if (digit_covered_texture)
                {
                    SDL_RenderCopy(renderer, digit_covered_texture, NULL, &rect);
                }
            }
        }
    }

    // If win animation running, remove a random non-removed cell periodically
    if (game_won && win_remaining > 0)
    {
        uint32_t now = SDL_GetTicks();
        if (now - win_last_remove >= 20)
        {
            // pick a random remaining cell
            int tries = 0;
            while (tries < 1000 && win_remaining > 0)
            {
                Coord rc = coord_make(rand() % map_w, rand() % map_h);
                if (!cell_is_removed(rc))
                {
                    cell_set_removed(rc, true);
                    win_remaining--;
                    break;
                }
                tries++;
            }
            win_last_remove = now;
        }
        // when all removed, close the game
        if (win_remaining <= 0)
        {
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
 * Deze functie initialiseert het venster en alle extra structuren die nodig zijn om het venster te manipuleren.
 * Ook wordt de display size van het te gebruiken scherm hier berekend.
 * Op deze manier kunnen we de GUI in het midden van het scherm zetten bij het opstarten van het spel.
 * Zie SDL2 documentatie:
 * - https://wiki.libsdl.org/SDL2/SDL_GetDesktopDisplayMode voor SDL_GetDesktopDisplayMode
 * - https://wiki.libsdl.org/SDL2/SDL_Log voor SDL_Log
 * - https://wiki.libsdl.org/SDL2/SDL_GetError voor SDL_GetError
 */
void initialize_window(const char *title, int window_width, int window_height)
{
    // Vraag de desktop display mode op.
    if (SDL_GetDesktopDisplayMode(0, &dm) != 0)
    {
        SDL_Log("SDL_GetDesktopDisplayMode failed: %s", SDL_GetError());
        exit(1);
    }

    // Maak het venster aan met de gegeven dimensies en titel.
    window = SDL_CreateWindow(title, (dm.w - window_width) / 2, (dm.h - window_height) / 2, window_width, window_height, SDL_WINDOW_SHOWN);

    if (window == NULL)
    {
        // Er ging iets verkeerd bij het initialiseren.
        printf("Couldn't set screen mode to required dimensions: %s\n", SDL_GetError());
        exit(1);
    }

    // Initialiseert de renderer.
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);
    // Zet de huidige window afmetingen juist.
    curr_win_w = window_width;
    curr_win_h = window_height;
    // Laat de default-kleur die de renderer in het venster tekent wit zijn.
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
}

// Laad alle afbeeldingen die getoond moeten worden in.
void initialize_textures()
{
    /*
     * Laad de nummer afbeeldingen in.
     * Indien een afbeelding niet kon geladen worden (bv. omdat het pad naar de afbeelding verkeerd is),
     * geeft SDL_LoadBMP een NULL-pointer terug.
     */
    const char *num_files[9] = {
        "Images/0.bmp", "Images/1.bmp", "Images/2.bmp",
        "Images/3.bmp", "Images/4.bmp", "Images/5.bmp",
        "Images/6.bmp", "Images/7.bmp", "Images/8.bmp"};
    for (int i = 0; i < 9; ++i)
    {
        SDL_Surface *s = SDL_LoadBMP(num_files[i]);
        if (s)
        {
            digit_textures[i] = SDL_CreateTextureFromSurface(renderer, s);
            SDL_FreeSurface(s);
        }
        else
        {
            digit_textures[i] = NULL;
        }
    }

    // Laad de andere afbeeldingen apart in.
    SDL_Surface *digit_covered_surface = SDL_LoadBMP("Images/covered.bmp");
    SDL_Surface *digit_flagged_surface = SDL_LoadBMP("Images/flagged.bmp");
    SDL_Surface *digit_mine_surface = SDL_LoadBMP("Images/mine.bmp");

    digit_covered_texture = SDL_CreateTextureFromSurface(renderer, digit_covered_surface);
    digit_flagged_texture = SDL_CreateTextureFromSurface(renderer, digit_flagged_surface);
    digit_mine_texture = SDL_CreateTextureFromSurface(renderer, digit_mine_surface);

    // Dealloceer de tijdelijke SDL_Surfaces die werden aangemaakt.
    SDL_FreeSurface(digit_covered_surface);
    SDL_FreeSurface(digit_flagged_surface);
    SDL_FreeSurface(digit_mine_surface);
}

/*
 * Initialiseert onder het venster waarin het speelveld getoond zal worden, en de texture van de afbeelding die getoond zal worden.
 * Deze functie moet aangeroepen worden aan het begin van het spel, vooraleer je de spelwereld begint te tekenen.
 */
void initialize_gui(int window_width, int window_height)
{
    initialize_window("Minesweeper", window_width, window_height);
    initialize_textures();
    // Maakt van wit de standaard, blanco achtergrondkleur.
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
}

// Dealloceert alle SDL structuren die geïnitialiseerd werden.
void free_gui()
{
    // Dealloceert de afbeeldingen.
    for (int i = 0; i < 9; ++i)
    {
        if (digit_textures[i])
            SDL_DestroyTexture(digit_textures[i]);
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

// Sla het huidige speelveld op in een genummerd bestand met naam: field_<width>x<height>_<n>.txt
void save_game()
{
    char filenamebuf[256];
    int n = 1;
    // Blijf proberen tot een geldige bestandsnaam is gevonden.
    while (1)
    {
        snprintf(filenamebuf, sizeof(filenamebuf), "field_%dx%d_%d.txt", map_w, map_h, n);
        // De functie controleert of het bestand al bestaat door het te proberen openen.
        FILE *f = fopen(filenamebuf, "r");
        // Als de bestandsnaam al is ingenomen, sluiten we het bestand en incrementen we de counter n.
        if (f)
        {
            fclose(f);
            n++;
            continue;
        }
        break;
    }
    // Er wordt geheugen gealloceerd voor twee tijdelijke arrays van char pointers.
    int cells = (int)map_w * (int)map_h;
    char *f_arr = (char *)malloc(cells); // Deze houdt de status van "flagged" per cel bij.
    char *u_arr = (char *)malloc(cells); // Deze houdt de status van "uncovered" per cel bij.
    if (!f_arr || !u_arr)
    {
        if (f_arr)
            free(f_arr);
        if (u_arr)
            free(u_arr);
        perror("Out of memory error!");
        return;
    }
    // Voor elk cel wordt 1 byte gebruikt in de tijdelijke arrays om aan te duiden of deze "flagged" of "uncovered" is.
    for (int x = 0; x < map_w; ++x)
    {
        for (int y = 0; y < map_h; ++y)
        {
            Coord c = coord_make(x, y);
            int i = y * map_w + x;
            f_arr[i] = cell_is_flagged(c) ? 1 : 0;
            u_arr[i] = cell_is_uncovered(c) ? 1 : 0;
        }
    }
    // Converteer Cell struct array naar char array voor save_field()
    char *map_as_char = (char *)malloc(cells);
    if (!map_as_char)
    {
        fprintf(stderr, "Out of memory error!\n");
        free(f_arr);
        free(u_arr);
        return;
    }
    for (int i = 0; i < cells; ++i)
    {
        if (map[i].is_mine)
            map_as_char[i] = 'M';
        else
            map_as_char[i] = '0' + map[i].neighbour_mines;
    }

    /*
     * We slaan het speelveld op via de save_field functie.
     * Deze functie zal de map, de flagged array en de uncovered array wegschrijven naar een bestand met naam filenamebuf.
     */
    if (save_field(filenamebuf, map_w, map_h, map_as_char, f_arr, u_arr) != 0)
    {
        fprintf(stderr, "Error saving field to %s\n", filenamebuf);
    }
    else
    {
        printf("Saved field to %s\n", filenamebuf);
    }
    free(map_as_char);
    free(f_arr);
    free(u_arr);
}

/*
 * We laden met de load_file functie een spelbestand in.
 * Een spelbestand bestaat uit:
 * - de state van een actief gespeelde map
 * - de uncovered map (oplossing)
 * Verder betekenen de volgende gebruikte letters in het bestand:
 * - U: uncovered
 * - F: flagged
 * - M: mine
 * - #: covered
 */
int load_file(const char *filename)
{
    char **lines = NULL;
    int count = 0;
    // We lezen het bestand regel per regel in via read_lines().
    if (read_lines(filename, &lines, &count) != 0)
        return -1;

    // Controleer of het bestand minimaal één regel bevat.
    if (count == 0)
    {
        free_lines(lines, count);
        return -1;
    }

    /*
     * Zoek de scheidingsregel (lege regel) tussen het speelveld en de oplossingsmap.
     * Als sep == -1, dan is er geen oplossingsmap aanwezig.
     */
    int sep = -1;
    for (int i = 0; i < count; ++i)
        if (lines[i][0] == '\0')
        {
            sep = i;
            break;
        }

    /*
     * Bepaal het aantal rijen van het speelveld.
     * Als er geen scheidingsregel wordt gevonden, dan bevat het bestand enkel het speelveld.
     */
    int map_count = (sep == -1) ? count : sep;

    // We bepalen het aantal kolommen door de niet-whitespace karakters in de eerste regel te tellen.
    int cols = 0;
    for (int i = 0; lines[0][i] != '\0'; i++)
    {
        if (lines[0][i] != ' ' && lines[0][i] != '\t')
            cols++;
    }

    // We controleren of er minimaal één kolom is.
    if (cols <= 0)
    {
        free_lines(lines, count);
        return -1;
    }

    /*
     * We roepen de init_map functie aan om een speelveld/map te initialiseren met de juiste afmetingen.
     * We geven 0 als het aantal mijnen mee, zodat er nog geen mijnen random geplaatst worden.
     * De mijnen worden later immers uit het bestand ingelezen.
     */
    if (init_map(cols, map_count, 0) != 0)
    {
        free_lines(lines, count);
        return -1;
    }

    /*
     * We gaan door elke regel van het speelveld en vullen ons speelveld/onze map in met de juiste waarden.
     * We tellen ook het aantal mijnen tijdens het inlezen.
     */
    int mines = 0;
    for (int i = 0; i < map_count; ++i)
    {
        int col = 0;
        for (int j = 0; lines[i][j] != '\0' && col < cols; j++)
        {
            // sla whitespaces over
            if (lines[i][j] != ' ' && lines[i][j] != '\t')
            {
                Coord c = coord_make(col, i);
                char ch = lines[i][j];
                if (ch == 'M')
                {
                    MAP_AT(c).is_mine = true;
                    MAP_AT(c).neighbour_mines = 0;
                    mines++;
                }
                else if (ch >= '0' && ch <= '8')
                {
                    MAP_AT(c).is_mine = false;
                    MAP_AT(c).neighbour_mines = ch - '0';
                }
                col++;
            }
        }
    }
    map_mines = mines;

    // We initialiseren via alloc_game_states() de benodigde game_states.
    if (alloc_game_states() != 0)
    {
        free_lines(lines, count);
        free_map();
        return -1;
    }

    // We zetten alle uncovered en flagged states standaard op false.
    for (int x = 0; x < map_w; ++x)
    {
        for (int y = 0; y < map_h; ++y)
        {
            Coord c = coord_make(x, y);
            cell_set_uncovered(c, false);
            cell_set_flagged(c, false);
        }
    }

    /*
     * Wanneer er een scheidingsregel is gevonden, bevat het bestand ook een oplossingsmap.
     * We moeten dus door elke regel van de oplossingsmap gaan en de FLAG/UNC states instellen voor alle cellen.
     */
    if (sep != -1)
    {
        // We berekenen het aantal rijen van de oplossingsmap.
        int state_rows = count - (sep + 1);
        // Als de oplossingsmap meer rijen bevat dan het speelveld, worden de overige rijen genegeerd.
        int use_rows = state_rows < map_count ? state_rows : map_count;

        // We gaan over elke regel van de oplossingsmap en vullen de FLAG/UNC states in.
        for (int i = 0; i < use_rows; ++i)
        {
            int col = 0;
            for (int j = 0; lines[sep + 1 + i][j] != '\0' && col < cols; j++)
            {
                // sla whitespaces over
                if (lines[sep + 1 + i][j] != ' ' && lines[sep + 1 + i][j] != '\t')
                {
                    Coord c = coord_make(col, i);
                    // deze cell is flagged
                    if (lines[sep + 1 + i][j] == 'F')
                        cell_set_flagged(c, true);
                    // deze cell is uncovered
                    else if (lines[sep + 1 + i][j] == 'U')
                        cell_set_uncovered(c, true);
                    col++;
                }
            }
        }
    }

    // We dealloceren het gebruikte geheugen voor de ingelezen lijnen en de bijhorende count.
    free_lines(lines, count);
    // De mijnen zijn nu geplaatst.
    mines_placed = true;
    return 0;
}
