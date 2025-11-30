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

static void access_cache(size_t addr, Cache* cache);
void update_cache(char op, size_t addr, Cache* cache);
void printSummary(int hits, int misses, int evictions);

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
    // size_t block_size = 1u << block_bits;

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

        update_cache(op, addr, &cache);
        
        printf("op=%c addr=0x%zx\n", op, addr);
    }

    fclose(fp);

    free(all_lines);
    free(all_sets);

    printSummary(cache.hits, cache.misses, cache.evictions);
    return 0;
}

static void access_cache(size_t addr, Cache* cache) {
    static size_t tick = 0;

    size_t set_bits = cache->config.set_bits;
    size_t asso = cache->config.asso;
    size_t block_bits = cache->config.block_bits;

    // size_t offset = addr & ((1u << block_bits) - 1);
    size_t set_index = (addr >> block_bits) & ((1u << set_bits) - 1);
    size_t tag = addr >> (block_bits + set_bits);

    Set* curr_set = &cache->sets[set_index];

    int hit_index = -1;
    int empty_index = -1;
    int victim_index = 0;
    size_t oldest = curr_set->lines[0].lru;

    for (int i = 0; i < asso; ++i) {
        Line* line = &curr_set->lines[i];
        if (line->valid && line->tag == tag) {
            hit_index = i;
            break;
        }
        if (!line->valid && empty_index == -1) {
            empty_index = i;
        }
        if (line->valid && line->lru < oldest) {
            victim_index = i;
            oldest = line->lru;
        }
    }

    if (hit_index != -1) {
        ++cache->hits;
        curr_set->lines[hit_index].lru = tick;
    } else if (empty_index != -1) {
        ++cache->misses;
        curr_set->lines[empty_index].valid = true;
        curr_set->lines[empty_index].tag = tag;
        curr_set->lines[empty_index].lru = tick;
    } else {
        ++cache->misses;
        ++cache->evictions;
        curr_set->lines[victim_index].tag = tag;
        curr_set->lines[victim_index].lru = tick;
    }

    ++tick;
}

void update_cache(char op, size_t addr, Cache* cache) {
    switch (op) {
        case 'L':
            access_cache(addr, cache);
            break;

        case 'S':
            access_cache(addr, cache);
            break;

        case 'M':
            access_cache(addr, cache);
            access_cache(addr, cache);
            break;
    }
}

// void printSummary(int hits, int misses, int evictions) {
//     printf("hits:%d misses:%d evictions:%d\n", hits, misses, evictions);
// }