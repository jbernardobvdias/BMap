#include "args.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Code to parse list of arguments
void parse_list(const char *input, StringList *list) {
    list->count = 0;
    if (input == NULL || strlen(input) == 0) {
        return;
    }

    char buf[1024];
    strncpy(buf, input, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    char *token = strtok(buf, ",");
    while (token != NULL && list->count < MAX_ITEMS) {
        while (*token == ' ') token++;
        strncpy(list->items[list->count], token, MAX_LEN - 1);
        list->items[list->count][MAX_LEN - 1] = '\0';
        list->count++;
        token = strtok(NULL, ",");
    }
}