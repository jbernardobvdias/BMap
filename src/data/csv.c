#define _POSIX_C_SOURCE 200809L
#include "csv.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char* getfield(char* line, int num) {
    const char* tok;
    for (tok = strtok(line, ","); tok && *tok; tok = strtok(NULL, "\n")) {
        if (!--num) {
            return tok;
        }
    }
    return NULL;
}

// Fills out[0..count-1] from the first line of the file.
int parse_csv(const char* path, int* out, int count) {

    // Open the file with the desired path.
    FILE* stream = fopen(path, "r");

    // Check if the file is open.
    if (!stream) {
        perror("Error opening file");
        return -1;
    }

    // The line is massive so I have to allocate 5120 bytes or 5 KB for it before parsing.
    char line[5120];
    int status = -1;

    if (fgets(line, sizeof(line), stream)) {
        int i = 0;
        char* tok = strtok(line, ",\n");
        status = 0;

        while (tok && i < count) {
            char* endptr;
            long val = strtol(tok, &endptr, 10);
            if (endptr == tok) {
                fprintf(stderr, "Field %d is not a number: %s\n", i + 1, tok);
                status = -1;
                break;
            }
            out[i] = (int)val;
            i++;
            tok = strtok(NULL, ",\n");
        }

        if (status == 0 && i < count) {
            fprintf(stderr, "Expected %d fields, got %d\n", count, i);
            status = -1;
        }
    }

    // Close the file
    fclose(stream);
    return status;
}