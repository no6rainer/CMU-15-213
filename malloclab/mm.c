/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 *
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include "mm.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "ateam",
    /* First member's full name */
    "Harry Bovik",
    /* First member's email address */
    "bovik@cs.cmu.edu",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)

/* rounds up to 8k + 4 */
#define ALIGN_PAYLOAD(size) ((((size) + 3) & ~0x7) + 4)

#define WSIZE 4
#define DSIZE 8
#define PAGE_SIZE 4096

#define ALLOC 1
#define FREE 0

#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define MIN(x, y) ((x) < (y) ? (x) : (y))

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

#define PACK(size, prev_flag, flag) ((size) | (prev_flag << 1) | (flag))

#define GET(ptr) (*(__uint32_t*)(ptr))
#define SET(ptr, val) (*(__uint32_t*)(ptr) = (val))

#define GET_SIZE(ptr) (GET(ptr) & ~0x7)
#define GET_FLAG(ptr) (GET(ptr) & 0x1)
#define GET_PREV_FLAG(ptr) ((GET(ptr) & 0x2) >> 1)

#define SET_FLAG(ptr, flag) (GET(ptr) = (GET(ptr) & ~0x1) | (flag))
#define SET_PREV_FLAG(ptr, prev_flag) (GET(ptr) = (GET(ptr) & ~0x2) | (prev_flag << 1))

#define SET_PACKED(ptr, size, prev_flag, flag) (SET(ptr, PACK(size, prev_flag, flag)))

#define OFFSET(bp, offset) ((char*)(bp) + offset)

#define GET_PREV_BLK(bp) (*(void**)(OFFSET(bp, 4)))
#define GET_NEXT_BLK(bp) (*(void**)(OFFSET(bp, 8)))

#define SET_PREV_PTR(bp, ptr) (GET_PREV_BLK(bp) = (void*)(ptr))
#define SET_NEXT_PTR(bp, ptr) (GET_NEXT_BLK(bp) = (void*)(ptr))

#define GET_FOOTER(bp, size) (OFFSET(bp, size - 4))

#define GET_PAYLOAD(bp) (OFFSET(bp, 4))
#define GET_BLOCK(payload_ptr) (OFFSET(payload_ptr, -4))

void** lists = NULL;

static inline int log2_ceil(unsigned int x) {
    if (x <= 1) {
        return 0;
    }

    return (int)(sizeof(unsigned int) * 8 - __builtin_clz(x - 1));
}

static inline int get_index(int payload_size) {
    int index = log2_ceil((payload_size - 4) / 8);
    if (index > 15) {
        index = 15;
    }
    return index;
}

static inline void init_block(void* block, int size, int prev_flag, int flag) {
    void* header = block;
    SET_PACKED(block, size, prev_flag, flag);

    if (flag == FREE) {
        void* footer = GET_FOOTER(block, size);
        SET_PACKED(footer, size, prev_flag, flag);
    }
}

static inline void insert_block(void* block, int payload_size) {
    int index = get_index(payload_size);

    void* head = lists[index];
    lists[index] = block;
    SET_PREV_PTR(block, NULL);
    SET_NEXT_PTR(block, head);

    if (head) {
        SET_PREV_PTR(head, block);
    }
}

static inline void delete_block(void* block) {
    int payload_size = GET_SIZE(block) - WSIZE; 
    int index = get_index(payload_size);

    void* prev = GET_PREV_BLK(block);
    void* next = GET_NEXT_BLK(block);

    if (!prev) {
        lists[index] = next;
    } else {
        SET_NEXT_PTR(prev, next);
    }   

    if (next) {
        SET_PREV_PTR(next, prev);
    }
}

static inline void* find_fit(int payload_size) {
    int index = get_index(payload_size);
    int malloc_block_size = payload_size + WSIZE;

    for (; index <= 15; ++index) {
        void* curr = lists[index];
        while (curr) {
            if (GET_SIZE(curr) >= malloc_block_size) {
                return curr;
            } else {
                curr = GET_NEXT_BLK(curr);
            }
        }
    }

    return NULL;
}

static inline void set_next_physical_prev_flag(void* block, int offset, int prev_flag) {
    void* next_physical = OFFSET(block, offset);
    
    void* next_physical_header = next_physical;
    SET_PREV_FLAG(next_physical_header, prev_flag);

    if (GET_FLAG(next_physical_header) == FREE) {
        void* next_physical_footer = GET_FOOTER(next_physical, GET_SIZE(next_physical));
        SET_PREV_FLAG(next_physical_footer, prev_flag);
    }
}

