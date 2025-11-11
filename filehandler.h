// filehandler.h - small utilities for reading/writing save files
#ifndef MINESWEEPER_FILEHANDLER_H
#define MINESWEEPER_FILEHANDLER_H

#include <stdbool.h>

// Simple wrapper to write a string to a file (keeps legacy name)
void writeFile(const char* filename, const char* data);

// Check if a file exists (returns 1 if exists, 0 otherwise)
int fh_file_exists(const char *filename);

// Read all lines from a file. On success, *out_lines is set to an allocated array of 'rows' strdup'ed strings.
// Caller must free with fh_free_lines.
int fh_read_lines(const char *filename, char ***out_lines, int *out_rows);
void fh_free_lines(char **lines, int rows);

// Save a field and its play state (map, blank line, play-state tokens) into filename.
// map should be a linear buffer of size w*h (row-major).
// flagged and uncovered may be NULL; if provided they are arrays of unsigned char (0/1) length w*h.
int fh_save_field_with_state(const char *filename, int w, int h, const char *map, const unsigned char *flagged, const unsigned char *uncovered);

#endif // MINESWEEPER_FILEHANDLER_H
