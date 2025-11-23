#ifndef MINESWEEPER_FILEHANDLER_H
#define MINESWEEPER_FILEHANDLER_H

int read_lines(const char *filename, char ***out_lines, int *out_count);
void free_lines(char **lines, int count);
int save_field(const char *filename, int w, int h, const char *map, const unsigned char *flagged, const unsigned char *uncovered);

#endif // MINESWEEPER_FILEHANDLER_H
