#include <stdlib.h>
#include <stdio.h>
#include <SDL2/SDL.h>
#include <string.h>
#include <stdbool.h>
#include "GUI.h"
#include "map.h"
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
static int curr_window_width = WINDOW_WIDTH;
static int curr_window_height = WINDOW_HEIGHT;

/*
 * Deze functie bepaalt de beste window en image size op basis van het aantal kolommen en rijen.
 * Het resultaat wordt via de out-parameters teruggegeven.
 */
int determine_img_win_size(int cols, int rows, int *out_img_size, int *out_window_width, int *out_window_height)
{
    if (!out_img_size || !out_window_width || !out_window_height)
        return -1;
    if (cols <= 0 || rows <= 0)
        return -1;

    /*
     * Code o.a. gebaseerd op:
     * http://lazyfoo.net/tutorials/SDL/02_getting_an_image_on_the_screen/index.php
     */
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        printf("Could not initialize SDL2: %s\n", SDL_GetError());
        return 1;
    }

    // We vragen de display mode van het scherm op en definiëren enkele standaardafmetingen.
    if (SDL_GetDesktopDisplayMode(0, &dm) != 0)
    {
        *out_img_size = DEFAULT_IMAGE_SIZE;
        *out_window_width = cols * (*out_img_size);
        *out_window_height = rows * (*out_img_size);
        return 0;
    }

    /*
     * De window afmetingen worden berekend d.m.v. aantal kolommen/rijen * image size.
     * Bereken de maximale window grootte d.m.v. de DEFAULT_IMAGE_SIZE
     */
    int img_size = DEFAULT_IMAGE_SIZE;
    int window_widthidth = cols * img_size;
    int window_heighteight = rows * img_size;

    // Als de window groter is dan het scherm, passen we de image size aan.
    if (window_widthidth > dm.w || window_heighteight > dm.h)
    {
        int max_width = dm.w / cols;
        int max_height = dm.h / rows;
        img_size = max_width < max_height ? max_width : max_height;
        if (img_size < 20)
            img_size = 20; // minimale grootte
        window_widthidth = cols * img_size;
        window_heighteight = rows * img_size;
    }

    *out_img_size = img_size;
    *out_window_width = window_widthidth;
    *out_window_height = window_heighteight;

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
 * Dit is 1 zolang de gebruiker de applicatie niet wilt afsluiten door op het kruisje te klikken.
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
static int losing_col = -1, losing_row = -1;
static uint32_t lose_start_time = 0;
static bool game_won = false;
static int win_remaining = 0;
static uint32_t win_last_remove = 0;
static bool show_all = false; // via 'p' key

int init_states()
{
    // We initialiseren alle cell states op false
    for (int y = 0; y < map_height; ++y)
    {
        for (int x = 0; x < map_width; ++x)
        {
            map[y][x].uncovered = false;
            map[y][x].flagged = false;
            map[y][x].removed = false;
            map[y][x].saved_uncovered = false;
        }
    }
    return 0;
}

/*
 * Bij elke interactie, wordt het speeldveld in de console geprint.
 * Zie HOC Slides 4_input_output dia 10 voor putchar.
 */
