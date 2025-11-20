#ifndef MINESWEEPER_ARGS_H
#define MINESWEEPER_ARGS_H

/*
* We gebruiken en struct om de argumenten via functies door te geven.
* Zie HOC Slides 3b_structures dia 26 voor typedef & struct.
*/
typedef struct {
    const char *load_file; // -f <bestand>
    int w; // -w <breedte>
    int h; // -h <hoogte>
    int m; // -m <mijnen>
    int error; // non-zero if parse failed
    char error_msg[256];
} CLIArgs;

int parse_args(int argc, char *argv[], CLIArgs *out);

#endif // MINESWEEPER_ARGS_H