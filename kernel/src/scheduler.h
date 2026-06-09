#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdint.h>
#include "process.h"

/* Scheduler algorithms */
#define SCHED_ROUND_ROBIN  0
#define SCHED_PRIORITY     1

/* Scheduler configuration */
typedef struct {
    uint32_t algorithm;
    uint32_t time_slice_ms;   /* Time slice in milliseconds */
} sched_config_t;

/**
 * Initialize the scheduler.
 */
int32_t scheduler_init(void);

/**
 * Start the scheduler (main loop).
 * This function does not return.
 */
void scheduler_run(void) __attribute__((noreturn));

/**
 * Yield the current time slice.
 */
void scheduler_yield(void);

/**
 * Block the current process (wait for event).
 */
void scheduler_block(void);

/**
 * Unblock a process.
 * 
 * @param pid  Process to unblock
 */
void scheduler_unblock(pid_t pid);

/**
 * Set scheduler configuration.
 */
int32_t scheduler_set_config(const sched_config_t* config);

/**
 * Get scheduler configuration.
 */
void scheduler_get_config(sched_config_t* config);

/**
 * Get the number of ready processes.
 */
uint32_t scheduler_ready_count(void);

#endif /* SCHEDULER_H */
