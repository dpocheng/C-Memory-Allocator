/*
 * Ian Stephenson   - 44419093
 * Pok On Cheng     - 75147306
 * Sung Mo Koo      - 51338217
 * Cassie Liu       - 52504836
 */

/*
 * Simple, 32-bit and 64-bit clean allocator based on implicit free
 * lists, first fit placement, and boundary tag coalescing, as described
 * in the CS:APP2e text. Blocks must be aligned to doubleword (8 byte)
 * boundaries. Minimum block size is 16 bytes.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "config.h"
#include "mm.h"
#include "memlib.h"

/* $begin mallocmacros */
/* Basic constants and macros */
#define WSIZE       4       /* Word and header/footer size (bytes) */ //line:vm:mm:beginconst
#define DSIZE       8       /* Doubleword size (bytes) */
#define CHUNKSIZE  (1<<12)  /* Extend heap by this amount (bytes) */  //line:vm:mm:endconst

#define MAX(x, y) ((x) > (y)? (x) : (y))

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc)  ((size) | (alloc)) //line:vm:mm:pack

/* Read and write a word at address p */
#define GET(p)       (*(unsigned int *)(p))            //line:vm:mm:get
#define PUT(p, val)  (*(unsigned int *)(p) = (val))    //line:vm:mm:put

/* Read the size and allocated fields from address p */
#define GET_SIZE(p)  (GET(p) & ~0x7)                   //line:vm:mm:getsize
#define GET_ALLOC(p) (GET(p) & 0x1)                    //line:vm:mm:getalloc

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp)       ((char *)(bp) - WSIZE)                      //line:vm:mm:hdrp
#define FTRP(bp)       ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE) //line:vm:mm:ftrp

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp)  ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE))) //line:vm:mm:nextblkp
#define PREV_BLKP(bp)  ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE))) //line:vm:mm:prevblkp
/* $end mallocmacros */

/* Global variables */
static char *heap_listp = 0;  /* Pointer to first block */
static unsigned int numberOfBlocks = 0;
static void *blockArray[1000];

/*
 * If NEXT_FIT defined use next fit search, else use first fit search (this is defined or undefined in shellex.c in the
 * builtin_command function. Macros are declared as global (<--- I think they are, need more research -Ian)
 */
#ifdef NEXT_FIT
static char *rover;           /* Next fit rover */
#endif

/* Function prototypes for internal helper routines */
static void *extend_heap(size_t words);
static void place(void *bp, size_t asize);
static void *find_fit(size_t asize);
static void *coalesce(void *bp);
static void printblock(void *bp);
static void checkheap(int verbose);
static void checkblock(void *bp);

/*
 * mm_init - Initialize the memory manager
 */
/* $begin mminit */
int mm_init(void) {
    /* Create the initial empty heap */
    if ((heap_listp = mem_sbrk(4*WSIZE)) == (void *)-1) { //line:vm:mm:begininit
        return -1;
    }
    
    /*
     * Creates the special prologue block (only header and footer with 8 bytes each-- no payload) that never get freed
     * from the heap. Marks the beginning of the heap.
     */
    PUT(heap_listp, 0);                          /* Alignment padding */
    PUT(heap_listp + (1*WSIZE), PACK(DSIZE, 1)); /* Prologue header */
    PUT(heap_listp + (2*WSIZE), PACK(DSIZE, 1)); /* Prologue footer */
    
    /*
     * Creates the special epilogue block header that marks the end of the heap. All blocks between the prologue block
     * and epilogue block are considered the heap.
     */
    PUT(heap_listp + (3*WSIZE), PACK(0, 1));     /* Epilogue header */
    
    /*
     * Always points to the prologue block. The end of the prologue block is the start of the heap.
     */
    heap_listp += (2*WSIZE);                     //line:vm:mm:endinit
    /* $end mminit */

    #ifdef NEXT_FIT
        rover = heap_listp;
    #endif
    /* $begin mminit */

    /* Extend the empty heap with a free block of CHUNKSIZE bytes */
    if (extend_heap(CHUNKSIZE/WSIZE) == NULL) {
        return -1;
    }
    return 0;
}
/* $end mminit */

/*
 * mm_malloc - Allocate a block with at least size bytes of payload
 */