static inline void place_block(void* block, int payload_size) {
    void* malloc_block = block;
    int curr_block_size = GET_SIZE(block);
    int malloc_block_size = payload_size + WSIZE;
    int prev_flag = GET_PREV_FLAG(block);

    int rem_block_size = curr_block_size - malloc_block_size;
    
    // slice the block if the remaining space is enough
    if (rem_block_size >= 4 * WSIZE) {
        init_block(malloc_block, malloc_block_size, prev_flag, ALLOC);

        void* rem_block = OFFSET(block, malloc_block_size);
        init_block(rem_block, rem_block_size, ALLOC, FREE);

        int rem_payload_size = rem_block_size - WSIZE;
        insert_block(rem_block, rem_payload_size);
    } else {
        init_block(malloc_block, curr_block_size, prev_flag, ALLOC);
        set_next_physical_prev_flag(block, curr_block_size, ALLOC);
    }
}

static inline void* allocate_block(void* block, int payload_size) {
    delete_block(block);
    place_block(block, payload_size);
    return GET_PAYLOAD(block);
}

static inline void* get_prev_physical(void* block) {
    void* prev_physical_footer = OFFSET(block, -WSIZE);
    int prev_physical_size = GET_SIZE(prev_physical_footer);
    void* prev_physical = OFFSET(block, -prev_physical_size);
    return prev_physical;
}

static inline void* coalesce(void* block) {
    int curr_block_size = GET_SIZE(block);
    int curr_payload_size = curr_block_size - WSIZE;

    int prev_block_flag = GET_PREV_FLAG(block);

    void* next_block = OFFSET(block, curr_block_size);
    int next_block_flag = GET_FLAG(next_block);

    if (prev_block_flag == ALLOC && next_block_flag == ALLOC) {
        insert_block(block, curr_payload_size);

        return block;

    } else if (prev_block_flag == FREE && next_block_flag == ALLOC) {
        void* prev_block = get_prev_physical(block);
        int prev_block_size = GET_SIZE(prev_block);
        int coalesced_size = prev_block_size + curr_block_size;
        int prev_prev_flag = GET_PREV_FLAG(prev_block);
        
        delete_block(prev_block);
        init_block(prev_block, coalesced_size, prev_prev_flag, FREE);
        int coalesced_payload_size = coalesced_size - WSIZE;
        insert_block(prev_block, coalesced_payload_size);

        return prev_block;

    } else if (prev_block_flag == ALLOC && next_block_flag == FREE) {
        int next_block_size = GET_SIZE(next_block);
        int coalesced_size = curr_block_size + next_block_size;
        
        delete_block(next_block);
        init_block(block, coalesced_size, ALLOC, FREE);
        int coalesced_payload_size = coalesced_size - WSIZE;
        insert_block(block, coalesced_payload_size);

        return block;
        
    } else {
        void* prev_block = get_prev_physical(block);
        int prev_block_size = GET_SIZE(prev_block);
        int prev_prev_flag = GET_PREV_FLAG(prev_block);

        int next_block_size = GET_SIZE(next_block);

        int coalesced_size = prev_block_size + curr_block_size + next_block_size;

        delete_block(prev_block);
        delete_block(next_block);
        init_block(prev_block, coalesced_size, prev_prev_flag, FREE);
        int coalesced_payload_size = coalesced_size - WSIZE;
        insert_block(prev_block, coalesced_payload_size);

        return prev_block;
    }
}

static inline void* extend_heap(int extend_size) {
    void* epilogue = OFFSET(mem_heap_hi(), -3);
    int prev_flag = GET_PREV_FLAG(epilogue);

    void* ptr = mem_sbrk(extend_size);
    if (ptr == (void*)-1) {
        return NULL;
    }

    void* new_block = epilogue;
    init_block(new_block, extend_size, prev_flag, FREE);
    int payload_size = extend_size - WSIZE;

    void* new_epilogue = OFFSET(mem_heap_hi(), -3);
    int epilogue_size = 0;
    void* header = new_epilogue;
    SET_PACKED(header, epilogue_size, FREE, ALLOC);

    new_block = coalesce(new_block);
    
    return new_block;
}


