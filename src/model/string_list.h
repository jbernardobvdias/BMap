#ifndef STRING_LIST_H_
#define STRING_LIST_H_

#define MAX_ITEMS 64
#define MAX_LEN 64

typedef struct {
    char items[MAX_ITEMS][MAX_LEN];
    int count;
} StringList;

#endif