/* $begin mmmalloc */
void *mm_malloc(size_t size) {
    size_t extendsize; /* Amount to extend heap if no fit */
    char *bp;

	/* $end mmmalloc */
    if (heap_listp == 0) {
		mm_init();
    }
	/* $begin mmmalloc */
    /* Ignore spurious requests */
    if (size == 0) {
		return NULL;
	}

    /* Search the free list for a fit */
    if ((bp = find_fit(size)) != NULL) {  //line:vm:mm:findfitcall
		place(bp, size);                  //line:vm:mm:findfitplace
		numberOfBlocks++;
		blockArray[numberOfBlocks] = bp;
		printf("%d\n", numberOfBlocks);
		return bp;
    }
    
    /* No fit found. Get more memory and place the block */
    extendsize = MAX(size,CHUNKSIZE);                 //line:vm:mm:growheap1
    if ((bp = extend_heap(extendsize/WSIZE)) == NULL) {
		return NULL;                                  //line:vm:mm:growheap2
	}
	place(bp, size);
	numberOfBlocks++;
	blockArray[numberOfBlocks] = bp;
	printf("%d\n", numberOfBlocks);
    return bp;
}
/* $end mmmalloc */

/*
 * mm_free - Free a block
 */
/* $begin mmfree */
void mm_free(void *bp) {
    /* $end mmfree */
    if(bp == 0) {
        return;
    }
    
    /* $begin mmfree */
    size_t size = GET_SIZE(HDRP(bp));
    /* $end mmfree */
    if (heap_listp == 0) {
        mm_init();
    }
    /* $begin mmfree */
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    coalesce(bp);
}
/* $end mmfree */
/*
 * coalesce - Boundary tag coalescing. Return ptr to coalesced block
 */
/* $begin mmfree */
static void *coalesce(void *bp) {
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));
    
    if (prev_alloc && next_alloc) {            /* Case 1 */
        return bp;
    }
    
    else if (prev_alloc && !next_alloc) {      /* Case 2 */
		size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
		PUT(HDRP(bp), PACK(size, 0));
		PUT(FTRP(bp), PACK(size,0));
    }
    
    else if (!prev_alloc && next_alloc) {      /* Case 3 */
		size += GET_SIZE(HDRP(PREV_BLKP(bp)));
		PUT(FTRP(bp), PACK(size, 0));
		PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
		bp = PREV_BLKP(bp);
    }
    
    else {                                     /* Case 4 */
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    /* $end mmfree */
#ifdef NEXT_FIT
    /* Make sure the rover isn't pointing into the free block */
    /* that we just coalesced */
    if ((rover > (char *)bp) && (rover < NEXT_BLKP(bp))) {
        rover = bp;
    }
#endif
    /* $begin mmfree */
    return bp;
}
/* $end mmfree */

/*
 * mm_realloc - Naive implementation of realloc
 */
void *mm_realloc(void *ptr, size_t size) {
    size_t oldsize;
    void *newptr;
    
    /* If size == 0 then this is just free, and we return NULL. */
    if (size == 0) {
        mm_free(ptr);
        return 0;
    }
    
    /* If oldptr is NULL, then this is just malloc. */
    if (ptr == NULL) {
        return mm_malloc(size);
    }
    
    newptr = mm_malloc(size);
    
    /* If realloc() fails the original block is left untouched  */
    if (!newptr) {
        return 0;
    }
    
    /* Copy the old data. */
    oldsize = GET_SIZE(HDRP(ptr));
    if (size < oldsize) oldsize = size; {
        memcpy(newptr, ptr, oldsize);
    }
    /* Free the old block. */
    mm_free(ptr);
    
    return newptr;
}

/*
 * checkheap - We don't check anything right now.
 */
void mm_checkheap(int verbose) {
}

/*
 * printblocklist - it will print the blocklist with format as requirements
 */
void mm_printblocklist(void) {
    char *bp = heap_listp;
    int count = 0;
    int num = 0;
    printf("Size\tAllocated\tStart\t\tEnd\n");
    for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) {
        size_t hsize;
        hsize = GET_SIZE(HDRP(bp));
        if (hsize != 0) {
            num++;
        }
    }
    for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) {
        size_t halloc;
        halloc = GET_ALLOC(HDRP(bp));
        count++;
        if (count == 1 || count == num) {
            // do nothing
        }
        else {
            /* Add 4 to footer, because FTRP just returns the start of the footer.
             * Add 4 to factor in the entire length of the footer */
            printf("%d\t%s\t\t%p\t%p\n", (int)((FTRP(bp) + 4)-HDRP(bp)), (halloc ? "yes" : "no"), HDRP(bp), (FTRP(bp) + 4));
        }
    }
    
}

/*
 * blocknumbertoblock - this function is to convert block number to corresponding block
 */
