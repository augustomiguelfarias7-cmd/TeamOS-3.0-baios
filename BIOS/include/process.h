/**
 * @file process.h
 * @brief Process & Thread manager — Microkernel de Software
 */
#ifndef BAIOS_PROCESS_H
#define BAIOS_PROCESS_H

#include "types.h"
#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    BAIOS_PROC_STATE_CREATED = 0,
    BAIOS_PROC_STATE_READY,
    BAIOS_PROC_STATE_RUNNING,
    BAIOS_PROC_STATE_BLOCKED,
    BAIOS_PROC_STATE_ZOMBIE,
    BAIOS_PROC_STATE_DEAD,
} baios_proc_state_t;

typedef struct baios_process {
    baios_pid_t       pid;
    baios_pid_t       ppid;
    char              name[64];
    baios_proc_state_t state;
    baios_prio_t      priority;
    u64               create_time_ns;
    u64               cpu_time_ns;
    u32               thread_count;
    u32               handle_count;
    u64               capabilities;   /* bitmask simples de capabilities */
    struct baios_process *next;
} baios_process_t;

baios_status_t baios_proc_init(void);
void           baios_proc_shutdown(void);

baios_status_t baios_proc_create(const char *name, baios_prio_t prio, baios_pid_t *out_pid);
baios_status_t baios_proc_exit(baios_pid_t pid, i32 exit_code);
baios_status_t baios_proc_kill(baios_pid_t pid);

baios_process_t *baios_proc_get(baios_pid_t pid);
baios_process_t *baios_proc_current(void);

u32 baios_proc_count(void);

#ifdef __cplusplus
}
#endif

#endif /* BAIOS_PROCESS_H */
