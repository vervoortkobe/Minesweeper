#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "filehandler.h"

int read_file(const char *filename, char ***out_lines, int *out_rows) {
    if (!filename || !out_lines || !out_rows) return -1;
    FILE *f = fopen(filename, "r");
    if (!f) return -1;
    int cap = 64;
    char **lines = malloc(cap * sizeof(char*));
    if (!lines) { fclose(f); return -1; }
    char buffer[4096];
    int rows = 0;
    while (fgets(buffer, sizeof(buffer), f)) {
        int len = strlen(buffer);
        while (len > 0 && (buffer[len-1] == '\n' || buffer[len-1] == '\r')) buffer[--len] = '\0';
        if (rows >= cap) {
            cap *= 2;
            char **tmp = realloc(lines, cap * sizeof(char*));
            if (!tmp) {
                for (int i = 0; i < rows; ++i) free(lines[i]);
                free(lines);
                fclose(f);
                return -1;
            }
            lines = tmp;
        }
        char *line = malloc(len + 1);
        if (!line) {
            for (int i = 0; i < rows; ++i) free(lines[i]);
            free(lines);
            fclose(f);
            return -1;
        }
        memcpy(line, buffer, len + 1);
        lines[rows++] = line;
    }
    fclose(f);
    *out_lines = lines;
    *out_rows = rows;
    return 0;
}

void free_lines(char **lines, int rows) {
    if (!lines) return;
    for (int i = 0; i < rows; ++i) if (lines[i]) free(lines[i]);
    free(lines);
}

/*
* Sla het speelveld op in een bestand via de 's' key.
* Zie HOC Slides 4_input_output:
* - dia 16 voor FILE
* - dia 57 voor fputc
* - dia 58 voor fopen
*/
int save_field(const char *filename, int w, int h, const char *map, const unsigned char *flagged, const unsigned char *uncovered) {
    if (!filename || !map) return -1;
    FILE *out = fopen(filename, "w");
    if (!out) return -1;
    for (int x = 0; x < w; ++x) {
        for (int y = 0; y < h; ++y) {
            fputc(map[y * w + x], out);
            fputc(' ', out);
        }
        fputc('\n', out);
    }
    fputc('\n', out);
    for (int x = 0; x < w; ++x) {
        for (int y = 0; y < h; ++y) {
            int i = y * w + x;
            if (flagged && flagged[i]) { fputc('F', out); fputc(' ', out); }
            else if (uncovered && uncovered[i]) { fputc('U', out); fputc(' ', out); }
            else { fputc('#', out); fputc(' ', out); }
        }
        fputc('\n', out);
    }
    fclose(out);
    return 0;
}
