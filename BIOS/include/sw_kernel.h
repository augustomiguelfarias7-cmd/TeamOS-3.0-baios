/**
 * @file sw_kernel.h
 * @brief Microkernel de Software (C++) — interface C-compatible
 */
#ifndef BAIOS_SW_KERNEL_H
#define BAIOS_SW_KERNEL_H

#include "types.h"
#include "config.h"
#include "ipc.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    bool initialized;
    u32  process_count;
    u32  thread_count;
    u64  syscall_count;
    u64  ipc_rx;
    u64  ipc_tx;
} baios_sw_stats_t;

baios_status_t baios_sw_init(void);
void           baios_sw_shutdown(void);

/** Loop principal do microkernel de software. */
void baios_sw_main_loop(void);

/** Processa mensagem vinda do Hardware kernel. */
baios_status_t baios_sw_handle_ipc(const baios_ipc_msg_t *msg);

/** Syscall entry (chamado pelas aplicações). */
baios_status_t baios_syscall(u32 nr, u64 arg0, u64 arg1, u64 arg2, u64 arg3, u64 *ret);

void baios_sw_get_stats(baios_sw_stats_t *out);

#ifdef __cplusplus
}
#endif

#endif /* BAIOS_SW_KERNEL_H */
