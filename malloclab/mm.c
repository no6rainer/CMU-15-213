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
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
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
    ""
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

#define GET(ptr) (*(__uint32_t*)(ptr))

#define SET(ptr, val) (*(__uint32_t*)(ptr) = (val))

#define GET_SIZE(ptr) (GET(ptr) & ~0x7)

#define GET_FLAG(ptr) (GET(ptr) & 0x1)

#define SET_FLAG(ptr, flag) (SET(ptr, GET(ptr) | (flag)))

#define GET_PREV_PTR(ptr) ((char*)ptr + 4)

#define GET_NEXT_PTR(ptr) ((char*)ptr + 8)

#define GET_REM_PTR(ptr, size) ((char*)ptr + size + 16)

void** lists = NULL;

static inline int log2_ceil(unsigned int x)
{
    if (x <= 1) {
        return 0;
    }

    return (int)(sizeof(unsigned int) * 8 - __builtin_clz(x - 1));
}

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void) {
    lists = mem_sbrk(16 * sizeof(void*));
    if (lists == (void *)-1) {
        return -1;
    }

    // lists[0] contains 8 bytes blocks, lists[1] contains 16, lists[2] contains 24-32

    memset(lists, 0, 16 * sizeof(void *));
    return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    int malloc_size = ALIGN(size);
    int index = log2_ceil(malloc_size / 8);
    if (index > 15) {
        index = 15;
    }
    
    for (; index <= 15; ++index) {
        void* curr = lists[index];

        while (curr) {
            int block_size = GET_SIZE(curr);

            if (block_size >= malloc_size) {
                SET_FLAG(curr, 0x1);

                // cherry-pick the block from the list
                void* prev = GET_PREV_PTR(curr);
                void* next = GET_NEXT_PTR(curr);

                void* prev_next = GET_NEXT_PTR(prev);
                if (!next) {
                    prev_next = NULL;
                } else {
                    prev_next = next;
                }
                
                if (next) {
                    void* next_prev = GET_PREV_PTR(next);
                    next_prev = prev;
                }

                // slice the block if the remaining space is enough
                if (block_size - malloc_size >= 16) {
                    void* remaining_block = GET_REM_PTR(curr, malloc_size);
                    int remaining_size = GET_SIZE(remaining_block);

                    void* prev = GET_PREV_PTR(curr);
                    void* next = GET_NEXT_PTR(curr);
                }

                CUT_THE_BLOCK_IF_NEEDED();
                ADD_REMAINING_BLOCK_TO_ITS_LIST();
                return PAYLOAD(curr);
            } else {
                curr = NEXT_BLOCK(curr);
            }
        }

        // No fit in this list, switch to next list
    }

    // No fit in all of the lists, have to allocate new heap memory

    int extend_size = max(newsize, PAGE_SIZE);
    void* new_block = extend_heap(extend_size);

    if (new_block == (void *)-1) {
        return NULL;
    } else {
        return new_block;
    }
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;
    
    newptr = mm_malloc(size);
    if (newptr == NULL)
      return NULL;
    copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
    if (size < copySize)
      copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}









