/**
 * @file hw_kernel.c
 * @brief Microkernel de Hardware — núcleo principal
 */

#include "hw_kernel.h"
#include "baios.h"
#include <string.h>

static baios_hw_stats_t g_hw_stats;
static bool             g_hw_running = false;

baios_status_t baios_hw_init(void) {
    if (g_hw_stats.initialized)
        return BAIOS_ERR_STATE;

    memset(&g_hw_stats, 0, sizeof(g_hw_stats));

    /* Memória física simulada: 64 MiB */
    baios_status_t st = baios_mm_init(0x100000ULL, 64ULL * 1024 * 1024);
    if (st != BAIOS_OK)
        return st;

    st = baios_irq_init();
    if (st != BAIOS_OK) {
        baios_mm_shutdown();
        return st;
    }

    st = baios_driver_init();
    if (st != BAIOS_OK) {
        baios_irq_shutdown();
        baios_mm_shutdown();
        return st;
    }

    g_hw_stats.initialized = true;
    BAIOS_LOG_INFO("hw_kernel: Microkernel de Hardware online");
    return BAIOS_OK;
}

void baios_hw_shutdown(void) {
    if (!g_hw_stats.initialized)
        return;

    g_hw_running = false;
    baios_driver_shutdown();
    baios_irq_shutdown();
    baios_mm_shutdown();
    g_hw_stats.initialized = false;
    BAIOS_LOG_INFO("hw_kernel: shutdown");
}

baios_status_t baios_hw_handle_ipc(const baios_ipc_msg_t *msg) {
    if (!g_hw_stats.initialized || !msg)
        return BAIOS_ERR_INVAL;

    switch (msg->type) {
    case BAIOS_IPC_MSG_SYSCALL:
        g_hw_stats.syscall_fwd++;
        /* Aqui o HW executaria a parte privilegiada da syscall
           (acesso a device, map de páginas físicas, etc.) e
           responderia via IPC_MSG_REPLY. */
        {
            baios_ipc_msg_t reply;
            memset(&reply, 0, sizeof(reply));
            reply.type    = BAIOS_IPC_MSG_REPLY;
            reply.flags   = BAIOS_IPC_FLAG_REPLY;
            reply.src_id  = 0; /* HW */
            reply.dst_id  = msg->src_id;
            reply.data_len = sizeof(baios_status_t);
            *(baios_status_t *)reply.data = BAIOS_OK;
            baios_ipc_send_to_sw(&reply);
        }
        break;

    case BAIOS_IPC_MSG_DRIVER:
        /* Controle de driver (load/unload/suspend...) */
        BAIOS_LOG_DEBUG("hw_kernel: driver control msg len=%u", msg->data_len);
        break;

    case BAIOS_IPC_MSG_POWER:
        BAIOS_LOG_INFO("hw_kernel: power management request");
        break;

    default:
        BAIOS_LOG_WARN("hw_kernel: unknown IPC type %u", msg->type);
        return BAIOS_ERR_INVAL;
    }
    return BAIOS_OK;
}

void baios_hw_main_loop(void) {
    if (!g_hw_stats.initialized)
        return;

    g_hw_running = true;
    BAIOS_LOG_INFO("hw_kernel: entering main loop");

    while (g_hw_running) {
        baios_ipc_msg_t msg;
        baios_status_t st = baios_ipc_recv_from_sw(&msg, 50); /* 50ms poll */
        if (st == BAIOS_OK) {
            baios_hw_handle_ipc(&msg);
        }
        /* Em kernel real: wfi / halt até próxima IRQ */
    }
}

void baios_hw_get_stats(baios_hw_stats_t *out) {
    if (out)
        *out = g_hw_stats;
}