char* mm_blocknumbertoblock(int blocknumber) {
    char* bp = heap_listp;
    int count = 0;
    if (blocknumber <= numberOfBlocks) {
        for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) {
            count++;
            if (count == blocknumber+1) {
                //size_t halloc;
                //halloc = GET_ALLOC(HDRP(bp));
                // printf("%d\t%s\t\t%p\t%p\n", (int)(FTRP(bp)-HDRP(bp)+1), (halloc ? "yes" : "no"), HDRP(bp), FTRP(bp));
                break;
            }
        }
    }
    else {
        printf("\"%d\": Invalid block number\n", blocknumber);
    }
    return bp;
}

/*
 * getpayloadsize - return the number of bytes with the payload size and padding size
 */
unsigned long mm_getpayloadsize(int blocknumber) {
    unsigned long totalpayloadsize = 0;
    char *bp = heap_listp;
    int i = 0;
    
    while(GET_SIZE(HDRP(bp)) > 0) {
        if(i == blocknumber) {
            /* Get total size of the block */
            totalpayloadsize = FTRP(bp) - HDRP(bp) + 1;             // +1 because addresses are zero-based
            
            /* Subract the size of the header and footer from the total size of the block */
            totalpayloadsize -= 2*WSIZE;
            break;
        }
        
        /* Iterate to next block */
        bp = NEXT_BLKP(bp);
        i++;
    }
    return totalpayloadsize;
}

/*
 * writeheap - Writes a character to the payload space of an allocated block n times
 */
void mm_writeheap(int blocknumber, char character, int numberOfRepetitions) {
    char* bp = heap_listp;
    int i = 0;
    
    /* Navigate to block */
    while (GET_SIZE(HDRP(bp)) > 0) {
        if (blocknumber == i) {
            bp = HDRP(bp) + WSIZE;          // Add header size to the header to get to the payload
            break;
        }
        
        /* Iterate to next block */
        bp = NEXT_BLKP(bp);
        i++;
    }
    
    /* Write to payload */
    int j;
    for(j = 0; j < numberOfRepetitions; j++) {
        *(bp + j) = character;
    }
    *(bp + j + 1) = '\0';                    // If need to null-terminate characters uncomment this line
}

/*
 * printheap - Prints out the fist numberOfBytesToRead bytes from blocknumber block
 */
void mm_printheap(int blocknumber, int numberOfBytesToRead) {
    char* bp = heap_listp;
    int i = 0;
    
    /* Navigate to block */
    while (GET_SIZE(HDRP(bp)) > 0) {
        if (blocknumber == i) {
            bp = HDRP(bp) + WSIZE;          // Add header size to the header to get to the payload
            break;
        }
        
        /* Iterate to next block */
        bp = NEXT_BLKP(bp);
        i++;
    }
    
    int k = 0;
    for (k = 0; k < numberOfBytesToRead; k++) {
        if (*(bp + k) == '\0') {
            printf("\"%d\": Invalid number of bytes to read\n", numberOfBytesToRead);
            return;
        }
    }
    
    /* Print out first numberOfBytesToRead from block */
    int j;
    for(j = 0; j < numberOfBytesToRead; j++) {
        printf("%c", *(bp + j));
    }
    printf("\n");
}

/*
 * freebufferinblock - delete character that write inside the block
 */
void mm_freebufferinblock(char* bp) {
    int j;
    unsigned long totalpayloadsize = FTRP(bp) - HDRP(bp) + 1;
    totalpayloadsize -= 2*WSIZE;
    int numberOfCharacterInBlock = (int)totalpayloadsize / 2;
    for(j = 0; j < numberOfCharacterInBlock; j++) {
        *(bp + j) = '\0';
    }
}

/*
 * getBlockArrayElement - Provides an interface to read a blockArray element in outside modules
 */
void *getBlockArrayElement(int blockNumber) {
	return blockArray[blockNumber];
}

/*
 * The remaining routines are internal helper routines
 */

/*
 * extend_heap - Extend heap with free block and return its block pointer
 */
/* $begin mmextendheap */
static void *extend_heap(size_t words) {
    char *bp;
    size_t size;

    /* Allocate an even number of words to maintain alignment */
    size = (words % 2) ? (words+1) * WSIZE : words * WSIZE; //line:vm:mm:beginextend
    if ((long)(bp = mem_sbrk(size)) == -1) {
		return NULL;                                        //line:vm:mm:endextend
	}
    /* Initialize free block header/footer and the epilogue header */
    PUT(HDRP(bp), PACK(size, 0));         /* Free block header */   //line:vm:mm:freeblockhdr
    PUT(FTRP(bp), PACK(size, 0));         /* Free block footer */   //line:vm:mm:freeblockftr
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); /* New epilogue header */ //line:vm:mm:newepihdr
    
    /* Coalesce if the previous block was free */
    return coalesce(bp);                                          	//line:vm:mm:returnblock
}
/* $end mmextendheap */

