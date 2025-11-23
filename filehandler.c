#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "filehandler.h"

/*
* Deze functie leest alle lijnen van een bestand uit en slaat deze op in een dynamisch gealloceerde array van char pointers.
* De gelezen lijnen worden teruggegeven via de out_lines en het aantal lijnen via out_count.
* Zie HOC Slides 4_input_output:
* - dia 16 voor FILE
* - dia 58 voor fopen
* - dia 18 voor fclose
* - dia 31 voor fgets
* Zie HOC Slides 3b_structures:
* - dia 37 voor malloc
* - dia 49 voor realloc
* Zie HOC Slides 3c_advanced:
* - dia 49 voor memcpy
*/
int read_lines(const char *filename, char ***out_lines, int *out_count) {
    if (!filename || !out_lines || !out_count) return -1;
    FILE *f = fopen(filename, "r");
    if (!f) return -1;
    // We alloceren geheugen voor het inlezen van 22 lijnen van char arrays (pointers).
    int cap = 22; // max aantal lijnen die gelezen worden
    char **lines = malloc(cap * sizeof(char*));
    if (!lines) {
        fclose(f);
        return -1;
    }

    // buffer voor het opslaan van een lijn
    // elke lijn heeft max 22 chars + null terminator
    char buffer[23];
    // aantal gelezen lijnen
    int count = 0;
    // We lezen de lijnen uit het bestand via fgets.
    while (fgets(buffer, sizeof(buffer), f)) {
        /*
        * In een bestand zijn telkens 2 speelvelden opgeslagen (covered en uncovered).
        * Deze speelvelden worden gescheiden door een lege lijn.
        * Elke lijn van een speelveld heeft een newline character op het einde.
        * Om op de juiste manier een lijn te lezen, moeten we dus de eind terminators verwijderen.
        */
        int len = strlen(buffer);
        while (len > 0 && (buffer[len - 1] == '\n' || buffer[len-1] == '\r'))
            buffer[--len] = '\0';

        // We alloceren geheugen voor het opslaan van de gelezen lijn.
        char *line = malloc(len + 1); // + 1 voor de null terminator
        if (!line) {
            for (int i = 0; i < count; ++i) {
                free(lines[i]);
            }
            free(lines);
            fclose(f);
            return -1;
        }

        // We kopiëren de gelezen lijn naar het gealloceerde geheugen van line.
        memcpy(line, buffer, len + 1);
        // We slaan de lijn op in de lines array.
        lines[count++] = line;
    }

    fclose(f);
    *out_lines = lines;
    *out_count = count;

    return 0;
}

// Dealloceer het geheugen gebruikt voor het inlezen van de "lines" uit een bestand.
void free_lines(char **lines, int count) {
    if (!lines) return;
    for (int i = 0; i < count; ++i) {
        if (lines[i]) 
            free(lines[i]);
    }
    free(lines);
}

/*
* Sla het speelveld op in een bestand via de 's' key.
* Zie HOC Slides 4_input_output:
* - dia 16 voor FILE
* - dia 58 voor fopen
* - dia 57 voor fputc
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
            if (flagged && flagged[i]) {
                fputc('F', out);
                fputc(' ', out);
            }
            else if (uncovered && uncovered[i]) {
                fputc('U', out);
                fputc(' ', out);
            }
            else {
                fputc('#', out);
                fputc(' ', out);
            }
        }
        fputc('\n', out);
    }
    fclose(out);
    return 0;
}
