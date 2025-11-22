#include <stdio.h>
#include "GUI.h"
#include "args.h"
#include "map.h"

//Beginfunctie van de gehele applicatie. Hierin worden alle andere functies aangeroepen.
int SDL_main(int argc, char *argv[]) {
    /*
    * Args: we instantiëren de args variabele om daarna te kunnen gebruiken in de functie en daarbuiten.
    * We parsen de CLI argumenten met de parse_args functie en printen een eventuele error (zie functie definitie).
    * Zie HOC Slides 4_input_output dia 29 voor perror.
    */
    Args args;
    if (parse_args(argc, argv, &args) != 0) {
        return 1;
    }

    /*
    * Kijk na of er een bestand werd meegegeven via args (dit wordt meegegeven args.file).
    * Indien ja, laad de map vanuit het bestand met load_file.
    * Print een eventuele error.
    */
    if (args.file) {
        if (load_file(args.file) != 0) {
           fprintf(stderr, "Failed to load map from %s\n", args.file);
            return 1;
        }
    /*
    * Als er geen bestand wordt meegegeven, wordt er gekeken of er een breedte, hoogte en aantal mijnen worden meegegeven.
    * Ook wordt er gecheckt of deze waarden geldig zijn (breedte en hoogte groter dan 0, aantal mijnen groter of gelijk aan 0).
    * Indien ja, dan creëren we een map met init_map en create_map.
    * Anders printen we de error.
    */
    } else if (args.w > 0 && args.h > 0 && args.m >= 0) {
        if (init_map(args.w, args.h, args.m) != 0) {
            fprintf(stderr, "Failed to initialize map %dx%d\n", args.w, args.h);
            return 1;
        }
        create_map();
        // We alloceren de nodige game states met alloc_game_states en printen een eventuele error.
        if (alloc_game_states() != 0) {
            perror("Failed to allocate game states");
            return 1;
        }
    // Als er geen bestand of breedte, hoogte of aantal mijnen wordt meegegeven, creëren we een standaard map van 10x10 met 10 mijnen.
    } else {
        create_map();
        // We alloceren de nodige game states met alloc_game_states en printen een eventuele error.
        if (alloc_game_states() != 0) {
            perror("Failed to allocate game states");
            return 1;
        }
    }

    /*
    * Initialiseer de dimensies van de window en bepaal een geschikte image size.
    * We doen dit door de functie determine_img_win_size aan te roepen.
    */
    int img_size = 0;
    int win_w = WINDOW_WIDTH;
    int win_h = WINDOW_HEIGHT;
    if (determine_img_win_size(map_w, map_h, &img_size, &win_w, &win_h) != 0) {
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
    // Dealloceer al het gebruikte geheugen voor de GUI, de game states en de map.
    free_gui();
    free_game_states();
    free_map();
    return 0;
}
