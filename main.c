#include <stdio.h>
#include "GUI.h"
#include "args.h"
#include "map.h"

int SDL_main(int argc, char *argv[]) {
    /* Command-line modes supported:
     * -f <file>         Load saved map from file and continue
     * -w <width> -h <height> -m <mines>
     *                   Create a new map with given width/height/mines (flags may appear in any order)
     */
    /* parse CLI args via args.c */
    CLIArgs cli;
    if (parse_cli_args(argc, argv, &cli) != 0) {
        fprintf(stderr, "%s\n", cli.error_msg);
        return 1;
    }

    if (cli.load_file) {
        if (load_game_file(cli.load_file) != 0) {
            fprintf(stderr, "Failed to load map from %s\n", cli.load_file);
            return 1;
        }
    } else if (cli.w > 0 && cli.h > 0 && cli.m >= 0) {
        if (init_map(cli.w, cli.h, cli.m) != 0) {
            fprintf(stderr, "Failed to initialize map %dx%d\n", cli.w, cli.h);
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
