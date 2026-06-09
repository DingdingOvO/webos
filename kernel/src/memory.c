/**
 * WebOS Kernel - Memory Management
 * 
 * Simple memory management for the WASM environment.
 * Uses a free-list allocator on top of the linear memory.
 */

#include "memory.h"
#include "host_func.h"

/* Maximum tracked memory regions */
#define MAX_REGIONS 256

/* Memory state */
static mem_region_t regions[MAX_REGIONS];
static uint32_t region_count = 0;
static mem_stats_t stats = {0};

int32_t memory_init(void) {
    /* Initialize region list */
    for (int i = 0; i < MAX_REGIONS; i++) {
        regions[i].type = MEM_REGION_FREE;
        regions[i].start = 0;
        regions[i].size = 0;
        regions[i].owner_pid = 0;
    }
    region_count = 0;

    /* Reserve first region for kernel */
    regions[0].start = 0;
    regions[0].size = PAGE_SIZE * 4;  /* 256KB for kernel */
    regions[0].type = MEM_REGION_KERNEL;
    regions[0].owner_pid = 0;
    region_count = 1;

    /* Initialize stats */
    stats.total_pages = 256;  /* Will be updated based on actual memory */
    stats.used_pages = 4;
    stats.free_pages = stats.total_pages - stats.used_pages;
    stats.kernel_pages = 4;
    stats.shared_pages = 0;

    return 0;
}

void* memory_alloc(uint32_t pages) {
    if (pages == 0) return (void*)0;

    /* Find a free region slot */
    int slot = -1;
    for (int i = 0; i < MAX_REGIONS; i++) {
        if (regions[i].type == MEM_REGION_FREE) {
            slot = i;
            break;
        }
    }

    if (slot < 0) return (void*)0;  /* No free slots */

    /* Simple bump allocator - find end of used memory */
    uint32_t start = 0;
    for (int i = 0; i < MAX_REGIONS; i++) {
        if (regions[i].type != MEM_REGION_FREE) {
            uint32_t end = regions[i].start + regions[i].size;
            if (end > start) start = end;
        }
    }

    /* Align to page boundary */
    start = (start + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

    regions[slot].start = start;
    regions[slot].size = pages * PAGE_SIZE;
    regions[slot].type = MEM_REGION_USED;
    regions[slot].owner_pid = 0;  /* Current process */

    stats.used_pages += pages;
    stats.free_pages -= pages;

    return (void*)(uintptr_t)start;
}

int32_t memory_free(void* ptr) {
    if (!ptr) return -1;  /* ERR_INVAL */

    uint32_t addr = (uint32_t)(uintptr_t)ptr;

    /* Find the region */
    for (int i = 0; i < MAX_REGIONS; i++) {
        if (regions[i].start == addr && regions[i].type == MEM_REGION_USED) {
            uint32_t pages = regions[i].size / PAGE_SIZE;
            regions[i].type = MEM_REGION_FREE;
            stats.used_pages -= pages;
            stats.free_pages += pages;
            return 0;
        }
    }

    return -1;  /* ERR_INVAL */
}

void memory_stats(mem_stats_t* out) {
    if (out) *out = stats;
}

void* memory_map_shared(uint32_t size, uint32_t owner_pid) {
    /* Align to page boundary */
    uint32_t pages = (size + PAGE_SIZE - 1) / PAGE_SIZE;

    void* ptr = memory_alloc(pages);
    if (ptr) {
        /* Find and update the region */
        uint32_t addr = (uint32_t)(uintptr_t)ptr;
        for (int i = 0; i < MAX_REGIONS; i++) {
            if (regions[i].start == addr) {
                regions[i].type = MEM_REGION_SHARED;
                regions[i].owner_pid = owner_pid;
                break;
            }
        }
        stats.shared_pages += pages;
    }
    return ptr;
}
