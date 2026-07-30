#ifndef BAIOS_H
#define BAIOS_H

/**
 * Baios — Dual Microkernel do TeamOS 3.0
 *
 * Header principal. Inclui todos os módulos públicos.
 */

#include "types.h"
#include "memory.h"
#include "ipc.h"
#include "hw_kernel.h"
#include "sw_kernel.h"
#include "interrupts.h"
#include "drivers.h"
#include "permissions.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Versão do kernel */
#define BAIOS_VERSION_MAJOR   3
#define BAIOS_VERSION_MINOR   0
#define BAIOS_VERSION_PATCH   0
#define BAIOS_VERSION_STRING  "3.0.0-baios"

/* Informações de build */
typedef struct {
    u32 major;
    u32 minor;
    u32 patch;
    const char *version_string;
    const char *build_date;
    const char *build_time;
    bool dual_microkernel;
    bool zero_copy_ipc;
    bool linux_compat;
} baios_version_info_t;

const baios_version_info_t *baios_get_version(void);

/* Inicialização global do Baios */
baios_error_t baios_init(void);
void          baios_shutdown(void);

/* Heartbeat / watchdog entre os dois microkernels */
typedef struct {
    u64 hw_heartbeat;
    u64 sw_heartbeat;
    u64 last_sync_ns;
    bool hw_alive;
    bool sw_alive;
} baios_health_t;

baios_error_t baios_health_check(baios_health_t *out);

/* Estatísticas globais */
typedef struct {
    u64 uptime_ns;
    u64 total_syscalls;
    u64 total_ipc_messages;
    u64 total_page_faults;
    u64 total_interrupts;
    u64 memory_used_bytes;
    u64 memory_total_bytes;
    u32 process_count;
    u32 thread_count;
} baios_stats_t;

baios_error_t baios_get_stats(baios_stats_t *out);

#ifdef __cplusplus
}
#endif

#endif /* BAIOS_H */
