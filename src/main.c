#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include "model/string_list.h"
#include "data/args.h"

struct option long_opts[] = {
    {"port",    required_argument, 0, 'p'},
    {"target",  required_argument, 0, 't'},
    {0, 0, 0, 0}
};

int main(int argc, char *argv[]) {
    StringList ports = {.count = 0};
    StringList ips = {.count = 0}; 

    int opt;
    while ((opt = getopt_long(argc, argv, "p:t", long_opts, NULL)) != -1) {
        switch (opt) {
            case 'p':
                parse_list(optarg, &ports);
                break;
            case 't':
                parse_list(optarg, &ips);
                break;
            case '?':
                printf("There was an error with the flags!");
                return 1;
            default:
                break;
        }
    }

    return 0;
}