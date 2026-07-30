/**
 * @file sw_kernel.cpp
 * @brief Microkernel de Software (C++) — núcleo principal
 */

#include "sw_kernel.h"
#include "process.h"
#include "baios.h"
#include <cstring>
#include <cstdio>

static baios_sw_stats_t g_sw_stats;
static bool             g_sw_running = false;

/* Números de syscall (exemplo) */
enum {
    SYS_NOP       = 0,
    SYS_WRITE     = 1,
    SYS_READ      = 2,
    SYS_OPEN      = 3,
    SYS_CLOSE     = 4,
    SYS_SPAWN     = 5,
    SYS_EXIT      = 6,
    SYS_GETPID    = 7,
    SYS_IPC_SEND  = 8,
    SYS_IPC_RECV  = 9,
    SYS_MMAP      = 10,
    SYS_MAX
};

extern "C" baios_status_t baios_sw_init(void) {
    if (g_sw_stats.initialized)
        return BAIOS_ERR_STATE;

    std::memset(&g_sw_stats, 0, sizeof(g_sw_stats));

    baios_status_t st = baios_proc_init();
    if (st != BAIOS_OK)
        return st;

    /* Processo idle / init */
    baios_pid_t init_pid;
    st = baios_proc_create("init", BAIOS_PRIO_KERNEL, &init_pid);
    if (st != BAIOS_OK) {
        baios_proc_shutdown();
        return st;
    }

    g_sw_stats.initialized = true;
    g_sw_stats.process_count = baios_proc_count();
    BAIOS_LOG_INFO("sw_kernel: Microkernel de Software online (init pid=%u)", init_pid);
    return BAIOS_OK;
}

extern "C" void baios_sw_shutdown(void) {
    if (!g_sw_stats.initialized)
        return;
    g_sw_running = false;
    baios_proc_shutdown();
    g_sw_stats.initialized = false;
    BAIOS_LOG_INFO("sw_kernel: shutdown");
}

extern "C" baios_status_t baios_sw_handle_ipc(const baios_ipc_msg_t *msg) {
    if (!g_sw_stats.initialized || !msg)
        return BAIOS_ERR_INVAL;

    g_sw_stats.ipc_rx++;

    switch (msg->type) {
    case BAIOS_IPC_MSG_IRQ:
        BAIOS_LOG_DEBUG("sw_kernel: IRQ notification from HW");
        break;
    case BAIOS_IPC_MSG_DMA_DONE:
        BAIOS_LOG_DEBUG("sw_kernel: DMA done");
        break;
    case BAIOS_IPC_MSG_REPLY:
        BAIOS_LOG_DEBUG("sw_kernel: received reply from HW");
        break;
    default:
        BAIOS_LOG_WARN("sw_kernel: unhandled IPC type %u", msg->type);
        break;
    }
    return BAIOS_OK;
}

extern "C" baios_status_t baios_syscall(u32 nr, u64 arg0, u64 arg1, u64 arg2, u64 arg3, u64 *ret) {
    if (!g_sw_stats.initialized)
        return BAIOS_ERR_STATE;

    g_sw_stats.syscall_count++;
    if (ret) *ret = 0;

    switch (nr) {
    case SYS_NOP:
        return BAIOS_OK;

    case SYS_GETPID: {
        baios_process_t *cur = baios_proc_current();
        if (ret && cur)
            *ret = cur->pid;
        return BAIOS_OK;
    }

    case SYS_SPAWN: {
        /* arg0 = ponteiro para nome (em host é válido) */
        const char *name = reinterpret_cast<const char *>(arg0);
        if (!name)
            return BAIOS_ERR_INVAL;
        baios_pid_t pid;
        baios_status_t st = baios_proc_create(name, BAIOS_PRIO_NORMAL, &pid);
        if (st == BAIOS_OK && ret)
            *ret = pid;
        g_sw_stats.process_count = baios_proc_count();
        return st;
    }

    case SYS_EXIT: {
        baios_process_t *cur = baios_proc_current();
        if (!cur)
            return BAIOS_ERR_STATE;
        return baios_proc_exit(cur->pid, static_cast<i32>(arg0));
    }

    case SYS_IPC_SEND:
    case SYS_WRITE:
    case SYS_READ:
    case SYS_OPEN:
    case SYS_CLOSE:
    case SYS_MMAP: {
        /* Encaminha para o HW via Zero-Copy IPC */
        baios_ipc_msg_t msg;
        std::memset(&msg, 0, sizeof(msg));
        msg.type   = BAIOS_IPC_MSG_SYSCALL;
        msg.src_id = baios_proc_current() ? baios_proc_current()->pid : 0;
        msg.data_len = sizeof(u32) * 2 + sizeof(u64) * 4;
        /* Pack simples: nr, arg0..arg3 */
        u8 *p = msg.data;
        *reinterpret_cast<u32 *>(p) = nr; p += 4;
        *reinterpret_cast<u64 *>(p) = arg0; p += 8;
        *reinterpret_cast<u64 *>(p) = arg1; p += 8;
        *reinterpret_cast<u64 *>(p) = arg2; p += 8;
        *reinterpret_cast<u64 *>(p) = arg3;

        baios_status_t st = baios_ipc_send_to_hw(&msg);
        if (st == BAIOS_OK)
            g_sw_stats.ipc_tx++;
        return st;
    }

    default:
        return BAIOS_ERR_NOSYS;
    }
}

extern "C" void baios_sw_main_loop(void) {
    if (!g_sw_stats.initialized)
        return;

    g_sw_running = true;
    BAIOS_LOG_INFO("sw_kernel: entering main loop");

    while (g_sw_running) {
        baios_ipc_msg_t msg;
        baios_status_t st = baios_ipc_recv_from_hw(&msg, 50);
        if (st == BAIOS_OK) {
            baios_sw_handle_ipc(&msg);
        }
    }
}

extern "C" void baios_sw_get_stats(baios_sw_stats_t *out) {
    if (out) {
        g_sw_stats.process_count = baios_proc_count();
        *out = g_sw_stats;
    }
}