/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void) {
    lists = mem_sbrk(PAGE_SIZE);
    if (lists == (void*)-1) {
        return -1;
    }

    // lists[0] contains 8 bytes blocks, lists[1] contains 16, lists[2] contains
    // 24-32

    int lists_size = 16 * sizeof(void*);
    memset(lists, 0, lists_size);

    int padding_size = WSIZE;

    void* prologue = OFFSET(lists, lists_size + padding_size);
    int prologue_size = 2 * WSIZE;
    init_block(prologue, prologue_size, ALLOC, ALLOC);

    void* epilogue = OFFSET(mem_heap_hi(), -3);
    int epilogue_size = 0;
    void* header = epilogue;
    SET_PACKED(header, epilogue_size, FREE, ALLOC);

    void* block = OFFSET(prologue, 2 * WSIZE);
    int block_size = PAGE_SIZE - lists_size - padding_size - prologue_size - WSIZE;
    init_block(block, block_size, ALLOC, FREE);
    int payload_size = block_size - WSIZE;
    insert_block(block, payload_size);

    return 0;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void* mm_malloc(size_t size) {
    int malloc_payload_size = ALIGN_PAYLOAD(size);
    if (malloc_payload_size < 3 * WSIZE) {
        malloc_payload_size = 3 * WSIZE;
    }
    int index = get_index(malloc_payload_size);
    int malloc_block_size = malloc_payload_size + WSIZE;

    void* block = find_fit(malloc_payload_size);
    if (block) {
        return allocate_block(block, malloc_payload_size);
    }

    // No fit in all of the lists, have to allocate new heap memory
    int extend_size = MAX(malloc_block_size, PAGE_SIZE);
    void* new_block = extend_heap(extend_size);
    if (new_block) {
        return allocate_block(new_block, malloc_payload_size);
    } else {
        return NULL;
    }
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void* ptr) {
    void* block = GET_BLOCK(ptr);
    int size = GET_SIZE(block);
    int prev_flag = GET_PREV_FLAG(block);

    init_block(block, size, prev_flag, FREE);
    set_next_physical_prev_flag(block, size, FREE);

    coalesce(block);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void* mm_realloc(void* ptr, size_t size) {
    if (!ptr) {
        return mm_malloc(size);
    }

    if (size == 0) {
        mm_free(ptr);
        return NULL;
    }

    void* old_payload = ptr;
    void* old_block = GET_BLOCK(old_payload);
    int old_block_size = GET_SIZE(old_block);

    int new_payload_size = ALIGN_PAYLOAD(size);
    if (new_payload_size < 3 * WSIZE) {
        new_payload_size = 3 * WSIZE;
    }
    int new_block_size = new_payload_size + WSIZE;
    
    if (old_block_size >= new_block_size) {
        place_block(old_block, new_payload_size);
        return old_payload;

    } else {
        void* next_physical_block = OFFSET(old_block, old_block_size);
        int next_physical_flag = GET_FLAG(next_physical_block);
        int next_physical_size = GET_SIZE(next_physical_block);
        int coalesced_size = old_block_size + next_physical_size;
        
        if (next_physical_flag == FREE && coalesced_size >= new_block_size) {
            delete_block(next_physical_block);

            int prev_flag = GET_PREV_FLAG(old_block);
            init_block(old_block, coalesced_size, prev_flag, ALLOC);

            place_block(old_block, new_payload_size);
            return old_payload;
        }

        int is_at_end = 0;
        if (next_physical_size == 0) {
            is_at_end = 1;
        } else if (next_physical_flag == FREE && GET_SIZE(OFFSET(next_physical_block, next_physical_size)) == 0) {
            is_at_end = 1;
        }

        if (is_at_end) {
            int available_size = old_block_size + (next_physical_flag == FREE ? next_physical_size : 0);
            int extend_size = new_block_size - available_size;
            extend_size = MAX(extend_size, PAGE_SIZE);

            void* extended_block = extend_heap(extend_size);
            if (extended_block) {
                delete_block(extended_block);

                int extended_block_size = GET_SIZE(extended_block);
                int coalesced_size = old_block_size + extended_block_size;

                int prev_flag = GET_PREV_FLAG(old_block);
                init_block(old_block, coalesced_size, prev_flag, ALLOC);

                place_block(old_block, new_payload_size);
                return old_payload;
            }
        }
    }

    void* new_payload = mm_malloc(size);
    if (!new_payload) {
        return NULL;
    }

    int copy_size = old_block_size - WSIZE;
    memcpy(new_payload, old_payload, copy_size);
    mm_free(old_payload);

    return new_payload;
}
