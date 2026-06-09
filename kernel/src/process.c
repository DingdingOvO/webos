/**
 * WebOS Kernel - Process Management
 * 
 * Manages process creation, destruction, and state transitions.
 */

#include "process.h"
#include "host_func.h"

/* Process table */
static process_t process_table[MAX_PROCESSES];
static pid_t current_pid = 0;
static pid_t next_pid = 1;

int32_t process_init(void) {
    /* Clear process table */
    for (int i = 0; i < MAX_PROCESSES; i++) {
        process_table[i].pid = 0;
        process_table[i].state = PROC_STATE_READY;
        process_table[i].priority = PROC_PRIORITY_NORMAL;
        process_table[i].module_handle = (void*)0;
        process_table[i].stack_ptr = (void*)0;
        process_table[i].entry_point = (void*)0;
        process_table[i].flags = 0;
        process_table[i].exit_code = 0;
    }
    current_pid = 0;
    next_pid = 1;
    return 0;
}

pid_t process_spawn(const char* module_path, uint32_t priority) {
    /* Find a free slot */
    int slot = -1;
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].pid == 0) {
            slot = i;
            break;
        }
    }

    if (slot < 0) return 0;  /* No free slots */

    pid_t pid = next_pid++;
    process_table[slot].pid = pid;
    process_table[slot].state = PROC_STATE_READY;
    process_table[slot].priority = priority;
    process_table[slot].module_handle = (void*)0;  /* Will be set by dynlink */
    process_table[slot].stack_ptr = (void*)0;
    process_table[slot].entry_point = (void*)0;
    process_table[slot].flags = 0;
    process_table[slot].exit_code = 0;

    return pid;
}

int32_t process_kill(pid_t pid) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].pid == pid) {
            process_table[i].state = PROC_STATE_ZOMBIE;
            process_table[i].exit_code = -1;
            return 0;
        }
    }
    return -2;  /* ERR_NOENT */
}

pid_t process_getpid(void) {
    return current_pid;
}

void process_exit(int32_t code) {
    process_t* proc = process_current();
    if (proc) {
        proc->state = PROC_STATE_ZOMBIE;
        proc->exit_code = code;
    }
    /* In a real implementation, this would yield to the scheduler */
    while (1) {}  /* Never returns */
}

int32_t process_wait(pid_t pid, int32_t* exit_code) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].pid == pid) {
            if (process_table[i].state == PROC_STATE_ZOMBIE) {
                if (exit_code) *exit_code = process_table[i].exit_code;
                process_table[i].pid = 0;  /* Free slot */
                return 0;
            }
            return -4;  /* ERR_BUSY - process still running */
        }
    }
    return -2;  /* ERR_NOENT */
}

process_t* process_get(pid_t pid) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].pid == pid) {
            return &process_table[i];
        }
    }
    return (process_t*)0;
}

process_t* process_current(void) {
    return process_get(current_pid);
}
