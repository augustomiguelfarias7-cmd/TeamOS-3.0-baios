#ifndef BAIOS_SW_KERNEL_H
#define BAIOS_SW_KERNEL_H

#include "types.h"
#include "permissions.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Software Microkernel
 *
 * Responsável por:
 * - Ciclo de vida de processos e threads
 * - Sistema de arquivos lógico
 * - Controle de permissões e sandbox
 * - Serviços de alto nível (áudio, rede, renderização)
 * - Recepção e validação de syscalls das aplicações
 */

#define BAIOS_SW_KERNEL_ID  1

#define BAIOS_MAX_PROCESSES   256
#define BAIOS_MAX_THREADS     1024
#define BAIOS_MAX_FDS         64

typedef struct baios_process {
    baios_pid_t       pid;
    baios_uid_t       uid;
    baios_uid_t       gid;
    char              name[64];
    baios_proc_state_t state;
    baios_priority_t  priority;
    u64               start_time_ns;
    u64               cpu_time_ns;
    baios_addr_t      entry_point;
    baios_addr_t      stack_base;
    baios_size_t      stack_size;
    u32               fd_table[BAIOS_MAX_FDS];
    baios_perm_set_t  permissions;
    struct baios_process *parent;
    struct baios_process *next;
} baios_process_t;

typedef struct {
    bool initialized;
    u64  heartbeat;
    u32  process_count;
    u32  next_pid;
    baios_process_t *process_list;
    baios_process_t *current;
} baios_sw_state_t;

/* Lifecycle */
baios_error_t sw_kernel_init(void);
void          sw_kernel_shutdown(void);
void          sw_kernel_main_loop(void);

/* Process management */
baios_error_t sw_process_create(const char *name, baios_addr_t entry,
                                baios_priority_t prio, baios_pid_t *out_pid);
baios_error_t sw_process_kill(baios_pid_t pid, i32 signal);
baios_error_t sw_process_wait(baios_pid_t pid, i32 *out_status);
baios_process_t *sw_process_get(baios_pid_t pid);
baios_process_t *sw_process_current(void);

/* Scheduler cooperativo simples */
void sw_scheduler_tick(void);
baios_error_t sw_scheduler_yield(void);

/* Syscall dispatcher (entrada das aplicações) */
typedef enum {
    BAIOS_SYS_EXIT          = 1,
    BAIOS_SYS_READ          = 2,
    BAIOS_SYS_WRITE         = 3,
    BAIOS_SYS_OPEN          = 4,
    BAIOS_SYS_CLOSE         = 5,
    BAIOS_SYS_MMAP          = 6,
    BAIOS_SYS_MUNMAP        = 7,
    BAIOS_SYS_IOCTL         = 8,
    BAIOS_SYS_GETPID        = 9,
    BAIOS_SYS_SLEEP         = 10,
    BAIOS_SYS_YIELD         = 11,
    BAIOS_SYS_CREATE_PROC   = 12,
    BAIOS_SYS_KILL          = 13,
    BAIOS_SYS_IPC_SEND      = 20,
    BAIOS_SYS_IPC_RECV      = 21,
    BAIOS_SYS_GET_PERM      = 30,
    BAIOS_SYS_REQUEST_PERM  = 31
} baios_syscall_t;

baios_error_t sw_syscall(u32 number, u64 arg0, u64 arg1, u64 arg2,
                         u64 arg3, u64 arg4, u64 *out_result);

#ifdef __cplusplus
}
#endif

#endif /* BAIOS_SW_KERNEL_H */
