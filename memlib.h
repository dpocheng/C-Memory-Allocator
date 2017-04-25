/*
 * Ian Stephenson   - 44419093
 * Pok On Cheng     - 75147306
 * Sung Mo Koo      - 51338217
 * Cassie Liu       - 52504836
 */

void mem_init(void);
void *mem_sbrk(int incr);
void mem_deinit(void);
void mem_reset_brk();
void *mem_heap_lo();
void *mem_heap_hi();
size_t mem_heapsize();
size_t mem_pagesize();