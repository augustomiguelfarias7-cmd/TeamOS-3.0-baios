/**
 * @file process_manager.c
 * @brief Gerenciador de processos do Microkernel de Software
 */

#include "process.h"
#include "baios.h"
#include <string.h>
#include <stdlib.h>

static baios_process_t *g_procs[BAIOS_MAX_PROCESSES];
static baios_process_t *g_current = NULL;
static u32              g_proc_count = 0;
static baios_pid_t      g_next_pid = 1;
static bool             g_proc_ready = false;

baios_status_t baios_proc_init(void) {
    if (g_proc_ready)
        return BAIOS_ERR_STATE;
    memset(g_procs, 0, sizeof(g_procs));
    g_current = NULL;
    g_proc_count = 0;
    g_next_pid = 1;
    g_proc_ready = true;
    BAIOS_LOG_INFO("proc: process manager ready");
    return BAIOS_OK;
}

void baios_proc_shutdown(void) {
    if (!g_proc_ready)
        return;
    for (u32 i = 0; i < BAIOS_MAX_PROCESSES; i++) {
        if (g_procs[i]) {
            free(g_procs[i]);
            g_procs[i] = NULL;
        }
    }
    g_current = NULL;
    g_proc_count = 0;
    g_proc_ready = false;
    BAIOS_LOG_INFO("proc: shutdown");
}

static int find_slot(void) {
    for (u32 i = 0; i < BAIOS_MAX_PROCESSES; i++) {
        if (!g_procs[i])
            return (int)i;
    }
    return -1;
}

baios_status_t baios_proc_create(const char *name, baios_prio_t prio, baios_pid_t *out_pid) {
    if (!g_proc_ready || !name || !out_pid)
        return BAIOS_ERR_INVAL;

    int slot = find_slot();
    if (slot < 0)
        return BAIOS_ERR_OVERFLOW;

    baios_process_t *p = (baios_process_t *)calloc(1, sizeof(*p));
    if (!p)
        return BAIOS_ERR_NOMEM;

    p->pid = g_next_pid++;
    p->ppid = g_current ? g_current->pid : 0;
    strncpy(p->name, name, sizeof(p->name) - 1);
    p->state = BAIOS_PROC_STATE_CREATED;
    p->priority = prio;
    p->thread_count = 1;
    p->capabilities = 0;

    g_procs[slot] = p;
    g_proc_count++;

    if (!g_current)
        g_current = p;

    p->state = BAIOS_PROC_STATE_READY;
    *out_pid = p->pid;

    BAIOS_LOG_DEBUG("proc: created pid=%u name='%s'", p->pid, p->name);
    return BAIOS_OK;
}

baios_status_t baios_proc_exit(baios_pid_t pid, i32 exit_code) {
    (void)exit_code;
    baios_process_t *p = baios_proc_get(pid);
    if (!p)
        return BAIOS_ERR_NOTFOUND;

    p->state = BAIOS_PROC_STATE_ZOMBIE;
    if (g_current == p)
        g_current = NULL;

    BAIOS_LOG_DEBUG("proc: exit pid=%u code=%d", pid, exit_code);
    return BAIOS_OK;
}

baios_status_t baios_proc_kill(baios_pid_t pid) {
    baios_process_t *p = baios_proc_get(pid);
    if (!p)
        return BAIOS_ERR_NOTFOUND;

    p->state = BAIOS_PROC_STATE_DEAD;
    if (g_current == p)
        g_current = NULL;

    /* Libera slot */
    for (u32 i = 0; i < BAIOS_MAX_PROCESSES; i++) {
        if (g_procs[i] == p) {
            free(p);
            g_procs[i] = NULL;
            g_proc_count--;
            break;
        }
    }
    return BAIOS_OK;
}

baios_process_t *baios_proc_get(baios_pid_t pid) {
    if (!g_proc_ready || pid == 0)
        return NULL;
    for (u32 i = 0; i < BAIOS_MAX_PROCESSES; i++) {
        if (g_procs[i] && g_procs[i]->pid == pid)
            return g_procs[i];
    }
    return NULL;
}

baios_process_t *baios_proc_current(void) {
    return g_current;
}

u32 baios_proc_count(void) {
    return g_proc_count;
}