static void print_view()
{
    for (int y = 0; y < map_height; ++y)
    {
        for (int x = 0; x < map_width; ++x)
        {
            if (map[y][x].uncovered)
            {
                if (map[y][x].is_mine)
                    putchar('M');
                else
                    putchar('0' + map[y][x].neighbour_mines);
            }
            else if (map[y][x].flagged)
            {
                putchar('F');
            }
            else if (show_mines && map[y][x].is_mine)
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
    int grid_rows = map_height, grid_cols = map_width;
    int cell_w = curr_window_width / grid_cols;
    int cell_h = curr_window_height / grid_rows;

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
                for (int x = 0; x < map_width; ++x)
                    for (int y = 0; y < map_height; ++y)
                        map[y][x].saved_uncovered = map[y][x].uncovered;
                // Voor de eerste muisklik, zijn er nog geen mijnen geplaatst.
                // Voordat we alles uncoveren, moeten we dus eerst de mijnen plaatsen.
                if (!mines_placed)
                {
                    create_map();
                    init_states();
                    add_mines(-1, -1);
                    mines_placed = true;
                }
                // uncover alles tijdelijk
                for (int x = 0; x < map_width; ++x)
                    for (int y = 0; y < map_height; ++y)
                        map[y][x].uncovered = true;
                print_view();
            }
            else
            {
                // terugzetten van vorige uncovered state
                for (int x = 0; x < map_width; ++x)
                    for (int y = 0; y < map_height; ++y)
                        map[y][x].uncovered = map[y][x].saved_uncovered;
            }
            changed = true;
        }
        else if (event.key.keysym.sym == SDLK_b)
        {
            // Zorg ervoor dat de map gegenereerd wordt, voordat we de mijnen kunnen tonen.
            if (!mines_placed)
            {
                create_map();
                init_states();
                add_mines(-1, -1);
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

        // Bereken de coördinaten van de geklikte cell.
        int clicked_col = mouse_x / cell_w;
        int clicked_row = mouse_y / cell_h;

        if (event.button.button == SDL_BUTTON_RIGHT)
        {
            /*
             * Rechter muisknop: toggle een vlag op de cell.
             * Maar alleen als we niet al het maximale aantal vlaggen hebben geplaatst.
             */
            // Zorg ervoor dat de map gegenereerd wordt, voordat we vlaggen kunnen plaatsen.
            if (!mines_placed)
            {
                create_map();
                init_states();
                add_mines(-1, -1);
                mines_placed = true;
            }

            bool currently_flagged = map[clicked_row][clicked_col].flagged;

            // tellen hoeveel vlaggen er al zijn geplaatst
            int total_flags = 0;
            for (int y = 0; y < map_height; ++y)
            {
                for (int x = 0; x < map_width; ++x)
                {
                    if (map[y][x].flagged)
                        total_flags++;
                }
            }

            // Als we een vlag willen plaatsen en al het maximale aantal vlaggen bereikt hebben, wordt de actie genegeerd.
            if (!currently_flagged && total_flags >= map_mines)
            {
                printf("Cannot place more flags (max: %d)\n", map_mines);
            }
            else
            {
                map[clicked_row][clicked_col].flagged = !map[clicked_row][clicked_col].flagged;
                printf("Right click at (%d, %d) -> cell (%d, %d) flag: %d\n", mouse_x, mouse_y, clicked_col, clicked_row, (int)map[clicked_row][clicked_col].flagged);
                changed = true;
            }

            /*
             * Controleer of de speler gewonnen heeft door alle mijnen correct te vlaggen.
             * We tellen het totaal aan vlaggen en hoeveel daarvan op een mijn staan.
             */
            int correct_flags = 0;
            int flagged_count = 0;
            for (int y = 0; y < map_height; ++y)
            {
                for (int x = 0; x < map_width; ++x)
                {
                    if (map[y][x].flagged)
                    {
                        flagged_count++;
                        if (map[y][x].is_mine)
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
                win_remaining = map_width * map_height;
                for (int y = 0; y < map_height; ++y)
                {
                    for (int x = 0; x < map_width; ++x)
                    {
                        map[y][x].removed = false; // nog niet verwijderd tijdens animatie
                    }
                }
                // Seed de RNG met huidige ticks zodat de verwijdervolgorde random is.
                srand((unsigned int)SDL_GetTicks());
                changed = true;
            }
        }
        else
        {
            /*
             * Linker muisknop klik: uncover cell.
             * Bij de eerste klik plaatsen we eerst de mijnen (exclusief de aangeklikte cell).
             */
            printf("Left click at (%d, %d) -> cell (%d, %d)\n", mouse_x, mouse_y, clicked_col, clicked_row);

            if (!mines_placed)
            {
                // Dit coördinaat sluiten we uit bij het plaatsen van de mijnen, aangezien de speler hier net als eerste geklikt heeft.
                add_mines(clicked_col, clicked_row);
                print_map();
                printf("\n");
                mines_placed = true;
                changed = true;
            }
            if (map[clicked_row][clicked_col].is_mine)
            {
                /*
                 * De speler klikte op een mijn -> game over
                 * Maar niet als show_mines actief is.
                 */
                if (!game_lost && !show_mines)
                {
                    game_lost = true;
                    losing_col = clicked_col;
                    losing_row = clicked_row;
                    lose_start_time = SDL_GetTicks();
                    // toon alle mijnen
                    for (int x = 0; x < map_width; ++x)
                    {
                        for (int y = 0; y < map_height; ++y)
                        {
                            if (map[y][x].is_mine)
                                map[y][x].uncovered = true;
                        }
                    }
                    // uncover ook de mijn waarop geklikt werd
                    map[clicked_row][clicked_col].uncovered = true;
                    printf("You clicked a mine at (%d, %d) - you lose.\n", clicked_col, clicked_row);
                    changed = true;
                }
            }
            else if (map[clicked_row][clicked_col].neighbour_mines == 0)
            {
                // Wanneer een nul-cell (zonder aangrenzende mijnen) wordt aangeklikt, worden de naburige cellen ook automatisch ontdekt.
                // We doen dit d.m.v. een array die als stack fungeert.
                int size = (int)map_width * (int)map_height;
                int stack_y[size], stack_x[size];
                int i = 0;
                // We pushen de startcel op de stack.
                stack_y[i] = clicked_row;
                stack_x[i] = clicked_col;
                i++;

                while (i > 0)
                {
                    // itereer zolang er cellen in de stack zitten
                    i--;
                    int curr_y = stack_y[i];
                    int curr_x = stack_x[i];

                    // bounds check
                    if (curr_x < 0 || curr_x >= map_width || curr_y < 0 || curr_y >= map_height)
                        continue;
                    // als de cell al uncovered is, slaan we ze over
                    if (map[curr_y][curr_x].uncovered)
                        continue;
                    // uncover de cell
                    map[curr_y][curr_x].uncovered = true;
                    changed = true;
                    // als de cell ook 0 aangrenzende mijnen heeft, pushen we alle niet-onthulde en niet-gevlagde buurcellen
                    if (map[curr_y][curr_x].neighbour_mines == 0)
                    {
                        // we pushen alle 8 buurcellen
                        for (int diag_x = -1; diag_x <= 1; diag_x++)
                        {
                            for (int diag_y = -1; diag_y <= 1; diag_y++)
                            {
                                if (diag_x == 0 && diag_y == 0)
                                    continue;
                                int neighbor_x = curr_x + diag_x;
                                int neighbor_y = curr_y + diag_y;
                                if (neighbor_x < 0 || neighbor_x >= map_width || neighbor_y < 0 || neighbor_y >= map_height)
                                    continue;
                                if (!map[neighbor_y][neighbor_x].uncovered && !map[neighbor_y][neighbor_x].flagged)
                                {
                                    // we pushen de buurcell
                                    stack_y[i] = neighbor_y;
                                    stack_x[i] = neighbor_x;
                                    i++;
                                }
                            }
                        }
                    }
                }
            }
            else
            {
                // uncover een "normale nummer cell"
                if (!map[clicked_row][clicked_col].uncovered)
                {
                    map[clicked_row][clicked_col].uncovered = true;
                    changed = true;
                }
            }

            /*
             * We checken of de speler alle nummer cellen als uncovered heeft aangeklikt -> win
             * Maar alleen als show_all niet actief is.
             */
            if (!game_won && !show_all)
            {
                bool all_number_cells_uncovered = true;
                for (int y = 0; y < map_height && all_number_cells_uncovered; ++y)
                {
                    for (int x = 0; x < map_width && all_number_cells_uncovered; ++x)
                    {
                        if (!map[y][x].is_mine && !map[y][x].uncovered)
                        {
                            all_number_cells_uncovered = false;
                        }
                    }
                }
                if (all_number_cells_uncovered)
                {
                    printf("All numbers cells uncovered - you win!\n");
                    // Start win-animatie: markeer game_won en initialiseer verwijder-lijst
                    game_won = true;
                    win_remaining = map_width * map_height;
                    for (int y = 0; y < map_height; ++y)
                    {
                        for (int x = 0; x < map_width; ++x)
                        {
                            map[y][x].removed = false; // nog niet verwijderd tijdens animatie
                        }
                    }
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

// Deze functie tekent het speelveld met alle afbeeldingen e.d.
void draw_window()
{
    // We berekenen de grootte van elke cell op basis van de rij en kolom aantallen en de huidige window grootte.
    int grid_rows = map_height, grid_cols = map_width;
    int cell_w = curr_window_width / grid_cols;
    int cell_h = curr_window_height / grid_rows;

    // We wissen de renderbuffer met een witte achtergrond.
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderClear(renderer);

    if (!game_won && !game_lost)
    {

        // We tekenen het muiscursor hover effect.
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

    // We voeren de win-animatie uit door willekeurige cellen te verwijderen.
    for (int row = 0; row < grid_rows; ++row)
    {
        for (int col = 0; col < grid_cols; ++col)
        {
            SDL_Rect rect = {col * cell_w, row * cell_h, cell_w, cell_h};
            // Tijdens win-animatie verdwijnen verwijderde cellen; anders normaal renderen.
            if (game_won && map[row][col].removed)
            {
                continue; // cell is al verwijderd
            }

            // Als de gebruiker heeft gevraagd om mijnen te tonen, dan worden ze hier getekend, zelfs als ze nog niet uncovered zijn.
            if (show_mines && map[row][col].is_mine)
            {
                SDL_RenderCopy(renderer, digit_mine_texture, NULL, &rect);
                continue;
            }

            if (map[row][col].uncovered)
            {
                if (map[row][col].is_mine)
                {
                    // Als de speler verloren heeft, laten we de mijn waarop laatst geklikt werd rood knipperen.
                    if (game_lost && col == losing_col && row == losing_row)
                    {
                        int time = SDL_GetTicks();
                        int elapsed = time - lose_start_time;
                        int visible = ((elapsed / 1000) % 2) == 0; // knipper elke seconde relatief tov start-verlies tijd
                        if (visible)
                        {
                            // rode tint
                            SDL_SetTextureColorMod(digit_mine_texture, 255, 0, 0);
                            SDL_RenderCopy(renderer, digit_mine_texture, NULL, &rect);
                            // reset de rode tint
                            SDL_SetTextureColorMod(digit_mine_texture, 255, 255, 255);
                        }
                        else
                        {
                            // when not visible, draw the covered texture so the marker or previous frames are hidden
                            // Wanneer de cell covered is, tekenen we de covered texture, zodat de hover marker of vorige frames terug worden verborgen.
                            if (digit_covered_texture)
                                SDL_RenderCopy(renderer, digit_covered_texture, NULL, &rect);
                        }
                    }
                    else
                        SDL_RenderCopy(renderer, digit_mine_texture, NULL, &rect);
                }
                else
                {
                    int i = map[row][col].neighbour_mines;
                    if (i >= 0 && i <= 8 && digit_textures[i])
                        SDL_RenderCopy(renderer, digit_textures[i], NULL, &rect);
                    else
                        SDL_RenderCopy(renderer, digit_covered_texture, NULL, &rect);
                }
            }
            else
            {
                if (map[row][col].flagged && digit_flagged_texture)
                    SDL_RenderCopy(renderer, digit_flagged_texture, NULL, &rect);
                else if (digit_covered_texture)
                    SDL_RenderCopy(renderer, digit_covered_texture, NULL, &rect);
            }
        }
    }

    // We voeren de winanimatie uit door willekeurige cellen te verwijderen.
    if (game_won && win_remaining > 0)
    {
        int now = SDL_GetTicks();
        if (now - win_last_remove >= 20)
        {
            // Kies een random overblijvende cel om te verwijderen.
            int tries = 0;
            while (tries < 1000 && win_remaining > 0)
            {
                int rc_y = rand() % map_height;
                int rc_x = rand() % map_width;
                if (!map[rc_y][rc_x].removed)
                {
                    map[rc_y][rc_x].removed = true;
                    win_remaining--;
                    break;
                }
                tries++;
            }
            win_last_remove = now;
        }
        // Wanneer alle cellen verwijderd zijn, wordt het spel afgesloten.
        if (win_remaining <= 0)
            should_continue = 0;
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
    curr_window_width = window_width;
    curr_window_height = window_height;
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
            digit_textures[i] = NULL;
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
        snprintf(filenamebuf, sizeof(filenamebuf), "field_%dx%d_%d.txt", map_width, map_height, n);
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
    int cells = (int)map_width * (int)map_height;
    char *f_arr = (char *)malloc(cells); // Deze houdt de status van "flagged" per cell bij.
    char *u_arr = (char *)malloc(cells); // Deze houdt de status van "uncovered" per cell bij.
    if (!f_arr || !u_arr)
    {
        if (f_arr)
            free(f_arr);
        if (u_arr)
            free(u_arr);
        perror("Out of memory error!");
        return;
    }
    // Voor elke cell wordt 1 byte gebruikt in de tijdelijke arrays om aan te duiden of deze "flagged" of "uncovered" is.
    for (int x = 0; x < map_width; ++x)
    {
        for (int y = 0; y < map_height; ++y)
        {
            int i = y * map_width + x;
            f_arr[i] = map[y][x].flagged ? 1 : 0;
            u_arr[i] = map[y][x].uncovered ? 1 : 0;
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
    for (int y = 0; y < map_height; ++y)
    {
        for (int x = 0; x < map_width; ++x)
        {
            int i = y * map_width + x;
            if (map[y][x].is_mine)
                map_as_char[i] = 'M';
            else
                map_as_char[i] = '0' + map[y][x].neighbour_mines;
        }
    }

    /*
     * We slaan het speelveld op via de save_field functie.
     * Deze functie zal de map, de flagged array en de uncovered array wegschrijven naar een bestand met naam filenamebuf.
     */
    if (save_field(filenamebuf, map_width, map_height, map_as_char, f_arr, u_arr) != 0)
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
                char ch = lines[i][j];
                if (ch == 'M')
                {
                    map[i][col].is_mine = true;
                    map[i][col].neighbour_mines = 0;
                    mines++;
                }
                else if (ch >= '0' && ch <= '8')
                {
                    map[i][col].is_mine = false;
                    map[i][col].neighbour_mines = ch - '0';
                }
                col++;
            }
        }
    }
    map_mines = mines;

    // We initialiseren via init_states() de benodigde game states.
    if (init_states() != 0)
    {
        free_lines(lines, count);
        free_map();
        return -1;
    }

    // We zetten alle uncovered en flagged states standaard op false.
    for (int x = 0; x < map_width; ++x)
    {
        for (int y = 0; y < map_height; ++y)
        {
            map[y][x].uncovered = false;
            map[y][x].flagged = false;
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
                    // deze cell is flagged
                    if (lines[sep + 1 + i][j] == 'F')
                        map[i][col].flagged = true;
                    // deze cell is uncovered
                    else if (lines[sep + 1 + i][j] == 'U')
                        map[i][col].uncovered = true;
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
