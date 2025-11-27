#ifndef MINESWEEPER_ARGS_H
#define MINESWEEPER_ARGS_H

/*
 * We gebruiken een struct om de argumenten via functies door te geven.
 * Zie HOC Slides 3b_structures dia 26 voor typedef & struct.
 */
typedef struct {
    const char *file; // -f <bestand>
    int w; // -w <breedte>
    int h; // -h <hoogte>
    int m; // -m <mijnen>
} Args;

int parse_args(int argc, char *argv[], Args *args);

#endif // MINESWEEPER_ARGS_H