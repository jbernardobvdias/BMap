#ifndef STRING_LIST_H
#define STRING_LIST_H

#define MAX_ITEMS 64
#define MAX_LEN 64

typedef struct {
    char items[MAX_ITEMS][MAX_LEN];
    int count;
} StringList;

#endif