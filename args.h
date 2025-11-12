#ifndef MINESWEEPER_ARGS_H
#define MINESWEEPER_ARGS_H

typedef struct {
    const char *load_file; // -f <bestand>
    int w; // -w <breedte>
    int h; // -h <hoogte>
    int m; // -m <bommen>
    int error; // non-zero if parse failed
    char error_msg[256];
} CLIArgs;

int parse_cli_args(int argc, char *argv[], CLIArgs *out);

#endif // MINESWEEPER_ARGS_H