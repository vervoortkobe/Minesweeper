//
// Created by vervo on 14/10/2025.
//

#include <stdio.h>

void writeFile(const char* filename, const char* data) {
    FILE* file = fopen(filename, "w");
    if (file != NULL) {
        fputs(data, file);
        fclose(file);
    } else {
        perror("Error opening file for writing");
    }
}
