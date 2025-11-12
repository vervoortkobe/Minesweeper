// filehandler.c - simple file utilities for Minesweeper
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "filehandler.h"

void writeFile(const char* filename, const char* data) {
    FILE* file = fopen(filename, "w");
    if (file != NULL) {
        fputs(data, file);
        fclose(file);
    } else {
        perror("Error opening file for writing");
    }
}

int read_lines(const char *filename, char ***out_lines, int *out_rows) {
    if (!filename || !out_lines || !out_rows) return -1;
    FILE *f = fopen(filename, "r");
    if (!f) return -1;
    size_t cap = 64;
    char **lines = (char**)malloc(cap * sizeof(char*));
    if (!lines) { fclose(f); return -1; }
    char buffer[4096];
    int rows = 0;
    while (fgets(buffer, sizeof(buffer), f)) {
        size_t len = strlen(buffer);
        while (len > 0 && (buffer[len-1] == '\n' || buffer[len-1] == '\r')) { buffer[--len] = '\0'; }
        if (rows >= (int)cap) {
            cap *= 2;
            char **tmp = (char**)realloc(lines, cap * sizeof(char*));
            if (!tmp) { for (int i=0;i<rows;i++) free(lines[i]); free(lines); fclose(f); return -1; }
            lines = tmp;
        }
        lines[rows++] = strdup(buffer);
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

int fh_save_field_with_state(const char *filename, int w, int h, const char *map, const unsigned char *flagged, const unsigned char *uncovered) {
    if (!filename || !map) return -1;
    FILE *out = fopen(filename, "w");
    if (!out) return -1;
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            fputc(map[y * w + x], out);
            fputc(' ', out);
        }
        fputc('\n', out);
    }
    fputc('\n', out);
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            int idx = y * w + x;
            if (flagged && flagged[idx]) { fputc('F', out); fputc(' ', out); }
            else if (uncovered && uncovered[idx]) { fputc('U', out); fputc(' ', out); }
            else { fputc('#', out); fputc(' ', out); }
        }
        fputc('\n', out);
    }
    fclose(out);
    return 0;
}
