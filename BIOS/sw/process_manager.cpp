/**
 * process_manager.cpp — Gerenciador de processos do Software Microkernel
 */

#include "../include/sw_kernel.h"
#include "../include/permissions.h"
#include "../include/memory.h"
#include "../include/types.h"

#include <cstring>
#include <cstdio>
#include <new>

static baios_sw_state_t g_sw;
static baios_process_t  g_proc_pool[BAIOS_MAX_PROCESSES];
static bool             g_proc_used[BAIOS_MAX_PROCESSES];

extern "C" {

baios_error_t sw_process_create(const char *name, baios_addr_t entry,
                                baios_priority_t prio, baios_pid_t *out_pid) {
    if (!name || !out_pid) return BAIOS_ERR_INVALID_ARG;

    int slot = -1;
    for (int i = 0; i < BAIOS_MAX_PROCESSES; i++) {
        if (!g_proc_used[i]) {
            slot = i;
            break;
        }
    }
    if (slot < 0) return BAIOS_ERR_NO_MEMORY;

    baios_process_t *p = &g_proc_pool[slot];
    std::memset(p, 0, sizeof(*p));

    p->pid          = g_sw.next_pid++;
    p->uid          = 1000; /* usuário padrão */
    p->gid          = 1000;
    std::strncpy(p->name, name, sizeof(p->name) - 1);
    p->state        = BAIOS_PROC_READY;
    p->priority     = prio;
    p->entry_point  = entry;
    p->stack_size   = 64 * 1024;
    p->stack_base   = (baios_addr_t)baios_kmalloc(p->stack_size);
    p->permissions  = BAIOS_PERM_DEFAULT_USER;
    p->parent       = g_sw.current;

    if (!p->stack_base && p->stack_size > 0) {
        return BAIOS_ERR_NO_MEMORY;
    }

    g_proc_used[slot] = true;
    p->next = g_sw.process_list;
    g_sw.process_list = p;
    g_sw.process_count++;

    *out_pid = p->pid;
    std::printf("[SW] Processo criado: pid=%u name=%s\n", p->pid, p->name);
    return BAIOS_OK;
}

baios_error_t sw_process_kill(baios_pid_t pid, i32 signal) {
    (void)signal;
    baios_process_t *p = sw_process_get(pid);
    if (!p) return BAIOS_ERR_NOT_FOUND;

    p->state = BAIOS_PROC_TERMINATED;
    std::printf("[SW] Processo %u terminado\n", pid);
    return BAIOS_OK;
}

baios_error_t sw_process_wait(baios_pid_t pid, i32 *out_status) {
    baios_process_t *p = sw_process_get(pid);
    if (!p) return BAIOS_ERR_NOT_FOUND;

    if (p->state != BAIOS_PROC_TERMINATED && p->state != BAIOS_PROC_ZOMBIE) {
        return BAIOS_ERR_BUSY;
    }
    if (out_status) *out_status = 0;
    return BAIOS_OK;
}

baios_process_t *sw_process_get(baios_pid_t pid) {
    for (int i = 0; i < BAIOS_MAX_PROCESSES; i++) {
        if (g_proc_used[i] && g_proc_pool[i].pid == pid) {
            return &g_proc_pool[i];
        }
    }
    return nullptr;
}

baios_process_t *sw_process_current(void) {
    return g_sw.current;
}

void sw_scheduler_tick(void) {
    if (!g_sw.process_list) return;

    /* Round-robin extremamente simples */
    baios_process_t *start = g_sw.current ? g_sw.current->next : g_sw.process_list;
    if (!start) start = g_sw.process_list;

    baios_process_t *p = start;
    do {
        if (p->state == BAIOS_PROC_READY || p->state == BAIOS_PROC_RUNNING) {
            if (g_sw.current && g_sw.current != p) {
                g_sw.current->state = BAIOS_PROC_READY;
            }
            g_sw.current = p;
            p->state = BAIOS_PROC_RUNNING;
            return;
        }
        p = p->next ? p->next : g_sw.process_list;
    } while (p != start);
}

baios_error_t sw_scheduler_yield(void) {
    if (g_sw.current) {
        g_sw.current->state = BAIOS_PROC_READY;
    }
    sw_scheduler_tick();
    return BAIOS_OK;
}

baios_error_t sw_kernel_init(void) {
    std::memset(&g_sw, 0, sizeof(g_sw));
    std::memset(g_proc_used, 0, sizeof(g_proc_used));
    g_sw.next_pid = 1;
    g_sw.initialized = true;

    /* Processo idle do sistema */
    baios_pid_t idle_pid;
    sw_process_create("idle", 0, BAIOS_PRIO_IDLE, &idle_pid);

    /* Processo init */
    baios_pid_t init_pid;
    sw_process_create("init", 0, BAIOS_PRIO_NORMAL, &init_pid);
    baios_process_t *init = sw_process_get(init_pid);
    if (init) {
        init->permissions = BAIOS_PERM_DEFAULT_SYSTEM;
        init->uid = 0;
        init->gid = 0;
    }

    g_sw.current = init;
    std::printf("[SW] Software Microkernel inicializado\n");
    return BAIOS_OK;
}

void sw_kernel_shutdown(void) {
    if (!g_sw.initialized) return;
    std::printf("[SW] Desligando Software Microkernel (%u processos)\n",
                g_sw.process_count);
    g_sw.initialized = false;
}

} // extern "C"
