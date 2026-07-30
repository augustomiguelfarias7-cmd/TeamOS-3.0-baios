/**
 * demo_main.c — Demonstração completa do Dual Microkernel Baios
 *
 * Compila e roda em userspace para validar a estrutura.
 */

#include "include/baios.h"
#include "include/hw_kernel.h"
#include "include/sw_kernel.h"
#include "include/ipc.h"

#include <stdio.h>

/* Declarados no lado SW */
extern baios_error_t sw_full_init(void);
extern void          sw_full_shutdown(void);
extern void          sw_kernel_main_loop(void);

int main(void) {
    printf("\n*** TeamOS 3.0 — Baios Dual Microkernel Demo ***\n\n");

    if (baios_init() != BAIOS_OK) {
        fprintf(stderr, "Falha em baios_init\n");
        return 1;
    }

    /* 1. Inicializa Hardware Microkernel */
    if (hw_kernel_init() != BAIOS_OK) {
        fprintf(stderr, "Falha em hw_kernel_init\n");
        return 1;
    }

    /* 2. Inicializa Software Microkernel + serviços */
    if (sw_full_init() != BAIOS_OK) {
        fprintf(stderr, "Falha em sw_full_init\n");
        hw_kernel_shutdown();
        return 1;
    }

    /* 3. Roda loops de demonstração */
    hw_kernel_main_loop();
    sw_kernel_main_loop();

    /* 4. Estatísticas finais */
    baios_stats_t stats;
    if (baios_get_stats(&stats) == BAIOS_OK) {
        printf("\n--- Estatísticas ---\n");
        printf("Uptime:        %llu ns\n", (unsigned long long)stats.uptime_ns);
        printf("Memória usada: %llu / %llu bytes\n",
               (unsigned long long)stats.memory_used_bytes,
               (unsigned long long)stats.memory_total_bytes);
    }

    const baios_version_info_t *v = baios_get_version();
    printf("Versão: %s (dual=%d, zero-copy=%d, linux-compat=%d)\n",
           v->version_string, v->dual_microkernel, v->zero_copy_ipc, v->linux_compat);

    /* 5. Shutdown ordenado */
    sw_full_shutdown();
    hw_kernel_shutdown();
    baios_shutdown();

    printf("\n*** Demo finalizada com sucesso ***\n");
    return 0;
}
