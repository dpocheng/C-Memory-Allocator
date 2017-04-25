/*
 * Ian Stephenson   - 44419093
 * Pok On Cheng     - 75147306
 * Sung Mo Koo      - 51338217
 * Cassie Liu       - 52504836
 */

/* $begin mallocinterface */
int mm_init(void); 
void *mm_malloc(size_t size); 
void mm_free(void *bp);
/* $end mallocinterface */

void mm_checkheap(int verbose);
void *mm_realloc(void *ptr, size_t size);
void mm_printblocklist(void);
unsigned long mm_getpayloadsize(int blocknumber);
void mm_writeheap(int blocknumber, char character, int numberOfRepetitions);
void mm_printheap(int blocknumber, int numberOfBytesToRead);
char* mm_blocknumbertoblock(int blocknumber);
void mm_freebufferinblock(char* bp);
void *getBlockArrayElement(int blockNumber);

/* Unused. Just to keep us compatible with the 15-213 malloc driver */
typedef struct {
    char *team;
    char *name1, *email1;
    char *name2, *email2;
} team_t;

extern team_t team;
