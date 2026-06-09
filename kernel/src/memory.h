#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>

/* Memory page size (WASM page = 64KB) */
#define PAGE_SIZE 65536

/* Maximum number of memory pages */
#define MAX_PAGES 1024

/* Memory region types */
#define MEM_REGION_FREE    0
#define MEM_REGION_USED    1
#define MEM_REGION_KERNEL  2
#define MEM_REGION_SHARED  3

/* Memory region descriptor */
typedef struct {
    uint32_t start;      /* Start address (byte offset) */
    uint32_t size;       /* Size in bytes */
    uint32_t type;       /* Region type */
    uint32_t owner_pid;  /* Owning process ID (0 = kernel) */
} mem_region_t;

/* Memory statistics */
typedef struct {
    uint32_t total_pages;
    uint32_t used_pages;
    uint32_t free_pages;
    uint32_t kernel_pages;
    uint32_t shared_pages;
} mem_stats_t;

/**
 * Initialize memory management subsystem.
 */
int32_t memory_init(void);

/**
 * Allocate memory pages.
 * 
 * @param pages  Number of 64KB pages to allocate
 * @return       Pointer to allocated memory, or NULL on failure
 */
void* memory_alloc(uint32_t pages);

/**
 * Free previously allocated memory.
 * 
 * @param ptr  Pointer to memory to free
 * @return     0 on success, negative on error
 */
int32_t memory_free(void* ptr);

/**
 * Get memory statistics.
 */
void memory_stats(mem_stats_t* stats);

/**
 * Map shared memory between processes.
 */
void* memory_map_shared(uint32_t size, uint32_t owner_pid);

#endif /* MEMORY_H */
