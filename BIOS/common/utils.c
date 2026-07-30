/**
 * common/utils.c — Utilitários compartilhados
 */

#include "../include/baios.h"
#include "../include/types.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

static const baios_version_info_t g_version = {
    .major           = BAIOS_VERSION_MAJOR,
    .minor           = BAIOS_VERSION_MINOR,
    .patch           = BAIOS_VERSION_PATCH,
    .version_string  = BAIOS_VERSION_STRING,
    .build_date      = __DATE__,
    .build_time      = __TIME__,
    .dual_microkernel = true,
    .zero_copy_ipc    = true,
    .linux_compat     = true
};

const baios_version_info_t *baios_get_version(void) {
    return &g_version;
}

static u64 g_start_ns = 0;
static u64 g_syscall_count = 0;
static u64 g_ipc_count = 0;

static u64 mono_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (u64)ts.tv_sec * 1000000000ULL + (u64)ts.tv_nsec;
}

baios_error_t baios_init(void) {
    g_start_ns = mono_ns();
    printf("========================================\n");
    printf("  Baios Dual Microkernel %s\n", BAIOS_VERSION_STRING);
    printf("  Build: %s %s\n", __DATE__, __TIME__);
    printf("========================================\n");
    return BAIOS_OK;
}

void baios_shutdown(void) {
    printf("[Baios] Shutdown completo.\n");
}

baios_error_t baios_health_check(baios_health_t *out) {
    if (!out) return BAIOS_ERR_INVALID_ARG;
    memset(out, 0, sizeof(*out));
    out->hw_heartbeat = 0; /* preenchido pelo hw */
    out->sw_heartbeat = 0;
    out->last_sync_ns = mono_ns();
    out->hw_alive = true;
    out->sw_alive = true;
    return BAIOS_OK;
}

baios_error_t baios_get_stats(baios_stats_t *out) {
    if (!out) return BAIOS_ERR_INVALID_ARG;
    memset(out, 0, sizeof(*out));
    out->uptime_ns = mono_ns() - g_start_ns;
    out->total_syscalls = g_syscall_count;
    out->total_ipc_messages = g_ipc_count;

    baios_mem_stats_t ms;
    if (baios_mem_stats(&ms) == BAIOS_OK) {
        out->memory_used_bytes  = ms.used_bytes;
        out->memory_total_bytes = ms.total_bytes;
    }
    return BAIOS_OK;
}

void baios_stats_inc_syscall(void) { g_syscall_count++; }
void baios_stats_inc_ipc(void)     { g_ipc_count++; }
