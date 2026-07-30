/**
 * hw/main.c — Ponto de entrada e loop principal do Hardware Microkernel
 */

#include "../include/baios.h"
#include "../include/hw_kernel.h"
#include "../include/ipc.h"
#include "../include/interrupts.h"
#include "../include/drivers.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

static baios_hw_state_t g_hw;

static u64 monotonic_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (u64)ts.tv_sec * 1000000000ULL + (u64)ts.tv_nsec;
}

static void timer_irq_handler(u32 irq, void *ctx) {
    (void)irq; (void)ctx;
    g_hw.heartbeat++;
    softirq_process();
}

baios_error_t hw_kernel_init(void) {
    memset(&g_hw, 0, sizeof(g_hw));

    baios_error_t err;

    err = baios_mem_init(0, 16 * 1024 * 1024);
    if (err != BAIOS_OK) return err;

    err = hw_mem_init_buddy(0x100000, 8 * 1024 * 1024); /* 8MB a partir de 1MB */
    if (err != BAIOS_OK) return err;

    err = irq_init();
    if (err != BAIOS_OK) return err;

    err = irq_register(BAIOS_IRQ_TIMER, timer_irq_handler, NULL, "timer");
    if (err != BAIOS_OK) return err;

    err = linux_compat_init();
    if (err != BAIOS_OK) return err;

    err = baios_ipc_init();
    if (err != BAIOS_OK) return err;

    g_hw.boot_time_ns = monotonic_ns();
    g_hw.initialized  = true;
    g_hw.heartbeat    = 0;

    printf("[HW] Hardware Microkernel inicializado (%s)\n", BAIOS_VERSION_STRING);
    return BAIOS_OK;
}

void hw_kernel_shutdown(void) {
    if (!g_hw.initialized) return;
    printf("[HW] Desligando Hardware Microkernel...\n");
    g_hw.initialized = false;
}

void hw_kernel_tick(void) {
    if (!g_hw.initialized) return;
    g_hw.heartbeat++;
    /* Simula tick de timer */
    irq_dispatch(BAIOS_IRQ_TIMER);
}

u64 hw_kernel_get_heartbeat(void) {
    return g_hw.heartbeat;
}

void hw_kernel_main_loop(void) {
    printf("[HW] Entrando no loop principal...\n");
    for (int i = 0; i < 10; i++) { /* limitado para demo */
        hw_kernel_tick();
        softirq_process();

        /* Processa mensagens vindas do Software Microkernel */
        baios_ipc_header_t hdr;
        u8 payload[256];
        baios_size_t plen = sizeof(payload);
        if (baios_ipc_try_recv(0 /* canal principal usa handle interno */, &hdr, payload, &plen) == BAIOS_OK) {
            printf("[HW] Recebeu opcode=%u size=%llu\n", hdr.opcode, (unsigned long long)hdr.payload_size);
        }
    }
    printf("[HW] Loop de demonstração finalizado (heartbeat=%llu)\n",
           (unsigned long long)g_hw.heartbeat);
}
