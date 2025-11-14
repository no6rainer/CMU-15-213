#include "cachelab.h"

#include <stdio.h>

#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>

#include <stdint.h>

#include <stdbool.h>

typedef struct line {
    bool valid;
    size_t tag;
    size_t lru;
} Line;

typedef struct set {
    Line *lines;
} Set;

typedef struct cache_config {
    size_t set_bits;
    size_t asso;
    size_t block_bits;
    bool verbose;
} Cache_config;

typedef struct cache {
    Set *sets;
    Cache_config config;
    int hits;
    int misses;
    int evictions;
} Cache;

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

    if (help) {
        fprintf(stdout,
                "Usage: %s [-hv] -s <s> -E <E> -b <b> -t <tracefile>\n",
                argv[0]);
        return 0;
    }

    bool missing_or_bad =
        set_bits <= 0 ||
        asso <= 0 ||
        block_bits <= 0 ||
        filename == NULL || *filename == '\0';

    if (missing_or_bad) {
        fprintf(stderr,
                "Usage: %s [-hv] -s <s> -E <E> -b <b> -t <tracefile>\n",
                argv[0]);
        exit(EXIT_FAILURE);
    }

    Cache_config config = {
        .set_bits = set_bits,
        .asso = asso,
        .block_bits = block_bits,
        .verbose = verbose
    };


    size_t set_count = 1u << set_bits;
    size_t line_count = (size_t)asso;
    size_t block_size = 1u << block_bits;

    Set *all_sets = calloc(set_count, sizeof(*all_sets));
    Line *all_lines = calloc(set_count * line_count, sizeof *all_lines);

    for (size_t s = 0; s < set_count; ++s) {
        all_sets[s].lines = all_lines + s * line_count;
    }

    Cache cache = {
        .sets = all_sets,
        .config = config,
        .hits = 0,
        .misses = 0,
        .evictions = 0
    };

    
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    char line[256];
    while (fgets(line, sizeof line, fp) != NULL) {
        if (line[0] == 'I') {
            continue;
        }

        char op = line[1];
        
        size_t addr = 0;
        int size = 0;

        sscanf(line + 3, "%zx,%d", &addr, &size);

        update_cache(op, addr, cache);
        
        printf("op=%c addr=0x%zx\n", op, addr);
    }

    fclose(fp);

    free(all_lines);
    free(all_sets);

    // printSummary(0, 0, 0);
    return 0;
}

void update_cache(char op, size_t addr, Cache cache) {
    static size_t tick = 0;

    size_t set_bits = cache.config.set_bits;
    size_t asso = cache.config.asso;
    size_t block_bits = cache.config.block_bits;

    size_t offset = addr & (1u << block_bits - 1);
    size_t set_index = (addr >> block_bits) & (1u << set_bits - 1);
    size_t tag = addr >> (block_bits + set_bits);

    switch (op) {
        case 'L': 
            for (int i = 0; i < asso; ++i) {
                if (cache.sets[set_index].lines[i].tag == tag) {
                    ++cache.hits;
                    cache.sets[set_index].lines[i].lru = tick;
                    break;
                }
            }
            
    }
    
    ++tick;
}