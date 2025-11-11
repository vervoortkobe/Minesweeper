// Simple CLI argument parser for minesweeper
#ifndef MINESWEEPER_ARGS_H
#define MINESWEEPER_ARGS_H

#include <stddef.h>

typedef struct {
    const char *load_file; // -f <file>
    int w; // -w width
    int h; // -h height
    int m; // -m mines
    int error; // non-zero if parse failed
    char error_msg[256];
} CLIArgs;

// Parse argc/argv into out. Returns 0 on success, non-zero on error (out->error set).
int parse_cli_args(int argc, char *argv[], CLIArgs *out);

#endif // MINESWEEPER_ARGS_H
//
// Created by vervo on 14/10/2025.
//

#ifndef MINESWEEPER_ARGS_H
#define MINESWEEPER_ARGS_H

#endif //MINESWEEPER_ARGS_H