/**
 * @file hw_kernel.h
 * @brief Microkernel de Hardware (C) — interface
 */
#ifndef BAIOS_HW_KERNEL_H
#define BAIOS_HW_KERNEL_H

#include "types.h"
#include "config.h"
#include "memory.h"
#include "interrupt.h"
#include "driver_compat.h"
#include "ipc.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    bool initialized;
    u64  uptime_ns;
    u64  irq_total;
    u64  syscall_fwd;   /* syscalls encaminhadas via IPC */
} baios_hw_stats_t;

baios_status_t baios_hw_init(void);
void           baios_hw_shutdown(void);

/** Loop principal do microkernel de hardware (pode rodar em thread própria). */
void baios_hw_main_loop(void);

/** Processa uma mensagem IPC vinda do Software kernel. */
baios_status_t baios_hw_handle_ipc(const baios_ipc_msg_t *msg);

void baios_hw_get_stats(baios_hw_stats_t *out);

#ifdef __cplusplus
}
#endif

#endif /* BAIOS_HW_KERNEL_H */
