#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>

/* Process states */
#define PROC_STATE_READY    0
#define PROC_STATE_RUNNING  1
#define PROC_STATE_BLOCKED  2
#define PROC_STATE_ZOMBIE   3

/* Maximum number of processes */
#define MAX_PROCESSES 64

/* Process priority levels */
#define PROC_PRIORITY_LOW    0
#define PROC_PRIORITY_NORMAL 1
#define PROC_PRIORITY_HIGH   2
#define PROC_PRIORITY_REALTIME 3

/* Process ID type */
typedef uint32_t pid_t;

/* Process control block */
typedef struct {
    pid_t pid;
    uint32_t state;
    uint32_t priority;
    void* module_handle;   /* WASM module handle */
    void* stack_ptr;       /* Current stack pointer */
    void* entry_point;     /* Program entry point */
    uint32_t flags;
    int32_t exit_code;
} process_t;

/**
 * Initialize process management subsystem.
 */
int32_t process_init(void);

/**
 * Create a new process from a WASM module.
 * 
 * @param module_path  Path to the WASM module
 * @param priority     Process priority
 * @return             Process ID, or 0 on failure
 */
pid_t process_spawn(const char* module_path, uint32_t priority);

/**
 * Kill a process.
 * 
 * @param pid  Process ID to kill
 * @return     0 on success, negative on error
 */
int32_t process_kill(pid_t pid);

/**
 * Get the current process ID.
 */
pid_t process_getpid(void);

/**
 * Exit the current process.
 */
void process_exit(int32_t code) __attribute__((noreturn));

/**
 * Wait for a process to exit.
 * 
 * @param pid       Process to wait for
 * @param exit_code Output: exit code
 * @return          0 on success, negative on error
 */
int32_t process_wait(pid_t pid, int32_t* exit_code);

/**
 * Get process control block by PID.
 */
process_t* process_get(pid_t pid);

/**
 * Get the currently running process.
 */
process_t* process_current(void);

#endif /* PROCESS_H */
