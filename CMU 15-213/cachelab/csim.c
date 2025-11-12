#include "cachelab.h"

#include <stdio.h>

#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>

#include <stdbool.h>

int main(int argc, char* argv[])
{
    int opt;
    bool help = false;
    bool verbose = false;
    int set_bits = 0;
    int asso = 0;
    int block_bits = 0;
    char* filename = NULL;

    while ((opt = getopt(argc, argv, "hvs:E:b:t:")) != -1) {
        switch (opt) {
            case 'h':
                help = true;
                break;
            case 'v':
                verbose = true;
                break;
            case 's':
                set_bits = atoi(optarg);
                break;
            case 'E':
                asso = atoi(optarg);
                break;
            case 'b':
                block_bits = atoi(optarg);
                break;
            case 't':
                filename = optarg;
                break;
            default:
                fprintf(stderr, "Usage: %s [-t nsecs] [-n] name\n",
                        argv[0]);
                exit(EXIT_FAILURE);
            }
        }
    }

    printSummary(0, 0, 0);
    return 0;
}
