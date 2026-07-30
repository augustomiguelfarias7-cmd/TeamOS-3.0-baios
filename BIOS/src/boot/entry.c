/**
 * @file entry.c
 * @brief Ponto de entrada do Dual Microkernel Baios (host / stub)
 *
 * Em hardware real este arquivo seria substituído por boot.S + early init.
 * Aqui servimos como harness de inicialização e teste em userspace Linux.
 */

#include "baios.h"
#include "hw_kernel.h"
#include "sw_kernel.h"
#include "ipc.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

static baios_state_t g_state = BAIOS_STATE_UNINITIALIZED;

/* ============================================================
 * Logging simples para host
 * ============================================================ */
void baios_log(baios_log_level_t level, const char *fmt, ...) {
    static const char *levels[] = {
        "TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL"
    };
    const char *tag = (level <= BAIOS_LOG_FATAL) ? levels[level] : "?";
    fprintf(stderr, "[baios][%-5s] ", tag);
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputc('\n', stderr);
}

const char *baios_version(void) {
    return BAIOS_VERSION_STRING;
}

baios_state_t baios_get_state(void) {
    return g_state;
}

/* ============================================================
 * Init / Shutdown global
 * ============================================================ */
baios_status_t baios_init(void) {
    if (g_state != BAIOS_STATE_UNINITIALIZED)
        return BAIOS_ERR_STATE;

    BAIOS_LOG_INFO("Baios %s starting...", baios_version());

    baios_status_t st = baios_ipc_init();
    if (st != BAIOS_OK) {
        BAIOS_LOG_ERROR("ipc init failed: %s", baios_status_str(st));
        return st;
    }

    g_state = BAIOS_STATE_HW_INIT;
    st = baios_hw_init();
    if (st != BAIOS_OK) {
        BAIOS_LOG_ERROR("hw init failed: %s", baios_status_str(st));
        baios_ipc_shutdown();
        g_state = BAIOS_STATE_UNINITIALIZED;
        return st;
    }

    g_state = BAIOS_STATE_SW_INIT;
    st = baios_sw_init();
    if (st != BAIOS_OK) {
        BAIOS_LOG_ERROR("sw init failed: %s", baios_status_str(st));
        baios_hw_shutdown();
        baios_ipc_shutdown();
        g_state = BAIOS_STATE_UNINITIALIZED;
        return st;
    }

    g_state = BAIOS_STATE_RUNNING;
    BAIOS_LOG_INFO("Baios dual microkernel is RUNNING");
    return BAIOS_OK;
}

baios_status_t baios_shutdown(void) {
    if (g_state != BAIOS_STATE_RUNNING && g_state != BAIOS_STATE_PANIC)
        return BAIOS_ERR_STATE;

    BAIOS_LOG_INFO("Baios shutting down...");
    g_state = BAIOS_STATE_SHUTDOWN;

    baios_sw_shutdown();
    baios_hw_shutdown();
    baios_ipc_shutdown();

    g_state = BAIOS_STATE_UNINITIALIZED;
    BAIOS_LOG_INFO("Baios shutdown complete");
    return BAIOS_OK;
}

/* ============================================================
 * Threads de simulação (host only)
 * ============================================================ */
static void *hw_thread_fn(void *arg) {
    (void)arg;
    baios_hw_main_loop();
    return NULL;
}

static void *sw_thread_fn(void *arg) {
    (void)arg;
    baios_sw_main_loop();
    return NULL;
}

/* ============================================================
 * main — harness de demonstração
 * ============================================================ */
int main(int argc, char **argv) {
    (void)argc; (void)argv;

    printf("=== TeamOS 3.0 / Baios Dual Microkernel ===\n");
    printf("Version: %s\n\n", baios_version());

    baios_status_t st = baios_init();
    if (st != BAIOS_OK) {
        fprintf(stderr, "Failed to init Baios: %s\n", baios_status_str(st));
        return 1;
    }

    /* Demonstração rápida de syscalls */
    u64 ret = 0;
    st = baios_syscall(7 /* SYS_GETPID */, 0, 0, 0, 0, &ret);
    printf("SYS_GETPID -> status=%s pid=%llu\n",
           baios_status_str(st), (unsigned long long)ret);

    st = baios_syscall(5 /* SYS_SPAWN */, (u64)"demo-app", 0, 0, 0, &ret);
    printf("SYS_SPAWN  -> status=%s new_pid=%llu\n",
           baios_status_str(st), (unsigned long long)ret);

    /* Stats */
    baios_mem_stats_t mem;
    baios_mm_get_stats(&mem);
    printf("\nMemory: total=%llu free=%llu kernel=%llu\n",
           (unsigned long long)mem.total_pages,
           (unsigned long long)mem.free_pages,
           (unsigned long long)mem.kernel_pages);

    baios_sw_stats_t sw;
    baios_sw_get_stats(&sw);
    printf("SW: processes=%u syscalls=%llu ipc_tx=%llu\n",
           sw.process_count,
           (unsigned long long)sw.syscall_count,
           (unsigned long long)sw.ipc_tx);

    printf("\nRodando loops HW/SW por 2 segundos...\n");

    pthread_t th_hw, th_sw;
    pthread_create(&th_hw, NULL, hw_thread_fn, NULL);
    pthread_create(&th_sw, NULL, sw_thread_fn, NULL);

    sleep(2);

    /* Força saída dos loops (em produção seria sinal/flag atômica) */
    baios_shutdown();

    pthread_join(th_hw, NULL);
    pthread_join(th_sw, NULL);

    printf("\nDemo finalizada com sucesso.\n");
    return 0;
}