/*
 * place - Place block of asize bytes at start of free block bp
 *         and split if remainder would be at least minimum block size
 */
/* $begin mmplace */
/* $begin mmplace-proto */
static void place(void *bp, size_t asize) {
    /* $end mmplace-proto */
    size_t csize = GET_SIZE(HDRP(bp));
    
    if ((csize - asize) >= (2*DSIZE)) {
		PUT(HDRP(bp), PACK(asize, 1));
		PUT(FTRP(bp), PACK(asize, 1));
		bp = NEXT_BLKP(bp);
		PUT(HDRP(bp), PACK(csize-asize, 0));
		PUT(FTRP(bp), PACK(csize-asize, 0));
    }
    else {
		PUT(HDRP(bp), PACK(csize, 1));
		PUT(FTRP(bp), PACK(csize, 1));
    }
}
/* $end mmplace */

/*
 * find_fit - Find a fit for a block with asize bytes
 */
/* $begin mmfirstfit */
/* $begin mmfirstfit-proto */
static void *find_fit(size_t asize) {
/* $end mmfirstfit-proto */
/* $end mmfirstfit */
	#ifdef NEXT_FIT
		/* Next fit search */
		char *oldrover = rover;
        char *bp;
        int index = 0;
        size_t size = ((FTRP(rover) + 4)-HDRP(rover));

        int i = 0;
        for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) {
            if ((!GET_ALLOC(rover)) && (((FTRP(bp) + 4)-HDRP(bp)) <= size)) {
                size = ((FTRP(bp) + 4)-HDRP(bp));
                index = i;
            }
            i++;
        }
        return blockArray[index];
    
		/* Search from the rover to the end of list */
		/*for ( ; GET_SIZE(HDRP(rover)) > 0; rover = NEXT_BLKP(rover)) {
			if (!GET_ALLOC(HDRP(rover)) && (asize <= GET_SIZE(HDRP(rover)))) {
				return rover;
			}
		}*/

		/* search from start of list to old rover */
		/*for (rover = heap_listp; rover < oldrover; rover = NEXT_BLKP(rover)) {
			if (!GET_ALLOC(HDRP(rover)) && (asize <= GET_SIZE(HDRP(rover)))) {
				return rover;
			}
		}

		return NULL;*/  /* no fit found */
	#else
	/* $begin mmfirstfit */
		/* First fit search */
		void *bp;

		for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) {
			if (!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp)))) {
				return bp;
			}
		}
		return NULL; /* No fit */
	/* $end mmfirstfit */
	#endif
}

static void printblock(void *bp)
{
    size_t hsize, halloc, fsize, falloc;
    
    checkheap(0);
    hsize = GET_SIZE(HDRP(bp));
    halloc = GET_ALLOC(HDRP(bp));
    fsize = GET_SIZE(FTRP(bp));
    falloc = GET_ALLOC(FTRP(bp));
    
    if (hsize == 0) {
		printf("%p: EOL\n", bp);
		return;
    }
    
    /*printf("%p: header: [%p:%c] footer: [%p:%c]\n", bp,
     hsize, (halloc ? 'a' : 'f'),
     fsize, (falloc ? 'a' : 'f'));*/
}

static void checkblock(void *bp)
{
    if ((size_t)bp % 8) {
		printf("Error: %p is not doubleword aligned\n", bp);
	}
    if (GET(HDRP(bp)) != GET(FTRP(bp))) {
		printf("Error: header does not match footer\n");
	}
}

/*
 * checkheap - Minimal check of the heap for consistency
 */
void checkheap(int verbose)
{
    char *bp = heap_listp;
    
    if (verbose)
        printf("Heap (%p):\n", heap_listp);
    
    if ((GET_SIZE(HDRP(heap_listp)) != DSIZE) || !GET_ALLOC(HDRP(heap_listp)))
        printf("Bad prologue header\n");
    checkblock(heap_listp);
    
    for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) {
        if (verbose)
            printblock(bp);
        checkblock(bp);
    }
    
    if (verbose)
        printblock(bp);
    if ((GET_SIZE(HDRP(bp)) != 0) || !(GET_ALLOC(HDRP(bp))))
        printf("Bad epilogue header\n");
}