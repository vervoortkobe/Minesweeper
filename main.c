#include <stdio.h>
#include "GUI.h"
#include "args.h"
#include "map.h"

//Beginfunctie van de gehele applicatie. Hierin worden alle andere functies aangeroepen.
int SDL_main(int argc, char *argv[]) {
    /*
    * CliArgs: we instantiëren de cli variabele om daarna te kunnen gebruiken in de functie en daarbuiten.
    * We parsen de CLI argumenten met de parse_cli_args functie en printen een eventuele error.
    * Zie HOC Slides 4_input_output dia 29.
    */
    CLIArgs cli;
    if (parse_cli_args(argc, argv, &cli) != 0) {
        fprintf(stderr, "%s\n", cli.error_msg);
        return 1;
    }

    /*
    * Kijk na of er een bestand werd meegegeven via args (dit wordt meegegeven cli.load_file).
    * Indien ja, laad de map vanuit het bestand met load_game_file.
    * Print een eventuele error.
    */
    if (cli.load_file) {
        if (load_game_file(cli.load_file) != 0) {
            fprintf(stderr, "Failed to load map from %s\n", cli.load_file);
            return 1;
        }
    /*
    * Als er geen bestand wordt meegegeven, wordt er gekeken of er een breedte, hoogte en aantal mijnen worden meegegeven.
    * Ook wordt er gecheckt of deze waarden geldig zijn (breedte en hoogte groter dan 0, aantal mijnen groter of gelijk aan 0).
    * Indien ja, dan creëren we een map met init_map en create_map.
    * Anders printen we de error.
    */
    } else if (cli.w > 0 && cli.h > 0 && cli.m >= 0) {
        if (init_map(cli.w, cli.h, cli.m) != 0) {
            fprintf(stderr, "Failed to initialize map %dx%d\n", cli.w, cli.h);
            return 1;
        }
        create_map();
        // We alloceren de nodige game states met alloc_game_states en printen een eventuele error.
        if (alloc_game_states() != 0) {
            fprintf(stderr, "Failed to allocate game states\n");
            return 1;
        }
    /*
    * Als er geen bestand of breedte, hoogte of aantal mijnen wordt meegegeven, creëren we een standaard map van 10x10 met 10 mijnen.
    */
    } else {
        create_map();
        // We alloceren de nodige game states met alloc_game_states en printen een eventuele error.
        if (alloc_game_states() != 0) {
            fprintf(stderr, "Failed to allocate game states\n");
            return 1;
        }
    }

    /*
    * Initialiseer de dimensies van de window en bepaal een geschikte image size.
    * We doen dit door de functie choose_image_and_window_size aan te roepen.
    */
    int chosen_image_size = 0;
    int win_w = WINDOW_WIDTH;
    int win_h = WINDOW_HEIGHT;
    if (choose_image_and_window_size(map_w, map_h, &chosen_image_size, &win_w, &win_h) != 0) {
        // Indien deze functie faalt, gebruiken we de standaard waarden.
        win_w = WINDOW_WIDTH;
        win_h = WINDOW_HEIGHT;
    }
    /*
    * Daarna initialiseren we de GUI met de juiste window breedte en hoogte.
    * De game loop wordt dan gestart, waarin we blijven tekenen en input lezen zolang should_continue waar is.
    */
    initialize_gui(win_w, win_h);
    while (should_continue) {
        draw_window();
        read_input();
    }
    // Dealloceer al het gebruikte geheugen voor de GUI, game states en map.
    free_gui();
    free_game_states();
    free_map();
    return 0;
}
