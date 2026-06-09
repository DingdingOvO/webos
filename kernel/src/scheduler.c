/**
 * WebOS Kernel - Process Scheduler
 * 
 * Round-robin scheduler with priority support.
 * In the WASM environment, scheduling is cooperative.
 */

#include "scheduler.h"
#include "process.h"
#include "host_func.h"

/* Scheduler state */
static sched_config_t sched_config = {
    .algorithm = SCHED_ROUND_ROBIN,
    .time_slice_ms = 100,
};

static int scheduler_running = 0;
static uint32_t current_index = 0;

int32_t scheduler_init(void) {
    sched_config.algorithm = SCHED_ROUND_ROBIN;
    sched_config.time_slice_ms = 100;
    scheduler_running = 0;
    current_index = 0;
    return 0;
}

void scheduler_run(void) {
    scheduler_running = 1;

    while (scheduler_running) {
        /* Find next ready process using round-robin */
        int found = 0;
        for (uint32_t i = 0; i < MAX_PROCESSES; i++) {
            uint32_t idx = (current_index + i) % MAX_PROCESSES;
            process_t* proc = &process_table[idx];

            if (proc->pid != 0 && proc->state == PROC_STATE_READY) {
                /* Run this process */
                proc->state = PROC_STATE_RUNNING;
                current_index = idx + 1;

                /* In a real WASM implementation, we would
                 * invoke the process's entry point here.
                 * For cooperative scheduling, each process
                 * runs until it yields or its time slice expires. */

                proc->state = PROC_STATE_READY;
                found = 1;
                break;
            }
        }

        if (!found) {
            /* No ready processes - idle */
            /* In WASM, we could use host sleep to avoid busy-waiting */
        }
    }

    /* Should never reach here */
    while (1) {}
}

void scheduler_yield(void) {
    process_t* proc = process_current();
    if (proc && proc->state == PROC_STATE_RUNNING) {
        proc->state = PROC_STATE_READY;
    }
}

void scheduler_block(void) {
    process_t* proc = process_current();
    if (proc && proc->state == PROC_STATE_RUNNING) {
        proc->state = PROC_STATE_BLOCKED;
    }
}

void scheduler_unblock(pid_t pid) {
    process_t* proc = process_get(pid);
    if (proc && proc->state == PROC_STATE_BLOCKED) {
        proc->state = PROC_STATE_READY;
    }
}

int32_t scheduler_set_config(const sched_config_t* config) {
    if (!config) return -1;
    sched_config = *config;
    return 0;
}

void scheduler_get_config(sched_config_t* config) {
    if (config) *config = sched_config;
}

uint32_t scheduler_ready_count(void) {
    uint32_t count = 0;
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].pid != 0 &&
            process_table[i].state == PROC_STATE_READY) {
            count++;
        }
    }
    return count;
}
